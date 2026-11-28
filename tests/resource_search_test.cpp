#include "survival.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
int main() {
    try {
        World w{};
        initialize(w,42);
        for(auto& t:w.tiles) t.water=0;
        auto& f=w.founders[0];
        f.x=0; f.y=0; f.thirst=100;
        w.tiles[3].water=2;
        check(find_resource(w,0,0,true).x==3,"find beyond empty neighbors");
        for(int i=1;i<=3;++i) {
            check(step_survival(w),"advance");
            check(f.x==i && f.y==0 && f.action==Action::move,"cross empty tiles");
        }
        check(step_survival(w),"drink");
        check(f.action==Action::drink && w.tiles[3].water==1,"consume on arrival");
        for(auto& t:w.tiles) t.water=0;
        w.tiles[9].water=1;
        check(!find_resource(w,0,0,true).found,"radius bounded");
        w.tiles[8].water=1;
        check(find_resource(w,0,0,true).x==8,"radius inclusive");
        w.tiles[2*width+2].water=2; w.tiles[4].water=3;
        auto t=find_resource(w,0,0,true);
        check(t.found && t.x==4 && t.y==0,"stable nearest tie");
        check(!find_resource(w,-1,0,true).found,"invalid input");
        check(!find_resource(w,width,0,true).found,"invalid input");
        check(find_resource(w,0,0,false).found,"food uses distinct stock");
        std::cout<<"PASS: bounded resource search, depleted traversal, arrival, tie order, edges\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1;
    }
}
