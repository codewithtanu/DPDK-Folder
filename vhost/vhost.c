#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#define NUM_MBUFS 1024
#define RX_DESC   64
#define TX_DESC   64

int main(int argc, char **argv)
{
    uint16_t port_id = 0;
    struct rte_mempool *mbuf_pool;
    struct rte_eth_conf port_conf = {0};

    /* Initialize EAL */
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create(
        "VHOST_POOL",
        NUM_MBUFS,
        0,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Configure vhost-user port */
    rte_eth_dev_configure(port_id, 1, 1, &port_conf);

    rte_eth_rx_queue_setup(
        port_id, 0, RX_DESC,
        rte_eth_dev_socket_id(port_id),
        NULL, mbuf_pool);

    rte_eth_tx_queue_setup(
        port_id, 0, TX_DESC,
        rte_eth_dev_socket_id(port_id),
        NULL);

    rte_eth_dev_start(port_id);

    printf("vhost-user backend READY (socket created)\n");

    /* Simple RX loop (consume packets) */
    while (1) {
        struct rte_mbuf *mbuf;
        if (rte_eth_rx_burst(port_id, 0, &mbuf, 1)) {
            printf("Packet received at backend\n");
            rte_pktmbuf_free(mbuf);
        }
    }
    //struct rte_pci_device x;
    

    return 0;
}
