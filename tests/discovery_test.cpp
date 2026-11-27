// Emulate the Arduino preprocessor definition before including the core.
#define bit(b) (1UL << (b))
#include "../core/discovery.hpp"
#include "../core/ecology.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
using namespace genesis::discovery;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
    try {
        auto w=std::unique_ptr<World>(new World);
        genesis::initialize(*w,42);State s{};
        check(!eligible(0,shelter)&&eligible(tech_bit(stone_tools),shelter),"prerequisites");
        w->founders[1].health=1;w->tick=60;
        auto& tile=local(*w,w->founders[0]);tile.wood=0;tile.stone=0;tile.food=0;
        auto rng=w->streams[research].state;
        check(advance(*w,s,probe_cadence).kind==EventKind::none&&w->streams[research].state==rng,"unfunded experiment");
        tile.wood=10;tile.stone=10;tile.food=10;w->tick=61;
        w->founders[0].action=Action::drink;
        check(advance(*w,s,probe_cadence).kind==EventKind::none,"busy opportunity stays open");
        w->founders[0].action=Action::idle;w->tick=62;
        auto event=advance(*w,s,probe_cadence);
        check(event.kind==EventKind::started&&s.attempts==1&&s.job.active,"start");
        const auto rule=rules[event.tech];
        check(tile.wood==10-rule.wood&&tile.stone==10-rule.stone&&tile.food==10-rule.food,"materials");
        w->founders[0].hunger=100;++w->tick;advance(*w,s,probe_cadence);
        check(s.job.work==0,"survival priority");
        w->founders[0].hunger=0;
        // Choose a deterministic RNG state whose next bounded draw fails.
        for(std::uint64_t seed=0;;++seed) {Rng r{seed};std::uint32_t roll{};r.bounded(100,roll);
            if(roll>=chance(w->founders[0],static_cast<Tech>(event.tech))) {w->streams[research].state=seed;break;}}
        for(unsigned i=0;i<work_required;++i) {++w->tick;event=advance(*w,s,probe_cadence);}
        check(event.kind==EventKind::failed&&s.failures==1&&s.known[0]==0&&discovered(s)==0,"failed experiment");
        check(tile.wood==10-rule.wood,"failure does not refund");
        w->tick=180;event=advance(*w,s,probe_cadence);
        check(event.kind==EventKind::started,"retry");
        for(std::uint64_t seed=0;;++seed) {Rng r{seed};std::uint32_t roll{};r.bounded(100,roll);
            if(roll<chance(w->founders[0],static_cast<Tech>(event.tech))) {w->streams[research].state=seed;break;}}
        for(unsigned i=0;i<work_required;++i) {++w->tick;event=advance(*w,s,probe_cadence);}
        check(event.kind==EventKind::discovered&&discovered(s)==1&&s.successes==1,"successful discovery");
        w->founders[1].health=100;w->tick=240;
        check(teach(*w,s).kind==EventKind::taught&&s.known[0]==s.known[1]&&discovered(s)==1,"local teaching");
        auto a=std::unique_ptr<World>(new World),b=std::unique_ptr<World>(new World);
        genesis::initialize(*a,77);genesis::initialize(*b,77);State x{},y{};
        const auto other=a->streams[reproduction].state;
        for(unsigned i=1;i<=3600;++i) {
            a->tick=b->tick=i;const auto e=advance(*a,x,probe_cadence),f=advance(*b,y,probe_cadence);
            check(e.kind==f.kind&&e.tech==f.tech&&e.founder==f.founder,"event parity");
        }
        check(x.known==y.known&&x.attempts==y.attempts&&x.successes==y.successes&&x.failures==y.failures&&a->streams[research].state==b->streams[research].state,"state parity");
        check(a->streams[reproduction].state==other&&discovered(x)>0,"isolated RNG and discovery");
        // Exercise the real survival/ecology clock, not hand-set idle citizens.
        genesis::initialize(*w,42);State actual{};Production production{};
        for(unsigned i=0;i<3600;++i) {
            check(step_world(*w,production),"authoritative world tick");
            advance(*w,actual,probe_cadence);
        }
        check(actual.attempts>0&&actual.successes>0&&discovered(actual)>0,"live founders must discover");
        check(actual.attempts==actual.successes+actual.failures+actual.abandoned+unsigned(actual.job.active),"live accounting");
        // A stalled job must release its slot without a research RNG roll or refund.
        genesis::initialize(*w,42);State stalled{};w->founders[1].health=1;w->tick=60;
        check(advance(*w,stalled,probe_cadence).kind==EventKind::started,"start stall fixture");
        const auto saved_rng=w->streams[research].state;
        w->founders[0].hunger=100;w->tick+=stall_limit-1;
        check(advance(*w,stalled,probe_cadence).kind==EventKind::none&&stalled.job.active,"retain short interruption");
        ++w->tick;
        check(advance(*w,stalled,probe_cadence).kind==EventKind::abandoned&&!stalled.job.active&&stalled.abandoned==1,"release stalled job");
        check(w->streams[research].state==saved_rng&&stalled.failures==0,"no fabricated failed roll");
        w->founders[0].hunger=0;
        ++w->tick;check(advance(*w,stalled,probe_cadence).kind==EventKind::started,"slot reusable");
        std::cout<<"PASS: discovery prerequisites, costs, failure, teaching, replay\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
