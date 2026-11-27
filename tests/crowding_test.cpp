#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
static void setup(World& w,bool crowded) {
    initialize(w,42);w.tick=60;w.descendant_count=5;w.next_citizen_id=8;
    for(auto& f:w.founders) {f.x=50;f.y=30;f.diseases[0].resistance=0;}
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
        c.x=crowded?50:0;c.y=30;c.diseases[0].immunity=10;
    }
}
static std::uint64_t fixture(unsigned lo,unsigned hi) {
    for(std::uint64_t i=0;i<100000;++i) {
        Rng r{i};std::uint32_t roll{};r.bounded(10000,roll);
        if(roll>=lo&&roll<hi) return i;
    }
    throw std::runtime_error("missing RNG fixture");
}
int main() {
    try {
        check(crowding_odds_percent(0)==100&&crowding_odds_percent(6)==200&&crowding_odds_percent(256)==200,"bounded local pressure");
        check(crowded_exposure(2000,1)==2000&&crowded_exposure(2000,6)==3333,"odds conversion");
        check(crowded_exposure(0,256)==0&&crowded_exposure(10000,256)==10000,"probability endpoints");
        for(unsigned p=0;p<=10000;p+=100) {
            for(unsigned n=1;n<10;++n) check(crowded_exposure(p,n)>=crowded_exposure(p,n-1),"monotone pressure");
        }
        World sparse{},dense{};setup(sparse,false);setup(dense,true);
        sparse.founders[0].diseases[0].illness=6;dense.founders[0].diseases[0].illness=6;
        sparse.streams[disease_events].state=dense.streams[disease_events].state=fixture(2000,3333);
        advance_diseases(sparse);advance_diseases(dense);
        check(!sparse.founders[1].diseases[0].illness&&dense.founders[1].diseases[0].illness==6,"same infectious contact, crowding increases exposure");
        setup(dense,true);auto rng=dense.streams[disease_events].state;
        advance_diseases(dense);
        check(!is_ill(dense.founders[0])&&!is_ill(dense.founders[1])&&dense.streams[disease_events].state==rng,"crowding alone does not create disease");
        setup(dense,true);dense.founders[0].diseases[0].illness=6;dense.founders[1].diseases[0].immunity=2;
        advance_diseases(dense);check(!dense.founders[1].diseases[0].illness&&dense.founders[1].diseases[0].immunity==1,"immunity preserved in crowded area");
        setup(sparse,false);setup(dense,true);
        sparse.founders[0].diseases[0].illness=dense.founders[0].diseases[0].illness=6;
        dense.founders[0].hunger=100;
        advance_diseases(sparse);advance_diseases(dense);
        check(sparse.founders[0].health==92&&dense.founders[0].health==84,"resource stress increases actual illness damage");
        setup(dense,true);dense.founders[0].health=1;dense.founders[0].diseases[0].illness=6;
        dense.founders[0].hunger=100;advance_diseases(dense);
        check(dense.founders[0].health==1,"founder immortality unchanged");
        std::cout<<"PASS: local crowding, bounded odds, immunity, stress and founder floor\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
