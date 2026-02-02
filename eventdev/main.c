#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_eventdev.h>
#include <rte_lcore.h>
#include <rte_service.h>

#include <stdint.h>
#include <stdio.h>

#define NUM_EVENTS 20
#define EXIT_EVENT 9999

static uint64_t core_stats[RTE_MAX_LCORE];
static unsigned worker_ports[RTE_MAX_LCORE];
static unsigned worker_count = 0;

static int worker(void *arg)
{
    uint8_t dev_id = 0;
    unsigned lcore = rte_lcore_id();
    uint8_t port_id = worker_ports[lcore];
    struct rte_event ev;

    printf("Worker started on lcore %u using port %u\n", lcore, port_id);

    while (1) {
        if (rte_event_dequeue_burst(dev_id, port_id, &ev, 1, 0) > 0) {

            core_stats[lcore]++;

            printf("Core %u processed event %lu\n", lcore, ev.u64);

            if (lcore == 2)
                rte_delay_ms(100);

            if (ev.u64 == EXIT_EVENT)
                break;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    uint8_t dev_id = 0;

    struct rte_event_dev_config dev_conf = {
        .nb_event_queues = 1,
        .nb_event_ports = 8,   // enough ports
        .nb_events_limit = 4096,
        .nb_event_queue_flows = 1024,
        .nb_event_port_dequeue_depth = 32,
        .nb_event_port_enqueue_depth = 32,
    };

    struct rte_event_queue_conf q_conf = {
        .schedule_type = RTE_SCHED_TYPE_ATOMIC,
        .nb_atomic_flows = 1024,
        .nb_atomic_order_sequences = 1024,
    };

    struct rte_event_port_conf p_conf = {
        .new_event_threshold = 1024,
        .dequeue_depth = 32,
        .enqueue_depth = 32,
    };

    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    printf("EAL initialized\n");

    uint32_t sid;
    if (rte_service_get_by_name("event_sw0_service", &sid) < 0)
        rte_exit(EXIT_FAILURE, "event service not found\n");

    rte_service_runstate_set(sid, 1);

    if (rte_event_dev_configure(dev_id, &dev_conf) < 0)
        rte_exit(EXIT_FAILURE, "Device configure failed\n");

    if (rte_event_queue_setup(dev_id, 0, &q_conf) < 0)
        rte_exit(EXIT_FAILURE, "Queue setup failed\n");

    for (int i = 0; i < 8; i++) {
        if (rte_event_port_setup(dev_id, i, &p_conf) < 0)
            rte_exit(EXIT_FAILURE, "Port setup failed\n");
    }

    uint8_t queues[] = {0};
    uint8_t priorities[] = {RTE_EVENT_DEV_PRIORITY_NORMAL};

    for (int i = 0; i < 8; i++) {
        if (rte_event_port_link(dev_id, i, queues, priorities, 1) != 1)
            rte_exit(EXIT_FAILURE, "Port link failed\n");
    }

    if (rte_event_dev_start(dev_id) < 0)
        rte_exit(EXIT_FAILURE, "Device start failed\n");

    printf("Event device started\n");

    /* launch workers with unique ports */
    unsigned lcore_id;
    unsigned port = 0;

    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        worker_ports[lcore_id] = port++;
        rte_eal_remote_launch(worker, NULL, lcore_id);
        worker_count++;
    }

    printf("Total workers: %u\n", worker_count);

    /* normal events */
    for (int i = 0; i < NUM_EVENTS; i++) {
        struct rte_event ev = {0};

        ev.queue_id = 0;
        ev.op = RTE_EVENT_OP_NEW;
        ev.sched_type = RTE_SCHED_TYPE_ATOMIC;
        ev.u64 = i;

        while (rte_event_enqueue_burst(dev_id, 0, &ev, 1) != 1)
            ;

        printf("Producer sent event %d\n", i);
    }

    rte_delay_ms(2000);

    /* exit events */
    for (unsigned i = 0; i < worker_count; i++) {
        struct rte_event ev = {0};

        ev.queue_id = 0;
        ev.op = RTE_EVENT_OP_NEW;
        ev.sched_type = RTE_SCHED_TYPE_ATOMIC;
        ev.u64 = EXIT_EVENT;

        while (rte_event_enqueue_burst(dev_id, 0, &ev, 1) != 1)
            ;
    }

    rte_eal_mp_wait_lcore();

    printf("\n=== Per-core stats ===\n");
    for (int i = 0; i < RTE_MAX_LCORE; i++) {
        if (core_stats[i])
            printf("Core %d processed %lu events\n", i, core_stats[i]);
    }

    rte_event_dev_stop(dev_id);
    rte_event_dev_close(dev_id);

    printf("Done\n");
    return 0;
}
