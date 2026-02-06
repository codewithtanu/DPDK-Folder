#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_eventdev.h>

#define EVDEV_ID 0
#define QUEUE_ID 0
#define PORT_ID  0

static int setup_eventdev(void)
{
    struct rte_event_dev_config dev_conf = {0};

    dev_conf.nb_event_queues = 1;
    dev_conf.nb_event_ports  = 1;
    dev_conf.nb_event_queue_flows = 1024;
    dev_conf.nb_event_port_dequeue_depth = 32;
    dev_conf.nb_event_port_enqueue_depth = 32;

    if (rte_event_dev_configure(EVDEV_ID, &dev_conf) < 0)
        return -1;

    struct rte_event_queue_conf qconf = {0};
    qconf.nb_atomic_flows = 1024;
    qconf.nb_atomic_order_sequences = 1024;
    qconf.schedule_type = RTE_SCHED_TYPE_ATOMIC;
    qconf.priority = RTE_EVENT_DEV_PRIORITY_NORMAL;

    if (rte_event_queue_setup(EVDEV_ID, QUEUE_ID, &qconf) < 0)
        return -1;

    struct rte_event_port_conf pconf = {0};
    pconf.dequeue_depth = 32;
    pconf.enqueue_depth = 32;
    pconf.new_event_threshold = 1024;

    if (rte_event_port_setup(EVDEV_ID, PORT_ID, &pconf) < 0)
        return -1;

    if (rte_event_port_link(EVDEV_ID, PORT_ID, NULL, NULL, 1) < 0)
        return -1;

    if (rte_event_dev_start(EVDEV_ID) < 0)
        return -1;

    return 0;
}

int main(int argc, char **argv)
{
    if (rte_eal_init(argc, argv) < 0)
        rte_panic("EAL init failed\n");

    if (setup_eventdev() < 0)
        rte_panic("Eventdev setup failed\n");

    printf("Primary: sending event...\n");

    struct rte_event ev = {0};
    ev.queue_id = QUEUE_ID;
    ev.op = RTE_EVENT_OP_NEW;
    ev.event_type = RTE_EVENT_TYPE_CPU;
    ev.u64 = 100;

    rte_event_enqueue_burst(EVDEV_ID, PORT_ID, &ev, 1);

    struct rte_event rev;

    printf("Primary: waiting for reply...\n");

    while (rte_event_dequeue_burst(EVDEV_ID, PORT_ID, &rev, 1, 0) == 0)
        usleep(1000);

    printf("Primary: reply received = %lu\n", rev.u64);

    return 0;
}
