#pragma once
#include "identified_checkpoint.hpp"
#include <limits>
namespace genesis::storage::identified_checkpoint {
// Adapter contract: read distinguishes absent files from I/O failure;
// publish writes ONLY the chosen inactive slot, flushes and closes it.
// Caller holds exclusive access and supplies independently persisted identity.
struct SlotRead {Bytes bytes;bool missing{};};
struct SaveResult {std::uint64_t generation{};int slot{-1};std::string error;};
template<class Adapter>
inline Recovery load(Adapter& io,const world_identity::Identity& id) {
    try {
        const auto a=io.read(0),b=io.read(1);
        return recover(a.bytes,b.bytes,id);
    }catch(const std::exception& e){Recovery r{};r.checkpoint.state.error=e.what();return r;}
}
template<class Adapter>
inline SaveResult save(Adapter& io,const world_identity::Identity& id,
                       const integrated::Simulation& sim,bool explicit_create=false) {
    try {
        if(!world_identity::valid(id)||!sim.ready()) throw std::runtime_error("invalid save inputs");
        const auto a=io.read(0),b=io.read(1);
        auto prior=recover(a.bytes,b.bytes,id);
        std::uint64_t generation=1;int slot=0;
        if(prior.checkpoint.state.simulation) {
            const auto& old=*prior.checkpoint.state.simulation;
            const auto& x=old.state();const auto& y=sim.state();
            if(!integrated_checkpoint::same_founding(x,y)||x.world.tick>y.world.tick||
               (x.cadence&&x.cadence!=y.cadence))
                throw std::runtime_error("save conflicts with persisted world");
            if(x.world.tick==y.world.tick&&integrated_checkpoint::encode(old,1)!=integrated_checkpoint::encode(sim,1))
                throw std::runtime_error("different state at same tick");
            if(prior.checkpoint.state.generation==std::numeric_limits<std::uint64_t>::max())
                throw std::runtime_error("generation exhausted");
            generation=prior.checkpoint.state.generation+1;slot=1-prior.checkpoint.slot;
        }else if(!explicit_create||!a.missing||!b.missing||sim.state().world.tick!=0) {
            throw std::runtime_error("refusing automatic initialization or overwrite: "+prior.checkpoint.state.error);
        }
        // Release recovered simulation before allocating the next serialized buffer.
        prior.checkpoint.state.simulation.reset();
        const auto bytes=encode(id,sim,generation);
        io.publish(slot,bytes);
        const auto verified=io.read(slot);
        if(verified.missing||verified.bytes!=bytes) throw std::runtime_error("checkpoint readback mismatch");
        return {generation,slot,{}};
    }catch(const std::exception& e){return {0,-1,e.what()};}
}
}
