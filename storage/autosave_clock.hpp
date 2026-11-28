#pragma once
#include <cstdint>
namespace genesis::storage::autosave {
// Monotonic real time, not simulation ticks. Boot must recover a valid
// checkpoint before arming. The owner pauses simulation while faulted.
enum class Status : std::uint8_t { disarmed, waiting, due, fault };
class Clock {
    std::uint64_t last_ms_{},last_tick_{};
    Status status_{Status::disarmed};
public:
    static constexpr std::uint64_t interval_ms=60000;
    bool arm(std::uint64_t now,std::uint64_t durable_tick) noexcept {
        if(status_!=Status::disarmed) return false;
        last_ms_=now;last_tick_=durable_tick;status_=Status::waiting;return true;
    }
    Status poll(std::uint64_t now,std::uint64_t tick) noexcept {
        if(status_==Status::disarmed||status_==Status::fault) return status_;
        if(now<last_ms_||tick<last_tick_){status_=Status::fault;return status_;}
        if(tick>last_tick_&&now-last_ms_>=interval_ms) status_=Status::due;
        return status_;
    }
    // Acknowledge only a verified successful write. Failed saves latch a
    // fault instead of silently advancing the durability watermark.
    bool complete(std::uint64_t now,std::uint64_t tick,bool verified) noexcept {
        if(status_!=Status::due) return false;
        if(!verified||now<last_ms_||tick<=last_tick_){status_=Status::fault;return false;}
        last_ms_=now;last_tick_=tick;status_=Status::waiting;return true;
    }
    std::uint64_t durable_tick() const noexcept {return last_tick_;}
    Status status() const noexcept {return status_;}
};
}
