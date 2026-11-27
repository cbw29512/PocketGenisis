#include "../core/construction_work.hpp"
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace genesis;
namespace build=genesis::construction;
struct Summary {
    std::string order,layout,history;
    unsigned discoveries{},buildings{},births{},living{},deaths{};
    std::uint64_t first_birth{},last_discovery{};
};
static void check(bool ok,const char* message){if(!ok) throw std::runtime_error(message);}
static Summary simulate(std::uint64_t seed,unsigned ticks) {
    auto w=std::unique_ptr<World>(new World);
    initialize(*w,seed);discovery::State research{};build::State sites{};Production p{};
    Summary result{};std::ostringstream trace,layout;
    for(unsigned i=0;i<ticks;++i) {
        discovery::Event re{};build::Event ce{};
        check(build::step(*w,research,sites,p,re,ce,discovery::probe_cadence),"rejected tick");
        check(re.kind==discovery::EventKind::none||ce.kind==build::EventKind::none,"overlapping tasks");
        if(re.kind!=discovery::EventKind::none) {
            trace<<"R:"<<w->tick<<':'<<unsigned(re.kind)<<':'<<unsigned(re.tech)<<':'<<re.founder<<';';
            if(re.kind==discovery::EventKind::discovered) {
                result.order+=char('A'+re.tech);result.last_discovery=w->tick;
            }
        }
        if(ce.kind!=build::EventKind::none)
            trace<<"B:"<<w->tick<<':'<<unsigned(ce.kind)<<':'<<unsigned(ce.type)<<':'<<ce.tile<<':'<<ce.builder<<';';
        for(std::size_t j=0;j<w->descendant_count;++j) {
            const auto& c=w->descendants[j];
            if(c.birth_tick!=w->tick) continue;
            if(!result.births) result.first_birth=w->tick;
            ++result.births;trace<<"C:"<<w->tick<<':'<<c.id<<':'<<c.parent1<<':'<<c.parent2<<';';
        }
    }
    check(valid_population(*w),"invalid final population");
    check(research.attempts==research.successes+research.failures+research.abandoned+unsigned(research.job.active),"research accounting");
    for(unsigned i=0;i<build::kind_count;++i) {
        const auto& site=sites.sites[i];
        if(site.stage==build::Stage::empty) continue;
        check(site.tile<width*height,"invalid structure tile");
        for(unsigned j=0;j<i;++j)
            check(sites.sites[j].stage==build::Stage::empty||sites.sites[j].tile!=site.tile,"overlapping structures");
        if(site.stage==build::Stage::complete) layout<<i<<':'<<site.tile<<';';
    }
    result.layout=layout.str();result.discoveries=discovery::discovered(research);
    result.buildings=build::completed(sites);result.living=unsigned(w->descendant_count+2);
    result.deaths=unsigned(w->archive_count);
    // Compare actual outcomes, not seed labels or initial RNG state.
    trace<<"F:"<<result.living<<':'<<result.deaths<<':'<<research.known[0]<<':'<<research.known[1]<<':'<<w->pregnancy_count;
    for(const auto& c:w->founders) trace<<':'<<c.x<<':'<<c.y<<':'<<unsigned(c.health);
    result.history=trace.str();return result;
}
int main(int argc,char** argv) {
    try {
        // Fast CI smoke test; explicit larger study is a separate host run.
        const unsigned count=argc>1?unsigned(std::stoul(argv[1])):16u;
        const unsigned ticks=argc>2?unsigned(std::stoul(argv[2])):3600u;
        check(count>=2&&count<=10000&&ticks>=600&&ticks<=432000,"study bounds: 2..10000 seeds, 600..432000 ticks");
        std::set<std::string> orders,layouts,histories;
        unsigned no_discovery=0,no_build=0,no_birth=0,all_discovered=0;
        std::uint64_t first_min=0,first_max=0;
        std::cout<<"seed,ticks,discovery_order,discoveries,buildings,live_births,living,deaths,first_birth_tick,last_discovery_tick,layout\n";
        for(unsigned seed=0;seed<count;++seed) {
            const auto s=simulate(seed,ticks);
            orders.insert(s.order);layouts.insert(s.layout);histories.insert(s.history);
            no_discovery+=s.discoveries==0;no_build+=s.buildings==0;no_birth+=s.births==0;
            all_discovered+=s.discoveries==discovery::tech_count;
            if(s.first_birth) {
                if(!first_min||s.first_birth<first_min) first_min=s.first_birth;
                if(s.first_birth>first_max) first_max=s.first_birth;
            }
            std::cout<<seed<<','<<ticks<<','<<s.order<<','<<s.discoveries<<','<<s.buildings<<','<<s.births<<','<<s.living<<','<<s.deaths<<','<<s.first_birth<<','<<s.last_discovery<<','<<s.layout<<'\n';
            if(seed==0) check(s.history==simulate(seed,ticks).history,"same-seed observed history mismatch");
        }
        std::cout<<"DIVERSITY seeds="<<count<<" orders="<<orders.size()<<" layouts="<<layouts.size()<<" distinct_histories="<<histories.size()
                 <<" no_discovery="<<no_discovery<<" no_build="<<no_build<<" no_birth="<<no_birth
                 <<" all_discovered="<<all_discovered<<" first_birth_min="<<first_min<<" first_birth_max="<<first_max<<'\n';
        check(histories.size()>1,"identical observed histories");
        if(count>=16&&ticks>=3600) check(orders.size()>1&&layouts.size()>1,"insufficient early outcome variety");
        return 0;
    } catch(const std::exception& e){std::cerr<<"FAIL diversity: "<<e.what()<<'\n';return 1;}
}
