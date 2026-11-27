#include "../core/wildlife.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
using namespace genesis::wildlife;
static void require(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
    try {
        WildlifeTile t{};Tile stock{};Totals out{};
        t.count[deer]=4;t.count[wolf]=1;
        stock.food=4;stock.water=5;
        pulse(t,stock,false,out);
        require(t.count[deer]==3&&t.count[wolf]==1&&out.predation==1&&out.deaths==1,"predation accounting");
        require(stock.food==0&&stock.water==0,"shared resource consumption");
        pulse(t,stock,true,out);
        require(t.count[deer]==0&&t.count[wolf]==0,"starvation");
        stock.food=100;stock.water=100;pulse(t,stock,true,out);
        require(t.count[deer]==0&&t.count[wolf]==0,"extinction reseeded");
        t.count[rabbit]=4;pulse(t,stock,true,out);
        require(t.count[rabbit]==5&&out.births==1,"fed reproduction");
        Citizen c{};c.id=1;c.hunger=30;
        hunt(c,t,0,out);
        require(t.count[rabbit]==4&&t.meat==1&&c.hunger==18&&out.hunted==1&&out.meals==1,"hunt transaction");
        hunt(c,t,0,out);
        require(t.meat==1,"double action");
        WildlifeTile empty{};c.hunger=30;c.action=Action::idle;hunt(c,empty,0,out);
        require(c.hunger==30&&empty.meat==0,"free food");
        auto a=std::unique_ptr<Wildlife>(new Wildlife),b=std::unique_ptr<Wildlife>(new Wildlife);
        auto w1=std::unique_ptr<World>(new World),w2=std::unique_ptr<World>(new World);
        genesis::initialize(*w1,42);genesis::initialize(*w2,42);
        wildlife::initialize(*a,42);wildlife::initialize(*b,42);
        require(population(*a)>0&&valid(*a),"invalid initial population");
        const auto start=population(*a);Totals x{},y{};
        for(unsigned i=1;i<=600;++i) {w1->tick=i;w2->tick=i;advance(*w1,*a,x);advance(*w2,*b,y);}
        require(valid(*a)&&valid(*b),"invalid final state");
        require(start+x.births==population(*a)+x.deaths,"population conservation");
        for(std::size_t i=0;i<a->tiles.size();++i) {
            require(a->tiles[i].count==b->tiles[i].count&&a->tiles[i].meat==b->tiles[i].meat,"non-deterministic animals");
            require(w1->tiles[i].food==w2->tiles[i].food&&w1->tiles[i].water==w2->tiles[i].water,"non-deterministic resources");
        }
        std::cout<<"PASS: wildlife transactions, extinction and deterministic replay\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
