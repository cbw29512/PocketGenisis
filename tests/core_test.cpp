#include "world.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace genesis;
static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
int main() {
    try {
        Rng rng{0};
        require(rng.next() == UINT64_C(0xe220a8397b1dcdaf), "RNG golden word 1");
        require(rng.next() == UINT64_C(0x6e789e6aa1b965f4), "RNG golden word 2");
        auto before = rng.state;
        std::uint32_t out = 77;
        require(!rng.bounded(0, out) && rng.state == before && out == 77, "zero bound");
        for (int i=0; i<1000; ++i) require(rng.bounded(101,out) && out<=100, "bounded draw");
        World a{}, b{}, c{};
        initialize(a, 42); initialize(b, 42); initialize(c, 43);
        bool map_diff = false, personality_diff = false;
        for (std::size_t i=0; i<a.tiles.size(); ++i) {
            auto x=a.tiles[i], y=b.tiles[i], z=c.tiles[i];
            require(x.food==y.food && x.wood==y.wood && x.water==y.water && x.stone==y.stone, "map replay");
            require(x.food>=60 && x.food<=100 && x.wood>=60 && x.wood<=100 && x.water>=60 && x.water<=100 && x.stone>=20 && x.stone<=60, "resource bounds");
            map_diff |= x.food!=z.food || x.wood!=z.wood || x.water!=z.water || x.stone!=z.stone;
        }
        for (std::size_t i=0; i<2; ++i) {
            require(a.founders[i].id==i+1 && a.founders[i].x<width && a.founders[i].y<height, "founder identity/position");
            require(a.founders[i].personality==b.founders[i].personality, "trait replay");
            for (auto t:a.founders[i].personality) require(t<=100,"trait range");
            personality_diff |= a.founders[i].personality!=c.founders[i].personality;
        }
        require(map_diff && personality_diff, "initial variation only (not civilization divergence)");
        for (std::size_t i=0; i<stream_count; ++i) require(a.streams[i].state==b.streams[i].state,"stream replay");
        auto saved=a.streams[research];
        a.streams[terrain].next();
        require(saved.state==a.streams[research].state,"stream isolation");
        for (unsigned s=0; s<4; ++s) {
            require(season(a)==s,"season boundary");
            for (std::uint64_t i=0;i<ticks_per_year/4;++i) require(advance_clock(a),"clock advance");
        }
        require(a.tick==ticks_per_year && season(a)==0,"year rollover");
        a.tick=std::numeric_limits<std::uint64_t>::max();
        require(!advance_clock(a) && a.tick==std::numeric_limits<std::uint64_t>::max(),"clock overflow");
        std::cout << "PASS: RNG, initial world replay and variation, stream isolation, founder bounds, clock and seasons\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
