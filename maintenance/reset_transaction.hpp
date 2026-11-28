#pragma once
#include <string>
#include <stdexcept>
namespace genesis::maintenance {
// Reset is separate from Genesis. The caller owns the paused simulation and
// exclusive card access. No credentials or mandatory backups for ordinary play.
// Adapter contract: inspect the dedicated Pocket Genesis namespace only;
// create a durable reset marker outside the world directory before deletion;
// remove only that namespace, verify absence, then remove the marker.
// If interrupted, boot must detect the marker and refuse automatic Genesis.
enum class ResetStatus { refused, incomplete, empty };
struct ResetResult {ResetStatus status{ResetStatus::refused};std::string error;};
struct ResetPermit {
    std::string identity,confirmation;
    bool consumed{};
};
inline ResetPermit prepare_reset(const std::string& identity,bool paused){
    if(!paused||identity.empty())throw std::runtime_error("paused world and identity required");
    return {identity,"DELETE POCKET GENESIS "+identity,false};
}
template<class Adapter>
ResetResult execute_reset(Adapter& io,ResetPermit& permit,const std::string& confirmation,bool paused){
    bool started=false;
    try{
        if(permit.consumed)throw std::runtime_error("reset confirmation already consumed");
        permit.consumed=true;
        if(!paused||permit.identity.empty()||confirmation!="DELETE POCKET GENESIS "+permit.identity||confirmation!=permit.confirmation)
            throw std::runtime_error("reset confirmation refused");
        // The marker is bound to the old identity. A changed card/world is not erased.
        if(io.identity()!=permit.identity)throw std::runtime_error("world identity changed");
        if(io.pending())throw std::runtime_error("existing reset must be resumed explicitly");
        started=true;io.begin_reset(permit.identity);
        if(!io.pending()||io.reset_identity()!=permit.identity)throw std::runtime_error("reset marker not durable or identity mismatch");
        io.erase_world_files(permit.identity);
        if(!io.world_absent())throw std::runtime_error("world deletion incomplete");
        io.finish_reset();
        if(io.pending())throw std::runtime_error("reset marker not cleared");
        return {ResetStatus::empty,{}};
    }catch(const std::exception& e){
        return {started?ResetStatus::incomplete:ResetStatus::refused,e.what()};
    }
}
template<class Adapter>
ResetResult resume_reset(Adapter& io,const std::string& confirmation,bool paused){
    try{
        if(!paused||!io.pending())return {ResetStatus::refused,"no paused pending reset"};
        const auto identity=io.reset_identity();
        if(identity.empty()||confirmation!="DELETE POCKET GENESIS "+identity)
            return {ResetStatus::refused,"reset confirmation refused"};
        // If the directory still exists, the adapter must verify its identity
        // before deletion; missing identity is allowed only for a proven empty shell.
        if(!io.world_absent())io.erase_world_files(identity);
        if(!io.world_absent())throw std::runtime_error("world deletion incomplete");
        io.finish_reset();
        if(io.pending())throw std::runtime_error("reset marker not cleared");
        return {ResetStatus::empty,{}};
    }catch(const std::exception& e){return {ResetStatus::incomplete,e.what()};}
}
}
