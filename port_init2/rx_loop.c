#include <stdio.h>
#include <stdint.h>

#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_byteorder.h>

#include "rx_loop.h"

#define BURST_SIZE 32

void
rx_loop(uint16_t port_id)
{
    struct rte_mbuf *rx_pkts[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(
            port_id,       /* port */
            0,             /* queue */
            rx_pkts,
            BURST_SIZE
        );

        if (nb_rx == 0)
            continue;

        for (uint16_t i = 0; i < nb_rx; i++) {

            struct rte_mbuf *m = rx_pkts[i];
            printf("L2 Type:%d\n",m->l2_type);
            printf("L3 Type:%d\n",m->l3_type);
            printf("L4 Type:%d\n",m->l4_type);
            /* ---- Ethernet header ---- */
            struct rte_ether_hdr *eth =
                rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

            uint16_t ether_type =
                rte_be_to_cpu_16(eth->ether_type);

            printf("\nPacket length: %u bytes\n", m->pkt_len);
            printf("EtherType: 0x%04x\n", ether_type);

            /* ---- IPv4 header ---- */
            if (ether_type == RTE_ETHER_TYPE_IPV4) {
                struct rte_ipv4_hdr *ip =
                    (struct rte_ipv4_hdr *)(eth + 1);

                uint32_t src = rte_be_to_cpu_32(ip->src_addr);

                printf("IPv4 Src IP: %u.%u.%u.%u\n",
                       (src >> 24) & 0xff,
                       (src >> 16) & 0xff,
                       (src >> 8) & 0xff,
                       src & 0xff);
            }

            /* ---- MODIFY DEST MAC ---- */
            struct rte_ether_addr new_mac =
                {{0x02, 0x11, 0x22, 0x33, 0x44, 0x55}};

            rte_ether_addr_copy(&new_mac, &eth->dst_addr);

            printf("Destination MAC rewritten\n");

            /* IMPORTANT: free mbuf */
            rte_pktmbuf_free(m);
        }
    }
}

