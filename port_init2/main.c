#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#include "port.h"
#include "rx_loop.h"

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

int
main(int argc, char **argv)
{
    struct rte_mempool *mbuf_pool;
    uint16_t port_id;
    int ret;

    /* EAL init */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NUM_MBUFS,
        MBUF_CACHE_SIZE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Mbuf pool creation failed\n");

    printf("Mempool created (%u mbufs)\n",
           rte_mempool_avail_count(mbuf_pool));

    /* Init all ports */
    RTE_ETH_FOREACH_DEV(port_id) {
        if (port_init(port_id, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Port %"PRIu16 " init failed\n", port_id);
    }

    printf("RX-oPRINT_NEW_LINEy DPDK app started\n");

   
    rx_loop(0);
    return 0;
}

