#include "../core/integrated_world.hpp"
#include "../storage/replay_lineage.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
using namespace genesis;

// A synthetic in-memory index exercises the same contract as a future SD
// adapter, without opening files or introducing a second population validator.
struct FixtureIndex {
    std::uint64_t world_seed{};
    std::vector<DeathRecord> records;
    std::uint64_t seed() const noexcept {return world_seed;}
    std::uint64_t count() const noexcept {return records.size();}
    const DeathRecord* last() const noexcept {return records.empty()?nullptr:&records.back();}
    const DeathRecord* find(std::uint64_t id) const noexcept {
        if(id<3||id-3>=records.size())return nullptr;
        return &records[static_cast<std::size_t>(id-3)];
    }
    template<class Visit> bool visit(const Visit& visitor) const {
        for(const auto& d:records)if(!visitor(d))return false;
        return true;
    }
};
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
    try{
        auto sim=std::make_unique<integrated::Simulation>();
        check(sim->create(42),"initial world admitted");
        auto state=std::make_unique<integrated::State>(sim->state());
        auto& w=state->world;
        state->cadence=60;w.tick=25*ticks_per_year;
        const auto adult=20*ticks_per_year;
        FixtureIndex index{w.seed,{}};
        // These are explicit test fixtures, not naturally simulated births.
        for(std::uint64_t id=3;id<579;++id){
            Citizen c{};c.id=id;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.health=0;c.thirst=100;
            index.records.push_back({c,adult+100,DeathCause::dehydration});
        }
        w.descendant_count=1;w.next_citizen_id=580;
        auto& child=w.descendants[0];
        child.id=579;child.parent1=3;child.parent2=4;child.birth_tick=adult;
        const auto validate=[&](const World& world){return storage::valid_history_view(world,index);};
        check(integrated::valid_with_population(*state,validate),"integrated boundary accepts verified external ancestry");
        check(!integrated::valid(*state),"legacy checkpoint validation must still refuse external identities");
        auto cache=storage::ReplayLineage::build(w,index);
        check(cache.valid(w)&&cache.reference_count()==2,"bounded lineage cache retains only needed archived parents");
        child.birth_tick=adult+101;
        check(!integrated::valid_with_population(*state,validate),"parent death before birth rejected");
        child.birth_tick=adult;
        child.id=578;
        check(!integrated::valid_with_population(*state,validate),"duplicate external identity rejected");
        child.id=579;
        ++w.next_citizen_id;
        check(!integrated::valid_with_population(*state,validate),"missing lifetime identity rejected");
        --w.next_citizen_id;
        ++index.world_seed;
        check(!integrated::valid_with_population(*state,validate),"foreign history rejected");
        --index.world_seed;
        check(integrated::valid_with_population(*state,validate),"valid fixture restored");
        std::cout<<"PASS integrated external history validation seam above 512 identities\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
