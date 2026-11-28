#pragma once
#include <cstdint>
namespace genesis::observer {
// Presentation-only state, never serialized into civilization checkpoints.
struct DisplaySettings {
    std::uint8_t brightness{128};
    bool overview{};
    bool touch(int x,int y) noexcept {
        if(!overview)return false;
        if(x>=48&&x<216&&y>=376&&y<420)brightness=64;
        else if(x>=232&&x<400&&y>=376&&y<420)brightness=128;
        else if(x>=416&&x<584&&y>=376&&y<420)brightness=255;
        else if(x>=600&&x<752&&y>=376&&y<420)overview=false;
        else return false;
        return true;
    }
};
struct SaveStatus {
    std::uint64_t generation{},tick{},card_bytes{};
    int slot{-1};
};
}
