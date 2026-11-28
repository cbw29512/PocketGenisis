#include "../benchmarks/elecrow/observer_settings.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace genesis::observer;
    DisplaySettings s;
    assert(s.ticks_per_second()==12&&!s.overview);
    assert(!s.touch(48,376));
    s.overview=true;
    assert(s.touch(48,376)&&s.brightness==64);
    assert(!s.touch(216,376)&&!s.touch(48,420));
    assert(s.touch(232,376)&&s.brightness==128);
    assert(s.touch(583,419)&&s.brightness==255);
    assert(s.touch(400,44)&&s.ticks_per_second()==3);
    assert(s.touch(400,44)&&s.ticks_per_second()==1);
    assert(s.touch(400,44)&&s.ticks_per_second()==1);
    for(int i=0;i<6;++i)s.touch(700,44);
    assert(s.ticks_per_second()==48);
    assert(s.touch(48,264)&&s.reset_stage==1&&!s.take_reset());
    assert(s.touch(450,376)&&s.reset_stage==2&&!s.take_reset());
    assert(s.touch(450,376)&&s.reset_stage==0&&!s.take_reset());
    assert(s.touch(48,264)&&s.touch(450,376)&&s.touch(48,376));
    assert(s.take_reset()&&!s.take_reset());
    assert(s.touch(751,419)&&!s.overview);
    TickPacer p;p.arm(0,12);assert(!p.due(83332,12));assert(p.due(83333,12));
    assert(!p.due(100000,3));assert(!p.due(433332,3));assert(p.due(433333,3));
    assert(p.due(9000000,3));assert(!p.due(9000000,3));
    std::cout<<"PASS settings, reset intent and pace boundaries\n";
}
