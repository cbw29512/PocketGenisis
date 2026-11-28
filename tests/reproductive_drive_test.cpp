#define PG_FATIGUE_PROBE
#define PG_REPRODUCTIVE_DRIVE_PROBE
#include "../core/ecology.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace drive=genesis::reproductive_drive;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
static void fixture(World& w,std::uint64_t seed,unsigned motivation) {
    initialize(w,seed);w.tick=600;
    for(auto& a:w.founders) {
        a.reproductive_drive=static_cast<std::uint8_t>(motivation);
        auto& t=w.tiles[detail::tile_key(a)];t.food=80;t.water=80;
    }
}
int main(){
    try {
        auto w=std::make_unique<World>();
        fixture(*w,42,50);
        drive::advance(*w);
        check(w->founders[0].reproductive_drive==58&&w->founders[1].reproductive_drive==58,
              "sparse local suitable population increases motivation");
        // Valid controlled family: six local infants count as resource
        // competition but can never become eligible parents.
        fixture(*w,42,50);
        for(unsigned i=0;i<6;++i) {
            auto& c=w->descendants[w->descendant_count++];
            c.id=w->next_citizen_id++;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.x=w->founders[0].x;c.y=w->founders[0].y;c.reproductive_drive=50;
        }
        check(valid_population(*w),"crowding fixture valid");
        drive::advance(*w);
        check(w->founders[0].reproductive_drive==46&&w->descendants[0].reproductive_drive==0,
              "crowding reduces motivation; infants have no reproductive drive");
        for(std::size_t i=0;i<w->descendant_count;++i) {
            w->descendants[i].x=0;w->descendants[i].y=0;
        }
        drive::advance(*w);
        check(w->founders[0].reproductive_drive==54,"remote population does not suppress local motivation");
        fixture(*w,42,100);
        w->tiles[detail::tile_key(w->founders[0])].food=0;
        attempt_conception(*w);
        check(w->founders[0].reproductive_drive==92&&!w->pregnancy_count,"scarcity blocks conception despite strong drive");
        fixture(*w,42,100);
        w->founders[0].fatigue=90;w->founders[0].action=Action::rest;
        attempt_conception(*w);
        check(!w->pregnancy_count,"resting parent cannot conceive");
        w->founders[0].reproductive_drive=101;
        check(!valid_agent(w->founders[0]),"invalid motivation rejected");
        unsigned low=0,high=0;
        // Paired seeds and identical resources/eligible pairs: only
        // motivation varies. Actual conception transaction, not fake births.
        for(unsigned seed=0;seed<128;++seed) {
            fixture(*w,seed,0);attempt_conception(*w);low+=w->pregnancy_count;
            fixture(*w,seed,90);attempt_conception(*w);high+=w->pregnancy_count;
            if(w->pregnancy_count) {
                check(w->pregnancies[0].parent1==1&&w->pregnancies[0].parent2==2&&
                      w->pregnancies[0].due_tick==w->tick+gestation_ticks&&
                      w->founders[0].reproductive_drive==58&&valid_population(*w),
                      "ordinary gestation and motivation relief");
                // The busy pair remains ineligible even with max drive.
                w->tick+=600;w->founders[0].reproductive_drive=100;
                w->founders[1].reproductive_drive=100;attempt_conception(*w);
                check(w->pregnancy_count==1,"no overlapping gestation");
            }
        }
        check(high>low&&high<128,"bounded motivation increases likelihood without guaranteeing births");
        // Real annual ticks with survival, disease, fatigue and lifecycle.
        initialize(*w,42);auto twin=std::make_unique<World>();initialize(*twin,42);
        Production p{},q{};
        for(unsigned n=0;n<43200;++n) {
            check(step_world(*w,p)&&step_world(*twin,q),"integrated lifecycle tick");
            check(w->descendant_count==twin->descendant_count&&w->pregnancy_count==twin->pregnancy_count&&
                  w->founders[0].reproductive_drive==twin->founders[0].reproductive_drive&&
                  w->founders[1].reproductive_drive==twin->founders[1].reproductive_drive&&
                  w->streams[reproduction].state==twin->streams[reproduction].state,"same-seed drive replay");
        }
        check(valid_population(*w)&&valid_population(*twin),"actual lifecycle validity");
        std::cout<<"REPRODUCTIVE_DRIVE low="<<low<<" high="<<high
                 <<" living="<<w->descendant_count+2<<" pregnancies="<<w->pregnancy_count
                 <<" citizen_bytes="<<sizeof(Citizen)<<'\n';
        std::cout<<"PASS local reproductive feedback, ordinary gestation and replay\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
