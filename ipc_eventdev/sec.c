#include <rte_eal.h>
#include <rte_eventdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdio.h>
#include <unistd.h>

#define EVENT_DEV_ID 0

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    printf("Secondary started\n");

    /* Wait until primary creates eventdev */
    while (rte_event_dev_count() == 0)
        sleep(1);

    struct rte_mempool *mp = rte_mempool_lookup("MBUF_POOL");
    if (!mp)
        rte_exit(EXIT_FAILURE, "Cannot find mempool\n");
    
        printf("MEMPOOL Created\n");
    while (1)
    {
        /* ---------- Receive request ---------- */
        struct rte_event ev;
        uint16_t nb = rte_event_dequeue_burst(EVENT_DEV_ID, 1, &ev, 1, 0);

        if (nb > 0)
        {
            printf("Secondary received request\n");

            /* ---------- Send reply ---------- */
            struct rte_event rev = {0};
            rev.queue_id = 1;
            rev.op = RTE_EVENT_OP_NEW;
            rev.event_type = RTE_EVENT_TYPE_CPU;
            rev.sched_type = RTE_SCHED_TYPE_ATOMIC;
            rev.mbuf = ev.mbuf;

            rte_event_enqueue_burst(EVENT_DEV_ID, 1, &rev, 1);
        }
    }
}
