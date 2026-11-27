#include "reference_spatial_phases.hpp"
#include "../benchmarks/elecrow/fixture.hpp"
#include "../storage/checkpoint.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
    try {
        World a{},b{};
        for(unsigned seed=0;seed<24;++seed) {
            check(benchmark::prepare(a,240,seed%2),"fixture");
            a.tick+=600;
            a.streams[disease_events].state=seed;
            a.streams[reproduction].state=seed;
            for(std::size_t i=0;i<a.descendant_count;++i) {
                auto& c=a.descendants[i];
                if(seed%3==0) {c.x=i%3;c.y=i%4;}
                if(seed%3==1) {c.x=99-i%3;c.y=59-i%4;}
                if(i%17==0) c.diseases[0].illness=1;
                if(i%19==0) c.diseases[1].illness=4;
                if(i%23==0) {c.health=1;c.diseases[0].illness=2;}
                if(i%29==0) c.diseases[1]={0,10,50};
                if(i%31==0) {c.birth_tick=a.tick-100;c.purpose=Purpose::none;}
                if(i%37==0) c.hunger=60;
            }
            // Existing busy parents, with IDs verified adult and alive.
            a.pregnancies[0]={4,5,a.tick+200};
            a.pregnancy_count=1;
            check(valid_population(a),"valid mixed fixture");
            b=a;
            // Snapshot semantics include deaths and both strains, before mortality.
            advance_diseases(a);reference_disease(b);
            update_mortality(a);update_mortality(b);
            // Clear pregnancies whose parents died using normal lifecycle on a
            // non-conception tick, then compare the actual opportunity separately.
            ++a.tick;++b.tick;
            update_lifecycle(a);update_lifecycle(b);
            a.tick+=599;b.tick+=599;
            // Earlier gestation would now be overdue; compare on original opportunity
            // instead by using a fresh future due time for all still-busy parents.
            for(std::size_t i=0;i<a.pregnancy_count;++i) {
                a.pregnancies[i].due_tick=a.tick+200;
                b.pregnancies[i].due_tick=b.tick+200;
            }
            check(storage::encode(a,1)==storage::encode(b,1),"disease canonical parity");
            attempt_conception(a);reference_conception(b);
            check(storage::encode(a,1)==storage::encode(b,1),"conception canonical and RNG parity");
        }
        check(benchmark::prepare(a,200,false),"sparse fixture");
        for(auto& c:a.descendants) {if(c.id) {c.x=0;c.y=0;}}
        a.founders[0].x=99;a.founders[0].y=0;
        a.founders[1].x=0;a.founders[1].y=1;
        b=a;
        attempt_conception(a);reference_conception(b);
        check(storage::encode(a,1)==storage::encode(b,1),"edge and dense pair parity");
        std::cout<<"PASS: spatial phases preserve exact canonical state and RNG\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
