#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_timer.h>
#include <rte_cycles.h>

/* Timer object */
static struct rte_timer my_timer;

/* Callback function */
void timer_callback(struct rte_timer *tim, void *arg)
{
    static int count = 0;
    printf("🔥 Timer triggered! Count = %d on lcore %u\n",
           count++, rte_lcore_id());
}

/* Main function */
int main(int argc, char **argv)
{
    int ret;

    /* Step 1: Initialize DPDK Environment */
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("Error initializing EAL\n");
        return -1;
    }

    printf("✅ DPDK EAL Initialized\n");

    /* Step 2: Initialize Timer Subsystem */
    rte_timer_subsystem_init();

    /* Step 3: Initialize Timer */
    rte_timer_init(&my_timer);

    /* Step 4: Get Timer Frequency */
    uint64_t hz = rte_get_timer_hz();
    printf("Timer frequency = %lu cycles per second\n", hz);

    /* Step 5: Set Timer (2 seconds periodic) */
    rte_timer_reset(
        &my_timer,
        2 * hz,                      // 2 seconds
        PERIODICAL,        // periodic timer
        rte_lcore_id(),              // current core
        timer_callback,              // callback function
        NULL                         // argument
    );

    printf("⏳ Timer started (every 2 seconds)\n");

    /* Step 6: Main Loop */
    while (1) {
        /* Simulate packet processing */
        printf("Processing packets...\n");

        /* IMPORTANT: Manage timers */
        rte_timer_manage();

        // sleep(1);  // just to slow output (NOT used in real DPDK apps)
    }

    return 0;
}