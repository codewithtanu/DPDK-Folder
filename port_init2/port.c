#include <stdio.h>
#include <inttypes.h>

#include <rte_ethdev.h>
#include <rte_ether.h>

#include "port.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

int
port_init(uint16_t port_id, struct rte_mempool *mbuf_pool)
{
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_queues = 1;
    const uint16_t tx_queues = 1;
    int ret;

    if (!rte_eth_dev_is_valid_port(port_id))
        return -1;

    /* Configure port */
    ret = rte_eth_dev_configure(port_id, rx_queues, tx_queues, &port_conf);
    if (ret < 0)
        return ret;

    /* RX queue setup */
    ret = rte_eth_rx_queue_setup(
        port_id,
        0,
        RX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        NULL,
        mbuf_pool
    );
    if (ret < 0)
        return ret;

    /* TX queue setup (required even if unused) */
    ret = rte_eth_tx_queue_setup(
        port_id,
        0,
        TX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        NULL
    );
    if (ret < 0)
        return ret;

    /* Start port */
    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        return ret;

    /* Promiscuous mode (important for TAP) */
    rte_eth_promiscuous_enable(port_id);

    /* Print MAC */
    struct rte_ether_addr addr;
    rte_eth_macaddr_get(port_id, &addr);

    printf("Port %u MAC: %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
           ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 "\n",
           port_id,
           RTE_ETHER_ADDR_BYTES(&addr));

    return 0;
}

