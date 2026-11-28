#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_system.h>
#include "../../../storage/fat_identified_slots.hpp"
#include "../../../storage/autosave_clock.hpp"
#ifdef PG_PERSISTENT_OBSERVER
#include "../persistent_view.hpp"
#include "../../../maintenance/reset_transaction.hpp"
#include "../../../maintenance/reset_files.hpp"
#endif

// Disposable world only. Dedicated directory, no formatting or deletion.
// Explicit serial N creates a test world; subsequent boots recover it.
namespace {
namespace c=genesis::storage::identified_checkpoint;
namespace w=genesis::storage::world_identity;
namespace m=genesis::storage::identity_manifest;
namespace a=genesis::storage::autosave;
constexpr char directory[]="/sd/pg-world-test-v1";
constexpr std::uint64_t cadence=60;
std::unique_ptr<genesis::integrated::Simulation> sim;
w::Identity identity{};
c::FatSlots slots(directory);
a::Clock clock_state;
#ifdef PG_PERSISTENT_OBSERVER
genesis::observer::PersistentView display;
genesis::observer::TickPacer pacer;
namespace maintenance=genesis::maintenance;
std::string identity_text(const w::Identity& id){
    const char hex[]="0123456789abcdef";std::string result;
    for(auto b:id.uuid){result+=hex[b>>4];result+=hex[b&15];}
    return result;
}
maintenance::ResetFiles reset_files{std::string(directory),[](){
    const auto record=slots.manifest();
    const auto decoded=m::decode(record.bytes);
    if(record.missing||!decoded.valid||decoded.identity.official)
        throw std::runtime_error("reset identity invalid or official");
    return identity_text(decoded.identity);
}};
bool reset_pending=false;
#endif
bool ready=false,awaiting_creation=false;
std::uint64_t next_tick_ms{};
std::uint64_t now_ms(){return static_cast<std::uint64_t>(esp_timer_get_time()/1000);}
void halt(const char* reason){
    ready=false;awaiting_creation=false;
    Serial.printf("WORLD_STORAGE_ERROR %s; simulation paused\n",reason);
#ifdef PG_PERSISTENT_OBSERVER
    display.status(reason);
#endif
}
void report(const char* label,std::uint64_t generation,int slot){
    const auto& state=sim->state();
#ifdef PG_PERSISTENT_OBSERVER
    display.save_status({generation,state.world.tick,SD.cardSize(),slot});
#endif
    Serial.printf("%s generation=%llu slot=%d tick=%llu population=%u psram_free=%u internal_free=%u\n",
        label,static_cast<unsigned long long>(generation),slot,
        static_cast<unsigned long long>(state.world.tick),
        static_cast<unsigned>(state.world.descendant_count+2),
        static_cast<unsigned>(ESP.getFreePsram()),static_cast<unsigned>(ESP.getFreeHeap()));
    Serial.print("WORLD_UUID ");
    for(auto byte:identity.uuid)Serial.printf("%02x",static_cast<unsigned>(byte));
    Serial.println();
    Serial.printf("WORLD_STACK_FREE %u\n",static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
}
// Recheck the independent identity before each transaction, rather than
// trusting an earlier mount after a card removal or replacement.
void require_identity(){
    const auto manifest=slots.manifest();
    const auto decoded=m::decode(manifest.bytes);
    if(manifest.missing||!decoded.valid||!w::equal(decoded.identity,identity))
        throw std::runtime_error("persisted world identity changed or unavailable");
}
void arm(){
    if(sim->state().cadence&&sim->state().cadence!=cadence)
        throw std::runtime_error("restored world cadence incompatible");
    const auto now=now_ms();
    if(!clock_state.arm(now,sim->state().world.tick))throw std::runtime_error("arm failed");
    next_tick_ms=now+1000;ready=true;awaiting_creation=false;
#ifdef PG_PERSISTENT_OBSERVER
    pacer.arm(esp_timer_get_time(),display.ticks_per_second());
    display.update(sim->state(),true);
#endif
}
void create_test(){
    // mkdir must succeed exclusively; an incomplete prior Genesis is not erased.
    if(::mkdir(directory,0700)!=0)throw std::runtime_error("test directory creation refused");
    w::Uuid entropy{};
    esp_fill_random(entropy.data(),entropy.size());
    identity=w::make(entropy,0,false);
    sim=std::make_unique<genesis::integrated::Simulation>();
    if(!sim->create(42))throw std::runtime_error("test world admission failed");
    slots.create_manifest(identity);
    const auto result=c::save(slots,identity,*sim,true);
    if(!result.generation)throw std::runtime_error(result.error);
    require_identity();
    report("WORLD_CREATED",result.generation,result.slot);arm();
}
#ifdef PG_PERSISTENT_OBSERVER
void empty_world(){
    sim.reset();clock_state=a::Clock{};identity={};
    ready=false;awaiting_creation=true;reset_pending=false;
    display.status("World erased. USB N creates a new test world.");
    Serial.println("WORLD_RESET_COMPLETE; type N for explicit new test world");
}
void reset_current(){
    // Runtime owns the only simulation and SD writer. Stop it before
    // accepting a confirmed request; no background autosave can race.
    require_identity();reset_files.preflight();
    ready=false;
    auto permit=maintenance::prepare_reset(identity_text(identity),true);
    const auto result=maintenance::execute_reset(reset_files,permit,permit.confirmation,true);
    if(result.status!=maintenance::ResetStatus::empty){
        reset_pending=reset_files.pending();
        throw std::runtime_error(result.error);
    }
    empty_world();
}
#endif
void boot(){
#ifdef PG_PERSISTENT_OBSERVER
    if(reset_files.pending()){
        reset_pending=true;
        display.status("Reset interrupted. USB R resumes; no new world.");
        Serial.println("WORLD_RESET_PENDING; type R to resume confirmed test reset");
        return;
    }
#endif
    struct stat st{};
    if(::stat(directory,&st)!=0){
        if(errno!=ENOENT)throw std::runtime_error("test directory stat failed");
#ifdef PG_PERSISTENT_OBSERVER
        awaiting_creation=true;
        display.status("No saved world. USB N creates a new test world.");
        Serial.println("WORLD_ABSENT; type N for explicit test world");
        return;
#else
        awaiting_creation=true;
        Serial.println("WORLD_ABSENT type N to create disposable test world");
        return;
#endif
    }
    if(!S_ISDIR(st.st_mode))throw std::runtime_error("invalid world directory");
    const auto manifest=slots.manifest();
    const auto id=m::decode(manifest.bytes);
    if(!id.valid||id.identity.official)throw std::runtime_error("missing or invalid disposable identity");
    identity=id.identity;
    auto recovery=c::load(slots,identity);
    if(!recovery.checkpoint.state.simulation)throw std::runtime_error(recovery.checkpoint.state.error);
    sim=std::move(recovery.checkpoint.state.simulation);
    report("WORLD_RESTORED",recovery.checkpoint.state.generation,recovery.checkpoint.slot);
    if(recovery.checkpoint.degraded)Serial.println("WORLD_STORAGE_DEGRADED one valid slot");
    arm();
}
}
void setup(){
    Serial.begin(115200);delay(5000);
    Serial.println("POCKET GENESIS INTEGRATED SD TEST; NO OFFICIAL WORLD");
    try{
        if(!psramFound())throw std::runtime_error("PSRAM unavailable");
        // Large checkpoint buffers and simulation allocations must use PSRAM
        // where possible. Measure actual internal heap headroom in the probe.
        heap_caps_malloc_extmem_enable(4096);
#ifdef PG_PERSISTENT_OBSERVER
        display.begin();
#endif
        SPI.begin(12,13,11,10);
        if(!SD.begin(10,SPI,4000000,"/sd",5,false)||SD.cardType()==CARD_NONE)
            throw std::runtime_error("SD mount failed");
        boot();
    }catch(const std::exception& e){halt(e.what());}
}
void loop(){
    try{
#ifdef PG_PERSISTENT_OBSERVER
        if(reset_pending&&Serial.available()){
            if(Serial.read()=='R'){
                const auto id=reset_files.reset_identity();
                const auto result=maintenance::resume_reset(reset_files,"DELETE POCKET GENESIS "+id,true);
                if(result.status!=maintenance::ResetStatus::empty)throw std::runtime_error(result.error);
                empty_world();
            }
        }
#endif
        if(awaiting_creation&&Serial.available()){
            if(Serial.read()=='N')create_test();
        }
        if(ready){
#ifdef PG_PERSISTENT_OBSERVER
            // Bounded batches keep touch and real-time autosave responsive.
            for(unsigned n=0;n<4&&pacer.due(esp_timer_get_time(),display.ticks_per_second());++n){
                if(!sim->step(cadence))throw std::runtime_error("simulation step failed");
            }
#else
            auto now=now_ms();
            if(now>=next_tick_ms){
                if(!sim->step(cadence))throw std::runtime_error("simulation step failed");
                next_tick_ms=now+1000;
            }
#endif
            const auto tick=sim->state().world.tick;
            const auto status=clock_state.poll(now_ms(),tick);
            if(status==a::Status::fault)throw std::runtime_error("autosave clock fault");
            if(status==a::Status::due){
                const auto begin=esp_timer_get_time();
                require_identity();
                Serial.printf("WORLD_SAVE_BEGIN tick=%llu\n",static_cast<unsigned long long>(tick));
                const auto result=c::save(slots,identity,*sim);
                if(result.generation)require_identity();
                if(!clock_state.complete(now_ms(),tick,result.generation!=0))
                    throw std::runtime_error(result.error.empty()?"autosave completion fault":result.error);
                report("WORLD_AUTOSAVED",result.generation,result.slot);
                Serial.printf("WORLD_SAVE_US %llu\n",static_cast<unsigned long long>(esp_timer_get_time()-begin));
            }
#ifdef PG_PERSISTENT_OBSERVER
            display.update(sim->state());
            if(display.take_reset())reset_current();
#endif
        }
    }catch(const std::exception& e){halt(e.what());}
    delay(10);
}
