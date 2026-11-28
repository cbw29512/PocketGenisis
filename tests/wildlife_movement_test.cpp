#include "../core/wildlife.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
using namespace genesis::wildlife;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
    try {
        auto w=std::unique_ptr<World>(new World);
        auto s=std::unique_ptr<Wildlife>(new Wildlife);
        genesis::initialize(*w,42);
        s->tiles.fill(WildlifeTile{});s->developed.fill(0);
        w->tiles.fill(Tile{});
        // Tick 120 selects even checkerboard sources.
        w->tick=120;
        s->tiles[0].count[deer]=4;
        w->tiles[1].food=20;w->tiles[1].water=20;
        w->tiles[2].food=100;w->tiles[2].water=100;
        Totals out{};
        migrate(*w,*s,out);
        check(s->tiles[0].count[deer]==0&&s->tiles[1].count[deer]==4&&s->tiles[2].count[deer]==0,"one-hop movement");
        check(population(*s)==4&&out.migrated==4,"movement conservation");
        // No entry to developed land even with more food.
        s->developed[2]=1;w->tick=180;
        migrate(*w,*s,out);
        check(s->tiles[2].count[deer]==0,"city exclusion");
        // On conversion, animals leave to adjacent wilderness before feeding.
        s->developed[1]=1;w->tiles[0].food=50;w->tiles[0].water=50;
        advance(*w,*s,out);
        check(s->tiles[1].count[deer]==0&&s->tiles[0].count[deer]==4,"city evacuation");
        // Predators follow actual prey.
        s->tiles.fill(WildlifeTile{});s->developed.fill(0);
        w->tiles.fill(Tile{});w->tick=120;
        s->tiles[0].count[wolf]=2;s->tiles[1].count[rabbit]=4;
        w->tiles[1].water=10;
        migrate(*w,*s,out);
        check(s->tiles[1].count[wolf]==2,"predator follows prey");
        // A blocked city tile cannot silently retain or breed wildlife.
        s->tiles.fill(WildlifeTile{});s->developed.fill(1);
        s->tiles[0].count[deer]=3;
        const auto before=out.deaths;
        advance(*w,*s,out);
        check(population(*s)==0&&out.deaths==before+3&&out.displaced_deaths==3,"stranded wildlife accounting");
        Citizen c{};c.id=1;c.hunger=30;
        s->tiles[0].count[deer]=1;
        hunt(c,s->tiles[0],w->tick,out,true);
        check(c.hunger==30&&s->tiles[0].count[deer]==1,"no city wild hunt");
        std::cout<<"PASS: bounded migration and developed land exclusion\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
