#include "ecology.hpp"
#include "../storage/checkpoint.hpp"
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
static std::uint64_t roll_fixture(unsigned bound,unsigned minimum,unsigned maximum) {
    for(std::uint64_t i=0;i<100000;++i) {
        Rng r{i};std::uint32_t roll{};r.bounded(bound,roll);
        if(roll>=minimum&&roll<maximum) return i;
    }
    throw std::runtime_error("missing deterministic roll fixture");
}
static void clear_resources(World& w) {
    for(auto& t:w.tiles) {t.food=0;t.water=0;}
}
int main() {
    try {
        auto w=std::make_unique<World>();initialize(*w,42);
        auto& a=w->founders[0];auto& b=w->founders[1];
        a.health=20;a.personality[0]=100;w->tick=1;
        const auto old_x=a.x,old_y=a.y;
        const auto rng=w->streams[decisions].state;
        act_survival(*w,a);
        check(a.action==Action::idle&&a.x==old_x&&a.y==old_y&&w->streams[decisions].state==rng,
              "incapacitated founder rests without exploration");
        clear_resources(*w);a.thirst=100;
        w->tiles[static_cast<std::size_t>(a.y)*width+a.x+1].water=1;
        act_survival(*w,a);
        check(a.x==old_x&&a.action==Action::idle,"no crawl between movement windows");
        w->tick=4;act_survival(*w,a);
        check(a.x==old_x+1&&a.action==Action::move,"bounded emergency crawl");
        act_survival(*w,a);
        check(a.action==Action::drink&&a.thirst==88,"real supply consumed despite incapacity");
        initialize(*w,42);clear_resources(*w);w->tick=1;
        a.x=50;a.y=30;b.x=51;b.y=30;a.health=1;a.thirst=100;
        auto& stock=w->tiles[30*width+51];stock.water=1;
        check(step_survival(*w),"assistance tick");
        check(a.action==Action::drink&&b.action==Action::care&&a.thirst==88&&stock.water==0,
              "partner assistance conserves one water unit and uses one action");
        check(!assist_founder(*w,b,a),"cannot provide two actions");
        initialize(*w,42);clear_resources(*w);
        for(auto& founder:w->founders) {founder.health=1;founder.hunger=100;founder.thirst=100;}
        Production p{};
        check(step_world(*w,p)&&a.health==1&&b.health==1,"no magical health without supplies");
        // Actual renewable-resource recovery after a complete mortal collapse.
        initialize(*w,42);w->tick=70*ticks_per_year;
        for(auto& founder:w->founders) {
            founder.health=1;founder.hunger=100;founder.thirst=100;founder.personality[0]=0;
        }
        w->descendant_count=1;w->next_citizen_id=4;
        auto& c=w->descendants[0];c.id=3;c.parent1=1;c.parent2=2;c.birth_tick=1;
        c.lifespan_years=60;c.purpose=Purpose::gatherer;
        clear_resources(*w);
        check(step_world(*w,p)&&w->descendant_count==0&&w->archive_count==1,
              "last mortal dies through ordinary mortality");
        check(a.health==1&&b.health==1&&!eligible_parent(a,w->tick),"founders survive but cannot conceive");
        // No manual replenishment after the collapse; ordinary seasonal renewal.
        const auto deadline=w->tick+5000;
        while(w->tick<deadline&&(a.health<50||b.health<50))
            check(step_world(*w,p),"natural recovery step");
        check(a.health>=50&&b.health>=50&&w->archive_count==1,"renewable resources enable recovery");
        // Test save/resume in the recovered state and deterministic conception.
        auto saved=encode(*w,1);auto restored=decode(saved);
        check(restored.world&&encode(*restored.world,1)==saved,"founder state roundtrip");
        // Both partners must recover; conception remains conditional and seeded.
        const auto birth_deadline=w->tick+gestation_ticks+12000;
        while(w->tick<birth_deadline&&w->descendant_count==0) {
            if((w->tick+1)%600==0&&!w->pregnancy_count)
                w->streams[reproduction].state=roll_fixture(4,0,1);
            if(w->pregnancy_count&&w->pregnancies[0].due_tick==w->tick+1)
                w->streams[health_events].state=roll_fixture(10000,1500,10000);
            check(step_world(*w,p),"repopulation step");
        }
        check(w->descendant_count>0&&w->descendants[0].id==4&&
              w->descendants[0].parent1==1&&w->descendants[0].parent2==2&&
              w->archive[0].citizen.id==3&&valid_population(*w),"actual birth after collapse, no reset or identity reuse");
        auto unsupported=encode(*w,2);unsupported[6]=15;
        const auto crc=crc32(unsupported,unsupported.size()-4);
        for(unsigned i=0;i<4;++i) unsupported[unsupported.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
        check(decode(unsupported).unsupported,"old rules rejected without silent migration");
        std::cout<<"PASS: founder incapacitation, conserved aid, natural recovery and post-collapse birth\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
