#pragma once
#include "../../core/ecology.hpp"

namespace genesis::benchmark {
// Synthetic, disposable adult citizens; no real world is loaded or persisted.
inline bool prepare(World& w, std::size_t total, bool contact) noexcept {
    if(total<2 || total>live_capacity) return false;
    initialize(w,42);
    w.tick=20*ticks_per_year;
    w.descendant_count=total-2;
    w.next_citizen_id=total+1;
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        c.id=i+3;
        c.parent1=1;
        c.parent2=2;
        c.birth_tick=1;
        // Preserve the original <=2048 fixture; larger populations need
        // a wider grid to stay inside the 100x60 world.
        const auto columns=total>2048?90u:80u;
        const auto margin=total>2048?5u:10u;
        c.x=static_cast<std::uint16_t>(contact?50:margin+i%columns);
        c.y=static_cast<std::uint16_t>(contact?30:margin+i/columns);
        c.personality.fill(50);
        c.purpose=choose_purpose(c);
    }
    if(contact) {
        w.founders[0].diseases[0].illness=6;
        w.founders[0].diseases[0].resistance=0;
    }
    return valid_population(w);
}
}
