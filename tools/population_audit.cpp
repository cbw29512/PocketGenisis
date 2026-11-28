#include "ecology.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace genesis;
struct Metrics {
    std::size_t peak{};
    bool reached_child{}, reached_adult{};
    unsigned min_founder_health{100};
    std::uint64_t first_birth{};
};
static Metrics simulate(World& w,unsigned years) {
    Metrics m{};Production p{};
    const auto end=std::uint64_t(years)*ticks_per_year;
    while(w.tick<end) {
        if(!step_world(w,p)) throw std::runtime_error("rejected tick="+std::to_string(w.tick));
        m.peak=std::max(m.peak,w.descendant_count);
        if(w.next_citizen_id>3&&!m.first_birth) m.first_birth=w.tick;
        m.min_founder_health=std::min({m.min_founder_health,unsigned(w.founders[0].health),unsigned(w.founders[1].health)});
        // Observe life stages periodically, without touching simulation state.
        if(w.tick%600==0) {
            for(std::size_t i=0;i<w.descendant_count;++i) {
                const auto stage=life_stage(w.descendants[i],w.tick);
                m.reached_child|=stage>=LifeStage::child;
                m.reached_adult|=stage>=LifeStage::adult;
            }
        }
    }
    if(!valid_population(w)) throw std::runtime_error("invalid final population");
    return m;
}
int main(int argc,char** argv) {
    try {
        const auto years=argc>1?std::stoul(argv[1]):5ul;
        const auto seeds=argc>2?std::stoul(argv[2]):32ul;
        if(!years||years>1000||!seeds||seeds>1024) throw std::runtime_error("years 1..1000, seeds 1..1024 required");
        unsigned births=0,extinct=0,mature=0,capacity=0;
        std::cout<<"seed,births,deaths,living,peak,age2,adult,first_birth,founder_min_health\n";
        for(unsigned seed=0;seed<seeds;++seed) {
            try {
                World w{};initialize(w,seed);
                const auto m=simulate(w,static_cast<unsigned>(years));
                births+=w.next_citizen_id>3;extinct+=w.descendant_count==0;
                mature+=m.reached_adult;capacity+=w.next_citizen_id==history_capacity+3;
                std::cout<<seed<<','<<w.next_citizen_id-3<<','<<w.archive_count<<','<<w.descendant_count<<','<<m.peak<<','
                         <<m.reached_child<<','<<m.reached_adult<<','<<m.first_birth<<','<<m.min_founder_health<<std::endl;
            } catch(const std::exception& e) {
                throw std::runtime_error("seed="+std::to_string(seed)+" "+e.what());
            }
        }
        std::cerr<<"years="<<years<<" seeds="<<seeds<<" anybirth="<<births<<" extinct="<<extinct<<" adult="<<mature
                 <<" archive_full="<<capacity<<" sizeof_world="<<sizeof(World)<<'\n';
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"AUDIT FAILED: "<<e.what()<<'\n';return 1;
    }
}
