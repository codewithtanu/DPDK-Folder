#include <stdint.h>
#include <stdio.h>

#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ether.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define nl printf("\n");
#define BURST_SIZE 32

void print_mbuff_info(struct rte_mbuf * m);
void print_mbuf_data(struct rte_mbuf *m)
{
    uint8_t *pkt = rte_pktmbuf_mtod(m, uint8_t *);
    uint16_t pkt_len = rte_pktmbuf_data_len(m);

    /* ---------- L2 : Ethernet ---------- */
    if (pkt_len < sizeof(struct rte_ether_hdr))
        return;

    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)pkt;

    printf("L2:\n");
    printf("  SRC MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->src_addr.addr_bytes[0],
           eth->src_addr.addr_bytes[1],
           eth->src_addr.addr_bytes[2],
           eth->src_addr.addr_bytes[3],
           eth->src_addr.addr_bytes[4],
           eth->src_addr.addr_bytes[5]);

    printf("  DST MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->dst_addr.addr_bytes[0],
           eth->dst_addr.addr_bytes[1],
           eth->dst_addr.addr_bytes[2],
           eth->dst_addr.addr_bytes[3],
           eth->dst_addr.addr_bytes[4],
           eth->dst_addr.addr_bytes[5]);

    uint16_t ether_type = rte_be_to_cpu_16(eth->ether_type);
    uint8_t *l3 = pkt + sizeof(struct rte_ether_hdr);

    /* ---------- L3 : IPv4 ---------- */
    if (ether_type == RTE_ETHER_TYPE_IPV4) {
        if (pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr))
            return;

        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)l3;
        uint16_t ip_hdr_len = (ip->version_ihl & 0x0F) * 4;

        printf("L3: IPv4\n");
        printf("  SRC IP: %u.%u.%u.%u\n",
               (ip->src_addr >> 24) & 0xff,
               (ip->src_addr >> 16) & 0xff,
               (ip->src_addr >> 8) & 0xff,
               ip->src_addr & 0xff);

        printf("  DST IP: %u.%u.%u.%u\n",
               (ip->dst_addr >> 24) & 0xff,
               (ip->dst_addr >> 16) & 0xff,
               (ip->dst_addr >> 8) & 0xff,
               ip->dst_addr & 0xff);

        uint8_t proto = ip->next_proto_id;
        uint8_t *l4 = l3 + ip_hdr_len;

        /* ---------- L4 : TCP ---------- */
        if (proto == IPPROTO_TCP) {
            if (pkt_len < sizeof(struct rte_ether_hdr) + ip_hdr_len + sizeof(struct rte_tcp_hdr))
                return;

            struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)l4;

            printf("L4: TCP\n");
            printf("  SRC PORT: %u\n", rte_be_to_cpu_16(tcp->src_port));
            printf("  DST PORT: %u\n", rte_be_to_cpu_16(tcp->dst_port));
            printf("  SEQ NO  : %u\n", rte_be_to_cpu_32(tcp->sent_seq));
        }
        /* ---------- L4 : UDP ---------- */
        else if (proto == IPPROTO_UDP) {
            if (pkt_len < sizeof(struct rte_ether_hdr) + ip_hdr_len + sizeof(struct rte_udp_hdr))
                return;

            struct rte_udp_hdr *udp = (struct rte_udp_hdr *)l4;

            printf("L4: UDP\n");
            printf("  SRC PORT: %u\n", rte_be_to_cpu_16(udp->src_port));
            printf("  DST PORT: %u\n", rte_be_to_cpu_16(udp->dst_port));
        }
        else {
            printf("L4: Unknown (proto=%u)\n", proto);
        }
    }

    /* ---------- L3 : IPv6 ---------- */
    else if (ether_type == RTE_ETHER_TYPE_IPV6) {
        if (pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv6_hdr))
            return;

        struct rte_ipv6_hdr *ip6 = (struct rte_ipv6_hdr *)l3;
        uint8_t proto = ip6->proto;
        uint8_t *l4 = l3 + sizeof(struct rte_ipv6_hdr);

        if (proto == IPPROTO_TCP) {
            struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)l4;

            printf("L4: TCP\n");
            printf("  SRC PORT: %u\n", rte_be_to_cpu_16(tcp->src_port));
            printf("  DST PORT: %u\n", rte_be_to_cpu_16(tcp->dst_port));
            printf("  SEQ NO  : %u\n", rte_be_to_cpu_32(tcp->sent_seq));

        } else if (proto == IPPROTO_UDP) {
            struct rte_udp_hdr *udp = (struct rte_udp_hdr *)l4;

            printf("L4: UDP\n");
            printf("  SRC PORT: %u\n", rte_be_to_cpu_16(udp->src_port));
            printf("  DST PORT: %u\n", rte_be_to_cpu_16(udp->dst_port));
        }
        else {
            printf("L4: Unknown (next_header=%u)\n", proto);
        }
    }
    else {
        printf("L3: Unknown EtherType (0x%04x)\n", ether_type);
    }

    /* ---------- L3 : ARP ---------- */
    if (ether_type == RTE_ETHER_TYPE_ARP) {
        printf("L3: ARP\n");
        // Handle ARP packet parsing here
    }
}



void process_packet(uint16_t port_id)
{
    struct rte_mbuf *rx_pkts[BURST_SIZE];
    uint16_t nb_rx = rte_eth_rx_burst(
        port_id, 0, rx_pkts, BURST_SIZE);

    if (nb_rx == 0){
        //printf("Not recevied packet 0 \n");
        return;  
    } // keep polling

    printf("Received %u packets\n", nb_rx);

    for (uint16_t i = 0; i < nb_rx; i++) {
        struct rte_mbuf *m = rx_pkts[i];

        print_mbuff_info(m);
        print_mbuf_data(m);

        rte_pktmbuf_free(rx_pkts[i]);
    }

        uint16_t nb_tx = rte_eth_tx_burst(
            port_id, 0, rx_pkts, nb_rx);

        if (nb_tx < nb_rx) {
            printf("Dropped packet : %u",nb_rx-nb_tx);
            for (uint16_t i = nb_tx; i < nb_rx; i++)
                rte_pktmbuf_free(rx_pkts[i]);
        }
    //}
}



