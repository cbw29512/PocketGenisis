#include "../core/integrated_world.hpp"
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto a=std::make_unique<integrated::Simulation>();
        check(!a->step(60)&&!a->ready(),"uninitialized world cannot advance");
        check(!a->create(42,0)&&!a->step(60)&&!a->create(42),"failed candidate cannot advance or retry in place");
        a=std::make_unique<integrated::Simulation>();
        check(a->create(42)&&a->ready(),"explicit creation");
        const auto& s=a->state();
        check(!a->create(9)&&s.world.seed==42&&s.world.tick==0,"cannot reset admitted world at tick zero");
        check(!a->step(0)&&s.world.tick==0,"invalid cadence refused");
        auto b=std::make_unique<integrated::Simulation>();
        check(b->create(42),"same seed creation");
        const auto& t=b->state();
        for(unsigned n=0;n<3600;++n) {
            check(a->step(60)&&b->step(60),"single authoritative tick");
            check(s.world.tick==n+1&&s.world.tick==t.world.tick,"one tick per call");
        }
        check(!a->create(99)&&integrated::valid(s)&&integrated::valid(t),"running state and cross-system ledgers valid");
        for(std::size_t i=0;i<s.world.tiles.size();++i) {
            const auto& u=s.world.tiles[i];const auto& v=t.world.tiles[i];
            check(u.food==v.food&&u.water==v.water&&u.wood==v.wood&&u.stone==v.stone,"resource replay");
            check(s.fish.cells[i].fish==t.fish.cells[i].fish&&s.fish.cells[i].forage==t.fish.cells[i].forage,
                  "aquatic replay");
            check(s.wild.tiles[i].count==t.wild.tiles[i].count&&s.wild.tiles[i].meat==t.wild.tiles[i].meat,
                  "wildlife replay");
        }
        for(unsigned i=0;i<stream_count;++i)
            check(s.world.streams[i].state==t.world.streams[i].state,"RNG replay");
        // Reuse the existing 32-seed acceptance suite with actual drainage
        // and the integrated owner, rather than the older habitat proxy.
        std::set<std::uint64_t> hashes;unsigned attempts=0,peak=0;
        for(unsigned seed=0;seed<32;++seed) {
            auto candidate=std::make_unique<integrated::Simulation>();
            check(candidate->create(seed)&&integrated::valid(candidate->state()),"connected world admission");
            const auto& w=candidate->state();
            hashes.insert(terrain_probe::fingerprint(w.terrain));
            attempts+=w.founding.attempts;peak=std::max(peak,w.founding.attempts);
        }
        check(hashes.size()==32,"distinct connected maps");
        std::cout<<"INTEGRATED_WORLD seeds=32 distinct="<<hashes.size()<<" attempts="<<attempts
                 <<" max_attempts="<<peak<<" state_bytes="<<sizeof(integrated::State)<<'\n';
        std::cout<<"PASS integrated ownership, creation refusal, cross-system accounting and replay\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
