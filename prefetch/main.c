// #include <stdio.h>
// #include <stdint.h>
// #include <inttypes.h>

// #include <rte_eal.h>
// #include <rte_ethdev.h>
// #include <rte_mbuf.h>
// #include <rte_prefetch.h>

// #define NUM_MBUFS       8192
// #define MBUF_CACHE_SIZE 250
// #define BURST_SIZE      32
// #define PREFETCH_OFFSET 4

// static struct rte_mempool *mbuf_pool;

// /* Simple packet processing function */
// static inline void
// process_packet(struct rte_mbuf *m)
// {
//     uint8_t *data = rte_pktmbuf_mtod(m, uint8_t *);
    
//     /* Example: read Ethernet type field */
//     volatile uint16_t eth_type = *(uint16_t *)(data + 12);
//     (void)eth_type;
// }

// /* Port initialization */
// static int
// port_init(uint16_t port, struct rte_mempool *mp)
// {
//     struct rte_eth_conf port_conf = {0};
//     const uint16_t rx_rings = 1, tx_rings = 1;
//     int retval;

//     if (!rte_eth_dev_is_valid_port(port))
//         return -1;

//     retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
//     if (retval < 0)
//         return retval;

//     retval = rte_eth_rx_queue_setup(port, 0, 1024,
//                 rte_eth_dev_socket_id(port),
//                 NULL, mp);
//     if (retval < 0)
//         return retval;

//     retval = rte_eth_tx_queue_setup(port, 0, 1024,
//                 rte_eth_dev_socket_id(port),
//                 NULL);
//     if (retval < 0)
//         return retval;

//     retval = rte_eth_dev_start(port);
//     if (retval < 0)
//         return retval;

//     rte_eth_promiscuous_enable(port);

//     return 0;
// }

// /* Main RX loop with prefetch */
// static void
// lcore_main(uint16_t port)
// {
//     struct rte_mbuf *bufs[BURST_SIZE];

//     printf("Starting RX loop with prefetch...\n");

//     while (1) {

//         uint16_t nb_rx = rte_eth_rx_burst(port, 0,
//                                           bufs, BURST_SIZE);

//         if (nb_rx == 0)
//             continue;

//         uint16_t i;

//         printf("Received Packets:%d\n",nb_rx);

//         /* Prefetch first few packets */
//         for (i = 0; i < PREFETCH_OFFSET && i < nb_rx; i++) {
//             rte_prefetch0(rte_pktmbuf_mtod(bufs[i], void *));
//         }

//         /* Main loop */
//         for (i = 0; i < nb_rx - PREFETCH_OFFSET; i++) {

//             rte_prefetch0(
//                 rte_pktmbuf_mtod(bufs[i + PREFETCH_OFFSET], void *)
//             );

//             process_packet(bufs[i]);
//         }

//         /* Remaining packets */
//         for (; i < nb_rx; i++) {
//             process_packet(bufs[i]);
//         }

//         /* Free packets */
//         for (i = 0; i < nb_rx; i++) {
//             rte_pktmbuf_free(bufs[i]);
//         }
//     }
// }

// int
// main(int argc, char *argv[])
// {
//     int ret;
//     uint16_t portid;

//     /* Initialize EAL */
//     ret = rte_eal_init(argc, argv);
//     if (ret < 0)
//         rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

//     argc -= ret;
//     argv += ret;

//     if (rte_eth_dev_count_avail() == 0)
//         rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

//     portid = 0;

//     /* Create mbuf pool */
//     mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
//                     NUM_MBUFS * rte_eth_dev_count_avail(),
//                     MBUF_CACHE_SIZE, 0,
//                     RTE_MBUF_DEFAULT_BUF_SIZE,
//                     rte_socket_id());

//     if (mbuf_pool == NULL)
//         rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

//     /* Initialize port */
//     if (port_init(portid, mbuf_pool) != 0)
//         rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", portid);

//     printf("Port %u initialized\n", portid);

//     /* Run main loop */
//     lcore_main(portid);

//     return 0;
// }

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_prefetch.h>
#include <rte_cycles.h>

#define NUM_MBUFS       8192
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE      32
#define PREFETCH_OFFSET 4

static struct rte_mempool *mbuf_pool;

/* Simple packet processing function */
static inline void
process_packet(struct rte_mbuf *m)
{
    uint8_t *data = rte_pktmbuf_mtod(m, uint8_t *);
    volatile uint16_t eth_type = *(uint16_t *)(data + 12);
    (void)eth_type;
}

/* Port initialization */
static int
port_init(uint16_t port, struct rte_mempool *mp)
{
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_rings = 1, tx_rings = 1;
    int retval;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval < 0)
        return retval;

    retval = rte_eth_rx_queue_setup(port, 0, 1024,
                rte_eth_dev_socket_id(port),
                NULL, mp);
    if (retval < 0)
        return retval;

    retval = rte_eth_tx_queue_setup(port, 0, 1024,
                rte_eth_dev_socket_id(port),
                NULL);
    if (retval < 0)
        return retval;

    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    rte_eth_promiscuous_enable(port);
    return 0;
}

/* RX Loop with Prefetch + PPS Counter */
static void
lcore_main(uint16_t port)
{
    struct rte_mbuf *bufs[BURST_SIZE];

    /* PPS measurement variables */
    uint64_t total = 0;
    uint64_t last = 0;
    uint64_t start = rte_get_timer_cycles();
    uint64_t hz = rte_get_timer_hz();

    printf("Starting RX loop with prefetch...\n");

    while (1) {

        uint16_t nb_rx = rte_eth_rx_burst(port, 0,
                                          bufs, BURST_SIZE);

        if (nb_rx == 0)
            continue;

        total += nb_rx;

        uint16_t i;

        /* Initial prefetch */
        for (i = 0; i < PREFETCH_OFFSET && i < nb_rx; i++) {
            rte_prefetch0(rte_pktmbuf_mtod(bufs[i], void *));
        }

        /* Main processing loop */
        for (i = 0; i < nb_rx - PREFETCH_OFFSET; i++) {

            rte_prefetch0(
                rte_pktmbuf_mtod(bufs[i + PREFETCH_OFFSET], void *)
            );

            process_packet(bufs[i]);
        }

        /* Remaining packets */
        for (; i < nb_rx; i++) {
            process_packet(bufs[i]);
        }

        /* Free packets */
        for (i = 0; i < nb_rx; i++) {
            rte_pktmbuf_free(bufs[i]);
        }

        /* Print PPS every 1 second */
        uint64_t now = rte_get_timer_cycles();

        if ((now - start) > hz) {
            printf("Packets per second: %" PRIu64 "\n", total - last);
            last = total;
            start = now;
        }
    }
}

int
main(int argc, char *argv[])
{
    int ret;
    uint16_t portid;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    if (rte_eth_dev_count_avail() == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports found\n");

    portid = 0;

    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                    NUM_MBUFS,
                    MBUF_CACHE_SIZE, 0,
                    RTE_MBUF_DEFAULT_BUF_SIZE,
                    rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    if (port_init(portid, mbuf_pool) != 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize port\n");

    printf("Port %u initialized\n", portid);

    lcore_main(portid);

    return 0;
}