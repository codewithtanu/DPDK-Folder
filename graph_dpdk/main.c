/*
 * DPDK 25.11 Graph example
 *
 * Graph:
 *   RX (source)
 *     |
 *   IPv4 check
 *    /   \
 *  TX   DROP
 *
 * This example demonstrates:
 *  - Graph node registration
 *  - Source node usage
 *  - Edge-based packet steering
 *  - Graph creation and execution
 */

#include <rte_graph_worker_common.h>
#include <rte_trace.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>

/* DPDK core */
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>

/* DPDK Graph */
#include <rte_graph.h>
#include <rte_graph_worker.h>
//#include <rte_node.h>

/* ---------------- Configuration ---------------- */

#define NB_MBUFS        8192
#define MBUF_CACHE     256
#define RX_RING_SIZE   1024
#define TX_RING_SIZE   1024
#define BURST_SIZE     RTE_GRAPH_BURST_SIZE

#define RX_NODE_NAME   "rx_node"
#define IPV4_NODE_NAME "ipv4_node"
#define TX_NODE_NAME   "tx_node"
#define DROP_NODE_NAME "drop_node"

static volatile int force_quit;

/* ---------------- Signal handling ---------------- */

/* Handle Ctrl+C cleanly */
static void
signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
        force_quit = 1;
}

/* ---------------- RX NODE (SOURCE NODE) ---------------- */

/*
 * RX node:
 * - Source node of the graph
 * - Pulls packets from NIC RX queue
 * - Injects packets into the graph
 */
static uint16_t
rx_node_process(struct rte_graph *graph,
                struct rte_node *node,
                void **objs,
                uint16_t nb_objs __rte_unused)
{
    struct rte_mbuf **pkts = (struct rte_mbuf **)objs;

    /* Receive packets from port 0, queue 0 */
    uint16_t nb_rx = rte_eth_rx_burst(0, 0, pkts, BURST_SIZE);

    if (nb_rx) {
        /* Send all packets to edge 0 → IPv4 node */
        rte_node_enqueue(graph, node, 0, objs, nb_rx);
    }

    return nb_rx;
}

 /* RX node registration 

        NIC
         |
         v
     RX NODE  (source)
         |
         v
     IPV4 NODE

        | Field        | Meaning                       |
        | ------------ | ----------------------------- |
        | `name`       | Node identity                 |
        | `flags`      | Is it a source or normal node |
        | `nb_edges`   | How many next paths           |
        | `next_nodes` | Names of next nodes           |



*/
static struct rte_node_register rx_node = {
    .name = RX_NODE_NAME,
    .process = rx_node_process,

    /* THIS IS MANDATORY: marks this node as a source */
    .flags = RTE_NODE_SOURCE_F,

    .nb_edges = 1,
    .next_nodes = {
        [0] = IPV4_NODE_NAME,
    }
};
RTE_NODE_REGISTER(rx_node);

/* ---------------- IPv4 NODE ---------------- */

/*
 * IPv4 node:
 * - Checks Ethernet type
 * - Chooses next edge dynamically
 *   Edge 0 → TX
 *   Edge 1 → DROP
 */

 
static uint16_t
ipv4_node_process(struct rte_graph *graph,
                  struct rte_node *node,
                  void **objs,
                  uint16_t nb_objs)
{
    uint16_t i;
    for (i = 0; i < nb_objs; i++) {
        struct rte_mbuf *pkt = (struct rte_mbuf *)objs[i];
        

        /* Get Ethernet header */
        struct rte_ether_hdr *eth =
            rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

        if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
            /* IPv4 packet → TX node */
            rte_node_enqueue_x1(graph, node, 0, pkt);
        } else {
            /* Non-IPv4 packet → DROP node */
            rte_node_enqueue_x1(graph, node, 1, pkt);
        }
    }

    return nb_objs;
}

/* IPv4 node registration */
static struct rte_node_register ipv4_node = {
    .name = IPV4_NODE_NAME,
    .process = ipv4_node_process,
    .nb_edges = 2,
    .next_nodes = {
        [0] = TX_NODE_NAME,
        [1] = DROP_NODE_NAME,
    },
};
RTE_NODE_REGISTER(ipv4_node);

/* ---------------- TX NODE ---------------- */

/*
 * TX node:
 * - Sends packets out through NIC TX queue
 */
static uint16_t
tx_node_process(struct rte_graph *graph __rte_unused,
                struct rte_node *node __rte_unused,
                void **objs,
                uint16_t nb_objs)
{
    struct rte_mbuf **pkts = (struct rte_mbuf **)objs;

    rte_eth_tx_burst(0, 0, pkts, nb_objs);
    return nb_objs;
}

/* TX node registration */
static struct rte_node_register tx_node = {
    .name = TX_NODE_NAME,
    .process = tx_node_process,
};
RTE_NODE_REGISTER(tx_node);

/* ---------------- DROP NODE ---------------- */

/*
 * DROP node:
 * - Frees packets
 */
static uint16_t
drop_node_process(struct rte_graph *graph __rte_unused,
                  struct rte_node *node __rte_unused,
                  void **objs,
                  uint16_t nb_objs)
{
    uint16_t i;

    for (i = 0; i < nb_objs; i++)
        rte_pktmbuf_free((struct rte_mbuf *)objs[i]);

    return nb_objs;
}

/* DROP node registration */
static struct rte_node_register drop_node = {
    .name = DROP_NODE_NAME,
    .process = drop_node_process,
};
RTE_NODE_REGISTER(drop_node);

/* ---------------- MAIN ---------------- */

int
main(int argc, char **argv)
{
    int ret;
    struct rte_mempool *mbuf_pool;
    struct rte_graph *graph;
    rte_graph_t graph_id;

    /* Initialize EAL */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NB_MBUFS,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Mbuf pool creation failed\n");

    /* Configure Ethernet port 0 */
    struct rte_eth_conf port_conf = {0};

    rte_eth_dev_configure(0, 1, 1, &port_conf);

    rte_eth_rx_queue_setup(0, 0, RX_RING_SIZE,
                           rte_eth_dev_socket_id(0),
                           NULL, mbuf_pool);

    rte_eth_tx_queue_setup(0, 0, TX_RING_SIZE,
                           rte_eth_dev_socket_id(0),
                           NULL);

    rte_eth_dev_start(0);

    /* Create graph */
    struct rte_graph_param graph_param = {
        .nb_node_patterns = 1,
        .node_patterns = (const char *[]) { RX_NODE_NAME },
    };

    graph_id = rte_graph_create("ipv4_graph", &graph_param);
    if (graph_id == RTE_GRAPH_ID_INVALID)
        rte_exit(EXIT_FAILURE, "Graph creation failed\n");

    graph = rte_graph_lookup("ipv4_graph");
    if (!graph)
        rte_exit(EXIT_FAILURE, "Graph lookup failed\n");

    printf("DPDK Graph running (Ctrl+C to quit)\n");

    /* Graph execution loop */
    while (!force_quit) {
        rte_graph_walk(graph);
    }

    rte_graph_destroy(graph_id);

    rte_eth_dev_stop(0);
    rte_eth_dev_close(0);

    printf("Exited cleanly\n");
    return 0;
}
