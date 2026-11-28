#pragma once
#include "integrated_checkpoint.hpp"
#include <array>
namespace genesis::storage::integrated_checkpoint {
// Identity of the founding attempt is a provisional consistency check,
// NOT a unique world UUID. Production storage must also bind a durable UUID.
inline bool same_founding(const integrated::State& a,const integrated::State& b) noexcept {
    return a.world.seed==b.world.seed&&
        a.founding.requested_seed==b.founding.requested_seed&&
        a.founding.accepted_seed==b.founding.accepted_seed&&
        a.founding.attempts==b.founding.attempts&&
        a.founding.report.spawn_a==b.founding.report.spawn_a&&
        a.founding.report.spawn_b==b.founding.report.spawn_b&&
        terrain_probe::fingerprint(a.terrain)==terrain_probe::fingerprint(b.terrain);
}
// Preserve unsupported formats rather than treating them as corrupt slots.
inline bool unsupported(const Bytes& bytes) {
    if(bytes.size()<20||bytes.size()>limit) return false;
    const auto end=bytes.size()-4;
    Reader tail{bytes,end,bytes.size()};std::uint32_t crc{};tail.field(crc);
    if(crc!=crc32(bytes,end)) return false;
    Reader in{bytes,0,end};std::uint32_t m{};std::uint16_t v{},r{};
    in.field(m);in.field(v);in.field(r);
    return m==magic&&(v!=version||r!=rules);
}
struct Recovery {
    Decoded state{};
    int slot{-1};
    bool degraded{},migration_required{};
};
inline Recovery recover(const Bytes& a,const Bytes& b) {
    Recovery result{};
    try {
        if(unsupported(a)||unsupported(b)) {
            result.migration_required=true;
            throw std::runtime_error("integrated checkpoint migration required");
        }
        auto x=decode(a),y=decode(b);
        if(!x.simulation&&!y.simulation) throw std::runtime_error("no valid integrated checkpoint");
        if(x.simulation&&y.simulation) {
            const auto& xs=x.simulation->state();const auto& ys=y.simulation->state();
            if(!same_founding(xs,ys)||(xs.cadence&&ys.cadence&&xs.cadence!=ys.cadence)||
               (x.generation==y.generation&&a!=b)||
               (x.generation>y.generation&&xs.world.tick<ys.world.tick)||
               (y.generation>x.generation&&ys.world.tick<xs.world.tick)||
               (xs.world.tick==ys.world.tick&&encode(*x.simulation,1)!=encode(*y.simulation,1)))
                throw std::runtime_error("conflicting integrated checkpoints");
        } else result.degraded=true;
        result.slot=x.simulation&&(!y.simulation||x.generation>=y.generation)?0:1;
        result.state=result.slot==0?std::move(x):std::move(y);
    }catch(const std::exception& e){result.state.error=e.what();}
    return result;
}
}
