#include <myria/types.h>
#include <myria/kapi.h>

static volatile u64 system_tick_count = 0;

void timer_tick_handler(void) {
    system_tick_count++;
    
    // Temporarily disable scheduler tick to isolate memory management issues
    // sched_tick();
    
    // TODO: Add timeout handling and other timer services
}

u64 get_system_ticks(void) {
    return system_tick_count;
}

// Convert ticks to milliseconds (1 tick = 1ms)
u64 get_system_time_ms(void) {
    return system_tick_count;
}