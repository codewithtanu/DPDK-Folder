#include <stdio.h>
#include <rte_ethdev.h>

const char *l2_to_str(uint16_t ether_type)
{
    switch (ether_type) {

    case 1:
        return "Ethernet";

    case 2:
        return "VLAN (802.1Q)";

    case 3:
        return "QinQ (802.1ad)";

    case 4:
        return "ARP";

    default:
        return "Unknown";
    }
}

const char *l3_to_str(uint8_t proto)
{
    switch (proto) {
    case 1:  return "IPV4";
    case 2:  return "IPV4-EXTENDED";
    case 3: return "IPV6";
    case 4: return "IPV6-EXTENDED";
    default: return "Other-L3";
    }
}

const char *l4_to_str(uint8_t proto)
{
    switch (proto) {
    case 1: return "TCP";
    case 2: return "UDP";
    case 3: return "SCTP";
    case 4: return "ICMP";
    default: return "L4/Unknown";
    }
}



void print_mbuff_info(struct rte_mbuf *m) {
    // Initialize the types to unknown (0)
    uint16_t l2_type = 0;
    uint8_t l3_type = 0;
    uint8_t l4_type = 0;

    // Get the packet data
    uint8_t *pkt = rte_pktmbuf_mtod(m, uint8_t *);
    uint16_t pkt_len = rte_pktmbuf_data_len(m);

    /* ---------- L2 : Ethernet ---------- */
    if (pkt_len < sizeof(struct rte_ether_hdr))
        return;

    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)pkt;
    l2_type = rte_be_to_cpu_16(eth->ether_type); // Set L2 type from EtherType

    printf("*****MBUF METADATA INFO*****\n");
    printf("Ports :%u\n", m->port);
    printf("Socket ID :%u\n", rte_socket_id());
    printf("Packet Length :%u\n", m->pkt_len);
    printf("Data Length :%u\n", m->data_len);
    printf("Data Offset:%u\n", m->data_off);
    printf("L2 type: %s %d\n", l2_to_str(l2_type), l2_type);

    /* ---------- L3 : IPv4 or IPv6 ---------- */
    uint8_t *l3 = pkt + sizeof(struct rte_ether_hdr);
    if (l2_type == RTE_ETHER_TYPE_IPV4) {
        if (pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr))
            return;

        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)l3;
        l3_type = 1; // Set L3 type to IPV4
        printf("L3: IPV4\n");

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

        uint8_t proto = ip->next_proto_id; // Protocol field to determine L4 type
        if (proto == IPPROTO_TCP) {
            l4_type = 1; // TCP
            printf("L4: TCP\n");
        } else if (proto == IPPROTO_UDP) {
            l4_type = 2; // UDP
            printf("L4: UDP\n");
        } else {
            l4_type = 0; // Unknown L4
            printf("L4: Unknown\n");
        }
    }
    else if (l2_type == RTE_ETHER_TYPE_IPV6) {
        // Handle IPv6 similarly
        l3_type = 3; // IPV6 type
        printf("L3: IPV6\n");
        // Add parsing for IPv6 header and L4 protocols
    }
    else if (l2_type == RTE_ETHER_TYPE_ARP) {
        l3_type = 4; // ARP (for L3)
        printf("L3: ARP\n");
    } else {
        printf("L3: Unknown EtherType (0x%04x)\n", l2_type);
    }

    // Print more packet details
    printf("Raw Packet Type: 0x%x\n", m->packet_type);
    printf("Buffer Length :%u\n", m->buf_len);
    printf("No of Segments :%u\n", m->nb_segs);
    printf("Offload flg : 0x%" PRIx64 "\n", m->ol_flags);
    printf("Inner Types\n");
    printf("Inner L2:%u, Inner L3:%u, Inner L4:%u\n", m->inner_l2_type, m->inner_l3_type, m->inner_l4_type);
}
