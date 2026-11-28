#include "../benchmarks/elecrow/observer_settings.hpp"
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){
    try{
        using namespace genesis::observer;
        DisplaySettings s;
        check(s.ticks_per_second()==12&&!s.overview,"initial display");
        check(!s.touch(48,376),"closed panel ignores touches");
        s.overview=true;
        check(!s.touch(48,264)&&!s.take_reset(),"world tab cannot erase");
        check(s.touch(166,376)&&s.page==StatsPage::people,"people tab");
        check(s.touch(284,376)&&s.page==StatsPage::deaths,"deaths tab");
        s.sync_deaths(3);
        check(s.touch(600,264)&&s.death_page==1,"older records");
        check(s.touch(600,264)&&s.death_page==2,"last page");
        check(s.touch(600,264)&&s.death_page==2,"bounded older navigation");
        check(s.touch(420,264)&&s.death_page==1,"newer records");
        s.sync_deaths(1);check(!s.death_page,"clamp shrinking record count");
        check(s.touch(402,376)&&s.page==StatsPage::progress,"progress tab");
        check(s.touch(520,376)&&s.page==StatsPage::settings,"settings tab");
        check(s.touch(48,198)&&s.brightness==64,"dim");
        check(!s.touch(216,198)&&!s.touch(48,242),"brightness bounds");
        check(s.touch(232,198)&&s.brightness==128,"normal");
        check(s.touch(583,241)&&s.brightness==255,"bright");
        check(s.touch(400,44)&&s.ticks_per_second()==3,"slower");
        check(s.touch(400,44)&&s.ticks_per_second()==1,"minimum");
        check(s.touch(400,44)&&s.ticks_per_second()==1,"minimum clamp");
        for(int i=0;i<6;++i)s.touch(700,44);
        check(s.ticks_per_second()==48,"maximum clamp");
        check(s.touch(48,264)&&s.reset_stage==1&&!s.take_reset(),"reset request");
        check(s.touch(450,376)&&s.reset_stage==2&&!s.take_reset(),"first confirmation");
        check(s.touch(450,376)&&s.reset_stage==0&&!s.take_reset(),"repeat tap cancels");
        check(s.touch(48,264)&&s.touch(450,376)&&s.touch(48,376),"two confirmations");
        check(s.take_reset()&&!s.take_reset(),"consume once");
        check(s.touch(751,419)&&!s.overview&&s.page==StatsPage::world,"close resets view");
        TickPacer p;p.arm(0,12);
        check(!p.due(83332,12)&&p.due(83333,12),"pace boundary");
        check(!p.due(100000,3)&&!p.due(433332,3)&&p.due(433333,3),"rate change");
        check(p.due(9000000,3)&&!p.due(9000000,3),"stall bounded");
        std::cout<<"PASS stats navigation, reset intent and pace boundaries\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
