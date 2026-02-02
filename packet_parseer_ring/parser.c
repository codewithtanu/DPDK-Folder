
#include "ring.h"
#include <rte_mbuf.h>
#include <rte_ring.h>

extern uint16_t port_id;
extern struct rte_ring *rx_to_parser;
extern struct rte_ring *parse_to_tx;

void packet_info(struct rte_mbuf** packets,int nb_pkts){
    NEW_LINE
    NEW_LINE
       
    for (uint16_t i = 0; i < nb_pkts; i++) {

        struct rte_mbuf *m = packets[i];

        /* Get Ethernet header */
        struct rte_ether_hdr *eth =
            rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

        /* Print MAC addresses */
        printf("\nPacket %u\n", i + 1);
        printf("SRC MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               eth->src_addr.addr_bytes[0],
               eth->src_addr.addr_bytes[1],
               eth->src_addr.addr_bytes[2],
               eth->src_addr.addr_bytes[3],
               eth->src_addr.addr_bytes[4],
               eth->src_addr.addr_bytes[5]);

        printf("DST MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               eth->dst_addr.addr_bytes[0],
               eth->dst_addr.addr_bytes[1],
               eth->dst_addr.addr_bytes[2],
               eth->dst_addr.addr_bytes[3],
               eth->dst_addr.addr_bytes[4],
               eth->dst_addr.addr_bytes[5]);
    NEW_LINE
    NEW_LINE
        /* Get EtherType */
        uint16_t ether_type = rte_be_to_cpu_16(eth->ether_type);

        /* ---------------- IPv4 ---------------- */
        if (ether_type == RTE_ETHER_TYPE_IPV4) {

            struct rte_ipv4_hdr *ip =
                (struct rte_ipv4_hdr *)(eth + 1);

            char src_ip[INET_ADDRSTRLEN];
            char dst_ip[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &ip->src_addr, src_ip, sizeof(src_ip));
            inet_ntop(AF_INET, &ip->dst_addr, dst_ip, sizeof(dst_ip));

            printf("Protocol : IPv4\n");
            printf("SRC IP   : %s\n", src_ip);
            printf("DST IP   : %s\n", dst_ip);
            NEW_LINE
            NEW_LINE
        }
        /* ---------------- IPv6 ---------------- */
        else if (ether_type == RTE_ETHER_TYPE_IPV6) {

            struct rte_ipv6_hdr *ip6 =
                (struct rte_ipv6_hdr *)(eth + 1);

            char src_ip[INET6_ADDRSTRLEN];
            char dst_ip[INET6_ADDRSTRLEN];

            // inet_ntop(AF_INET6, ip6->src_addr, src_ip, sizeof(src_ip));
            // inet_ntop(AF_INET6, ip6->dst_addr, dst_ip, sizeof(dst_ip));

            printf("Protocol : IPv6\n");
            printf("SRC IP   : %s\n", src_ip);
            printf("DST IP   : %s\n", dst_ip);
            NEW_LINE
            NEW_LINE
        }
        /* ---------------- OTHER ---------------- */
        else {
            printf("Protocol : Unknown (EtherType: 0x%04X)\n", ether_type);
        }
    }
}


int packet_parser(void *arg)
{
    printf("Parser lcore: %u\n", rte_lcore_id());
    int pars_packet = 0;

    while (1) {

        struct rte_mbuf *packets[BURST_SIZE];

        unsigned int nb_pr =
            rte_ring_dequeue_burst(rx_to_parser,
                                   (void **)packets,
                                   BURST_SIZE,
                                   NULL);

        if (nb_pr > 0) {
            pars_packet += nb_pr;
            printf("Parser got %u packets\n", nb_pr);
            packet_info(packets,nb_pr);
            
            uint16_t nb_tx=rte_ring_enqueue_burst(parse_to_tx, (void**)packets,nb_pr, NULL);

            // if(nb_tx<nb_pr){
            //     for(int i=nb_tx;i<nb_pr;i++){
            //         rte_pktmbuf_free(packets[i]);
            //     }
            // }
            
        }

        if (pars_packet >= 25) {
            printf("Reached Parser limit\n");
            break;
        }
    }
    return 0;
}
