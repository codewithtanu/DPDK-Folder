#include <stdint.h>
#include <stdio.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#define NUM_MBUFS 512

int main(int argc, char **argv)
{
    uint16_t port_id = 0;
    struct rte_mempool *mbuf_pool;
    struct rte_mbuf *mbuf;
    struct rte_ether_hdr *eth;

    /* Initialize EAL */
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create(
        "VIRTIO_POOL",
        NUM_MBUFS,
        0,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Configure virtio-user port */
    struct rte_eth_conf port_conf = {0};

    rte_eth_dev_configure(port_id, 0, 1, &port_conf);
    rte_eth_tx_queue_setup(
        port_id, 0, 64,
        rte_eth_dev_socket_id(port_id),
        NULL);

    rte_eth_dev_start(port_id);

    /* Allocate and build packet */
    mbuf = rte_pktmbuf_alloc(mbuf_pool);
    rte_pktmbuf_append(mbuf, sizeof(struct rte_ether_hdr));

    eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

    struct rte_ether_addr src =
        {{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
    struct rte_ether_addr dst =
        {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

    rte_ether_addr_copy(&dst, &eth->dst_addr);
    rte_ether_addr_copy(&src, &eth->src_addr);
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    /* Send ONE packet */
    uint16_t sent = rte_eth_tx_burst(port_id, 0, &mbuf, 1);

    if (sent == 1)
        printf("virtio-user: Packet SENT\n");
    else {
        printf("virtio-user: TX failed\n");
        rte_pktmbuf_free(mbuf);
    }

    return 0;
}
