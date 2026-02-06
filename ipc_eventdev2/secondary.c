#include <rte_reorder.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_eventdev.h>

#define EVDEV_ID 0
#define QUEUE_ID 0
#define PORT_ID  0

int main(int argc, char **argv)
{
    struct rte_reorder_buffer *x;
    
    if (rte_eal_init(argc, argv) < 0)
        rte_panic("EAL init failed\n");

    printf("Secondary: waiting for event...\n");

    struct rte_event ev;

    while (rte_event_dequeue_burst(EVDEV_ID, PORT_ID, &ev, 1, 0) == 0)
        usleep(1000);

    printf("Secondary: received value = %lu\n", ev.u64);

    /* Modify and send back */
    ev.u64 += 1;
    ev.op = RTE_EVENT_OP_FORWARD;

    rte_event_enqueue_burst(EVDEV_ID, PORT_ID, &ev, 1);

    printf("Secondary: reply sent\n");

    return 0;
}
