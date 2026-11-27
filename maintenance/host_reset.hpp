#pragma once
#include "../storage/host_checkpoint.hpp"
#include <filesystem>

namespace genesis::maintenance {
// Host prototype: these facts MUST come from a trusted device-side adapter,
// never from a USB command payload or touchscreen. No adapter exists yet.
enum class Transport { touchscreen, network, physical_usb };
struct Access {
    Transport transport{Transport::touchscreen};
    bool authenticated{}, simulation_paused{};
};
inline bool allowed(const Access& a) noexcept {
    return a.transport==Transport::physical_usb&&a.authenticated&&a.simulation_paused;
}
struct ResetPermit {
    storage::Bytes snapshot;
    std::string confirmation, error;
    bool consumed{};
    ResetPermit()=default;
    ResetPermit(const ResetPermit&)=delete;
    ResetPermit& operator=(const ResetPermit&)=delete;
    ResetPermit(ResetPermit&&)=default;
    ResetPermit& operator=(ResetPermit&&)=default;
};
// Caller holds exclusive maintenance access until staging finishes.
inline ResetPermit prepare_reset(const std::string& active,const World& live,const Access& access) {
    ResetPermit permit;
    try {
        if(!allowed(access)) throw std::runtime_error("authenticated physical USB and paused simulation required");
        auto current=storage::load_host(active);
        if(!current.state.world) throw std::runtime_error("active checkpoint unavailable: "+current.state.error);
        const auto generation=current.state.generation;
        permit.snapshot=storage::encode(live,generation);
        if(permit.snapshot!=storage::encode(*current.state.world,generation))
            throw std::runtime_error("checkpoint must match the paused live world");
        // Seed is a provisional test identity, NOT a production world UUID.
        permit.confirmation="DESTROY "+std::to_string(live.seed)+" GENERATION "+
            std::to_string(generation)+" TICK "+std::to_string(live.tick);
    } catch(const std::exception& e) {permit.snapshot.clear();permit.error=e.what();}
    return permit;
}
struct ResetResult {bool staged{};std::string error;};
inline ResetResult stage_reset(ResetPermit& permit,const Access& access,const World& live,
                               const std::string& active,const std::string& backup,
                               const std::string& target,const std::string& confirmation,
                               std::uint64_t new_seed) {
    try {
        if(permit.consumed) throw std::runtime_error("reset permit already used");
        permit.consumed=true; // Every attempt consumes authorization, including rejection.
        if(!allowed(access)||permit.snapshot.empty()||!permit.error.empty())
            throw std::runtime_error("reset authorization refused");
        if(confirmation!=permit.confirmation) throw std::runtime_error("confirmation does not match");
        if(new_seed==live.seed) throw std::runtime_error("replacement seed must differ");
        // Exact snapshot binds the authorization to this paused world and generation.
        auto current=storage::load_host(active);
        if(!current.state.world||storage::encode(*current.state.world,current.state.generation)!=permit.snapshot||
           storage::encode(live,current.state.generation)!=permit.snapshot)
            throw std::runtime_error("world changed; prepare a new reset");
        namespace fs=std::filesystem;
        const auto active_dir=fs::canonical(active),target_dir=fs::canonical(target);
        const auto backup_file=fs::canonical(backup);
        if(active_dir==target_dir||backup_file.parent_path()==active_dir||backup_file.parent_path()==target_dir||
           !fs::is_empty(target_dir)) throw std::runtime_error("separate backup and empty replacement directory required");
        const auto saved=storage::read_file(backup_file.string());
        if(saved.missing||saved.bytes!=permit.snapshot||!storage::decode(saved.bytes).world)
            throw std::runtime_error("verified exact backup required");
        // Stage only: no deletion, active-world switch, or official Genesis event.
        auto replacement=std::make_unique<World>();initialize(*replacement,new_seed);
        auto first=storage::save_host(target_dir.string(),*replacement,true);
        if(first.generation!=1) throw std::runtime_error("replacement write failed: "+first.error);
        auto second=storage::save_host(target_dir.string(),*replacement);
        if(second.generation!=2) throw std::runtime_error("replacement redundancy failed: "+second.error);
        auto result=storage::load_host(target_dir.string());
        if(!result.state.world||result.degraded||result.state.generation!=2||
           storage::encode(*result.state.world,2)!=storage::encode(*replacement,2))
            throw std::runtime_error("replacement readback failed");
        return {true,{}};
    } catch(const std::exception& e) {return {false,e.what()};}
}
} // namespace genesis::maintenance
