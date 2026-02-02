#include <rte_graph_worker_common.h>
#include <rte_mbuf_core.h>
#include <rte_trace.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_mbuf.h>

#include <rte_graph.h>
#include <rte_graph_worker.h>

#define NB_MBUFS 8192
#define MBUF_CACHE 256
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define BURST_SIZE RTE_GRAPH_BURST_SIZE

#define RX_NODE_NAME "rx_node"
#define IPV4_NODE_NAME "ipv4_node"
#define TX_NODE_NAME "tx_node"
#define DROP_NODE_NAME "drop_node"

volatile int force_quit;

static void signal_handler(int sig) {

  if (sig == SIGINT || sig == SIGTERM) {
    printf("Forcefully Signal Has Been Raised: %d\n", sig);
    force_quit = 1;
  }
}

static uint16_t rx_node_process(struct rte_graph *graph, struct rte_node *node,
                                void **obj, uint16_t nb_objs __rte_unused) {
  uint16_t nb_rx;

  struct rte_mbuf **pkt = (struct rte_mbuf **)obj;

  nb_rx = rte_eth_rx_burst(0, 0, pkt, 32);

  if (nb_rx) {

    printf("Inside the RX_NODE Process Node Got %d Packets \n", (int)nb_rx);
    rte_node_enqueue(graph, node, 0, (void **)pkt, nb_rx);
  }

  return nb_rx;
}

struct rte_node_register rx_node = {.name = RX_NODE_NAME,
                                    .process = rx_node_process,
                                    .flags = RTE_NODE_SOURCE_F,
                                    .nb_edges = 1,
                                    .next_nodes = {
                                        [0] = IPV4_NODE_NAME,
                                    }};

static void __attribute__((constructor(65535), used))
rte_node_register_rx_node(void) {
  rx_node.parent_id = RTE_NODE_ID_INVALID; /* 0xFFFFFFFF */
  rx_node.id = __rte_node_register(&rx_node);
  printf("RX Node has been Registered\n");
}

static uint16_t ipv4_node_process(struct rte_graph *graph,
                                  struct rte_node *node, void **obj,
                                  uint16_t nb_objs __rte_unused) {

  printf("Inside the IPV4 Process Node Got %d Packets\n", (int)nb_objs);

  uint16_t nb_tx = 0, nb_drop = 0;
  struct rte_mbuf *tx_pkts[RTE_GRAPH_BURST_SIZE];
  struct rte_mbuf *drop_pkts[RTE_GRAPH_BURST_SIZE];

  for (int i = 0; i < nb_objs; i++) {

    struct rte_mbuf *pkt = (struct rte_mbuf *)obj[i];
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

    if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
      tx_pkts[nb_tx++] = pkt;
    }

    else {
      drop_pkts[nb_drop++] = pkt;
    }
  }

  printf("TX Packet %d and DROP Packet %d\n",(int)nb_tx,nb_drop);

  if (nb_tx)
    rte_node_enqueue(graph, node, 0, (void **)tx_pkts, nb_tx);

  if (nb_drop)
    rte_node_enqueue(graph, node, 1, (void **)drop_pkts, nb_drop);

  return nb_objs;
}

struct rte_node_register ipv4_node = {
    .name = IPV4_NODE_NAME,
    .process = ipv4_node_process,
    .nb_edges = 2,
    .next_nodes =
        {
            [0] = TX_NODE_NAME,
            [1] = DROP_NODE_NAME,
        },
};


static void __attribute__((constructor(65535), used))
rte_node_register_ipv4_node(void) {
  ipv4_node.parent_id = RTE_NODE_ID_INVALID; /* 0xFFFFFFFF */
  ipv4_node.id = __rte_node_register(&ipv4_node);
  printf("IPV4 Node has been Registered\n");
}

static uint16_t tx_node_process(struct rte_graph *graph, struct rte_node *node,
                                void **obj, uint16_t nb_objs __rte_unused) {
  uint16_t nb_tx;
  printf("Inside the TX_NODE Process Node Got %d Packets\n", (int)nb_objs);

  struct rte_mbuf **pkt = (struct rte_mbuf **)obj;

  nb_tx = rte_eth_tx_burst(0, 0, pkt, nb_objs);
  return nb_tx;
}

struct rte_node_register tx_node = {
    .name = TX_NODE_NAME,
    .process = tx_node_process,
};

static void __attribute__((constructor(65535), used))
rte_node_register_tx_node(void) {
  tx_node.parent_id = RTE_NODE_ID_INVALID; /* 0xFFFFFFFF */
  tx_node.id = __rte_node_register(&tx_node);
  printf("TX Node has been Registered\n");
}

static uint16_t drop_node_process(struct rte_graph *graph,
                                  struct rte_node *node, void **obj,
                                  uint16_t nb_objs __rte_unused) {
  printf("Inside the DROP_NODE Process Node Got %d Packets\n", (int)nb_objs);

  uint16_t i;

  for (i = 0; i < nb_objs; i++)
    rte_pktmbuf_free((struct rte_mbuf *)obj[i]);

  return nb_objs;
}

struct rte_node_register drop_node = {
    .name = DROP_NODE_NAME,
    .process = drop_node_process,
};

static void __attribute__((constructor(65535), used))
rte_node_register_drop_node(void) {
  drop_node.parent_id = RTE_NODE_ID_INVALID; /* 0xFFFFFFFF */
  drop_node.id = __rte_node_register(&drop_node);
  printf("Drop Node has been Registered\n");
}

int main(int argc, char **argv) {
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
  mbuf_pool =
      rte_pktmbuf_pool_create("MBUF_POOL", NB_MBUFS, MBUF_CACHE, 0,
                              RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  if (!mbuf_pool)
    rte_exit(EXIT_FAILURE, "Mbuf pool creation failed\n");

  /* Configure Ethernet port 0 */
  struct rte_eth_conf port_conf = {0};

  rte_eth_dev_configure(0, 1, 1, &port_conf);

  rte_eth_rx_queue_setup(0, 0, RX_RING_SIZE, rte_eth_dev_socket_id(0), NULL,
                         mbuf_pool);

  rte_eth_tx_queue_setup(0, 0, TX_RING_SIZE, rte_eth_dev_socket_id(0), NULL);

  rte_eth_dev_start(0);

  struct rte_graph_param graph_param = {
      .nb_node_patterns = 1,
      .node_patterns = (const char *[]){RX_NODE_NAME},
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