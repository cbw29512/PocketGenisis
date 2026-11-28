#include "../benchmarks/elecrow/observer_settings.hpp"
#include <cassert>
#include <iostream>
int main(){using namespace genesis::observer;DisplaySettings s;assert(!s.overview&&s.brightness==128);assert(!s.touch(48,376));s.overview=true;assert(s.touch(48,376)&&s.brightness==64&&s.overview);assert(!s.touch(216,376));assert(!s.touch(48,420));assert(s.touch(232,376)&&s.brightness==128);assert(s.touch(583,419)&&s.brightness==255);assert(!s.touch(584,419));assert(s.touch(751,419)&&!s.overview);assert(!s.touch(48,376)&&s.brightness==255);SaveStatus save{9,480,62534975488ULL,0};assert(save.generation==9&&save.slot==0);std::cout<<"PASS settings touch bounds, brightness and close behavior\n";}
