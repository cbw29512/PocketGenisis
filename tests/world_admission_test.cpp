#include "../core/world_admission.hpp"
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
using namespace genesis;
namespace admission=genesis::world_admission;
namespace tp=genesis::terrain_probe;
namespace tr=genesis::terrain_resources;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
struct Candidate {
    World w{};
    tp::State terrain{};
    tr::State minerals{};
    wildlife::Wildlife wild{};
    freshwater::State fish{};
    admission::Selection generate(std::uint64_t seed,unsigned attempts=admission::max_attempts) {
        return admission::generate_candidate(w,terrain,minerals,wild,fish,seed,attempts);
    }
    admission::Report inspect() const {return admission::inspect(w,terrain,minerals,wild,fish);}
};
int main(){
    try {
        auto a=std::make_unique<Candidate>();auto b=std::make_unique<Candidate>();
        std::set<std::uint64_t> hashes;
        unsigned total_attempts=0,max_attempts=0;
        for(std::uint64_t seed=0;seed<32;++seed) {
            const auto x=a->generate(seed);const auto y=b->generate(seed);
            check(x.accepted&&y.accepted&&x.attempts<=admission::max_attempts,"bounded initial world qualification");
            check(x.accepted_seed==y.accepted_seed&&x.attempts==y.attempts&&
                  x.report.spawn_a==y.report.spawn_a&&x.report.spawn_b==y.report.spawn_b&&
                  a->w.seed==x.accepted_seed&&a->w.tick==0,"same seed deterministic admission");
            check(a->inspect().accepted&&b->inspect().accepted,"accepted generated stocks");
            check(a->w.founders[0].x+1==a->w.founders[1].x&&
                  a->w.founders[0].y==a->w.founders[1].y,"adjacent viable founding pair");
            for(unsigned k=1;k<x.report.ore.size();++k)
                check(x.report.ore[k]>=admission::min_mineral_units,"all required finite minerals present");
            for(auto n:x.report.groups) check(n>=admission::min_species_groups,"breeding-size animal groups");
            for(unsigned k=0;k<stream_count;++k)
                check(a->w.streams[k].state==b->w.streams[k].state,"same live RNG initialization");
            const auto hash=tp::fingerprint(a->terrain);
            check(hash==tp::fingerprint(b->terrain),"same generated terrain");
            hashes.insert(hash);total_attempts+=x.attempts;
            max_attempts=std::max(max_attempts,x.attempts);
        }
        check(hashes.size()==32,"distinct accepted geography");
        const auto selected=a->generate(42);
        check(selected.accepted,"controlled seed42");
        // A qualified initial world can later lose resources. Inspection
        // rejects missing ore; it does not generate or restore anything.
        for(auto& d:a->minerals.deposits) if(d.kind==tr::Mineral::iron) d.remaining=0;
        check(!a->inspect().accepted&&a->inspect().ore[unsigned(tr::Mineral::iron)]==0,"missing required ore rejected without refill");
        check(a->generate(42).accepted,"fresh disposable fixture");
        for(auto& t:a->w.tiles) t.water=0;
        check(!a->inspect().accepted&&!a->inspect().founder_pairs,"no viable water or founder location rejected");
        check(a->generate(42).accepted,"fresh fish fixture");
        for(std::size_t i=0;i<a->fish.cells.size();++i)
            while(freshwater::catch_one(a->fish,i)) {}
        check(freshwater::valid(a->fish)&&!a->inspect().accepted&&
              freshwater::population(a->fish)==0,"depleted fish never replenished by qualification");
        check(a->generate(42).accepted,"fresh elapsed fixture");
        ++a->w.tick;
        const auto hash=tp::fingerprint(a->terrain);
        check(!a->inspect().accepted&&tp::fingerprint(a->terrain)==hash,"existing elapsed world not an initial candidate");
        check(!b->generate(42,0).accepted,"bounded exhaustion fails closed");
        std::cout<<"WORLD_ADMISSION seeds=32 distinct="<<hashes.size()
                 <<" total_attempts="<<total_attempts<<" max_attempts="<<max_attempts<<'\n';
        std::cout<<"PASS initial resource diversity, viable spawn, deterministic bounded admission and no refill\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
