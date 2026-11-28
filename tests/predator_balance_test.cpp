#include "../core/wildlife.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace wild=genesis::wildlife;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        check(wild::valid_cadence(wild::predator_candidate)&&
              !wild::valid_cadence({600,3600,3000,true}),"predator cadence guards");
        wild::WildlifeTile t{};Tile stock{};wild::Totals totals{};
        stock.food=stock.water=100;t.count[wild::deer]=8;t.count[wild::wolf]=2;
        wild::pulse_species(t,stock,true,false,totals);
        check(t.count[wild::deer]==8&&t.count[wild::wolf]==2&&
              totals.births==2&&totals.predation==2,"prey reproduces but predators wait");
        stock.food=stock.water=100;
        wild::pulse_species(t,stock,true,true,totals);
        check(t.count[wild::wolf]==3&&totals.predation==5&&totals.births==5,"predator birth uses real prey");
        check(10+totals.births==t.count[wild::deer]+t.count[wild::wolf]+totals.deaths,"population conservation");
        auto w=std::make_unique<World>();auto animals=std::make_unique<wild::Wildlife>();
        initialize(*w,42);w->tiles.fill(Tile{});w->tick=120;
        animals->tiles[0].count[wild::rabbit]=4;
        animals->tiles[0].count[wild::wolf]=2;
        w->tiles[0].food=w->tiles[0].water=20;
        w->tiles[1].food=w->tiles[1].water=19;
        check(wild::support(*w,*animals,0,wild::rabbit,true,false)==20,"legacy food score");
        check(wild::support(*w,*animals,0,wild::rabbit,true,true)==16,"local predator risk");
        wild::Totals movement{};
        wild::migrate(*w,*animals,movement,true);
        check(animals->tiles[0].count[wild::rabbit]==0&&animals->tiles[1].count[wild::rabbit]==4,"prey seeks safer viable tile");
        check(wild::population(*animals)==6&&wild::valid(*animals),"movement conservation");
        // Even when no safe alternative exists, no forced disappearance.
        animals->tiles.fill(wild::WildlifeTile{});w->tiles.fill(Tile{});
        animals->tiles[0].count[wild::deer]=2;animals->tiles[0].count[wild::bear]=1;
        w->tiles[0].food=w->tiles[0].water=30;
        wild::migrate(*w,*animals,movement,true);
        check(animals->tiles[0].count[wild::deer]==2,"no unsafe resource-free flight");
        std::cout<<"PASS predator breeding intervals, resource conservation and local avoidance\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
