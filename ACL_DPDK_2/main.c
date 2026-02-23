#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>

static volatile int g_keep_running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    g_keep_running = 0;
}

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_acl.h>

#define MIN_PKT_LEN  (sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + 4)  /* eth + ip + ports */

#define NUM_FIELDS 5
#define MAX_RULES 32
#define MAX_PKT_BURST 32
#define MEMPOOL_CACHE_SIZE 256
#define NB_MBUF 8192
#define MAX_CATEGORIES 1

#define ACTION_ALLOW 1
#define ACTION_DROP 0

enum {
    PROTO,
    SRC_IP,
    DST_IP,
    SRC_PORT,
    DST_PORT
};

typedef struct pkt {
    struct rte_ipv4_hdr ip_hdr;
    struct rte_tcp_hdr tcp_hdr; /* enough for UDP too (ports overlap) */
} pkt;

struct acl_rule {
    struct rte_acl_rule_data data;
    struct rte_acl_field field[NUM_FIELDS];
};

static struct rte_acl_field_def field_defs[NUM_FIELDS] = {
    { RTE_ACL_FIELD_TYPE_BITMASK, sizeof(uint8_t),  PROTO,    0, offsetof(pkt, ip_hdr.next_proto_id) },
    { RTE_ACL_FIELD_TYPE_MASK,    sizeof(uint32_t), SRC_IP,   1, offsetof(pkt, ip_hdr.src_addr) },
    { RTE_ACL_FIELD_TYPE_MASK,    sizeof(uint32_t), DST_IP,   2, offsetof(pkt, ip_hdr.dst_addr) },
    { RTE_ACL_FIELD_TYPE_RANGE,   sizeof(uint16_t), SRC_PORT, 3, offsetof(pkt, tcp_hdr.src_port) },
    { RTE_ACL_FIELD_TYPE_RANGE,   sizeof(uint16_t), DST_PORT, 4, offsetof(pkt, tcp_hdr.dst_port) },
};

static struct rte_acl_ctx *build_acl(void) {
    struct rte_acl_param prm = {
        .name = "fw_acl",
        .socket_id = rte_socket_id(),
        .max_rule_num = MAX_RULES,
        .rule_size = RTE_ACL_RULE_SZ(NUM_FIELDS),
    };

    struct rte_acl_ctx *ctx = rte_acl_create(&prm);
    if (!ctx) return NULL;

    struct acl_rule rules[2];
    memset(rules, 0, sizeof(rules));

    /* Rule 0: Allow TCP from 192.168.1.0/24 to any dst port 80 */
    rules[0].data.priority = 10;
    rules[0].data.category_mask = 1;
    rules[0].data.userdata = ACTION_ALLOW;

    rules[0].field[PROTO].value.u8 = IPPROTO_TCP;
    rules[0].field[PROTO].mask_range.u8 = 0xFF;

    rules[0].field[SRC_IP].value.u32 = RTE_IPV4(192,168,1,0);
    rules[0].field[SRC_IP].mask_range.u32 = 24;

    rules[0].field[DST_IP].value.u32 = 0;
    rules[0].field[DST_IP].mask_range.u32 = 0;

    rules[0].field[SRC_PORT].value.u16 = 0;
    rules[0].field[SRC_PORT].mask_range.u16 = 65535;

    rules[0].field[DST_PORT].value.u16 = 80;
    rules[0].field[DST_PORT].mask_range.u16 = 80;

    /* Rule 1: Drop everything else */
    rules[1].data.priority = 1;
    rules[1].data.category_mask = 1;
    rules[1].data.userdata = ACTION_DROP;

    int ret = rte_acl_add_rules(ctx, (struct rte_acl_rule *)rules, 2);
    if (ret) return NULL;

    struct rte_acl_config cfg = {
        .num_categories = MAX_CATEGORIES,
        .num_fields = NUM_FIELDS,
    };
    memcpy(cfg.defs, field_defs, sizeof(field_defs));

    if (rte_acl_build(ctx, &cfg) != 0) return NULL;

    return ctx;
}

static int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_rings = 1, tx_rings = 1;

    if (rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf) != 0)
        return -1;

    if (rte_eth_rx_queue_setup(port, 0, 1024, rte_eth_dev_socket_id(port), NULL, mbuf_pool) < 0)
        return -1;

    if (rte_eth_tx_queue_setup(port, 0, 1024, rte_eth_dev_socket_id(port), NULL) < 0)
        return -1;

    if (rte_eth_dev_start(port) < 0)
        return -1;

    rte_eth_promiscuous_enable(port);
    return 0;
}

/* Simple flow: rx_burst -> ACL (allow/drop) -> allowed packets go to tx_burst, rest freed */
static void firewall_loop(uint16_t port, struct rte_acl_ctx *acl) {
    struct rte_mbuf *rx_bufs[MAX_PKT_BURST];
    struct rte_mbuf *tx_bufs[MAX_PKT_BURST];

    while (g_keep_running) {
        /* 1. Receive packets */
        uint16_t nb_rx = rte_eth_rx_burst(port, 0, rx_bufs, MAX_PKT_BURST);
        if (nb_rx == 0) {
            usleep(100);
            continue;
        }

        printf("Received %d packets\n", nb_rx);
        printf("--------------------------------\n");

        /* 2. Classify each packet: ALLOW or DROP */
        uint16_t nb_tx = 0;
        for (uint16_t i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = rx_bufs[i];
            pkt *p = rte_pktmbuf_mtod(m, pkt *);

            const uint8_t *data[1] = {(uint8_t *)p};
            uint32_t result[1];
            rte_acl_classify(acl, data, result, 1, MAX_CATEGORIES);

            if (result[0] == ACTION_ALLOW)
                tx_bufs[nb_tx++] = m;
            else{
                printf("Dropping packet\n");
                printf("--------------------------------\n");
                rte_pktmbuf_free(m);
            }
        }

        /* 3. Send allowed packets */
        if (nb_tx > 0){
            printf("Sending %d packets\n", nb_tx);
            printf("--------------------------------\n");
            rte_eth_tx_burst(port, 0, tx_bufs, nb_tx);
        }
    }
}

int main(int argc, char **argv) {
    if (rte_eal_init(argc, argv) < 0) return -1;

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL", NB_MBUF, MEMPOOL_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (!mbuf_pool) return -1;

    if (rte_eth_dev_count_avail() < 1) {
        printf("Need at least 1 port. Example: --vdev=eth_af_packet0,iface=eth0\n");
        return -1;
    }

    if (port_init(0, mbuf_pool) != 0)
        return -1;

    struct rte_acl_ctx *acl = build_acl();
    if (!acl) {
        printf("ACL build failed\n");
        return -1;
    }

    signal(SIGINT, sigint_handler);
    printf("ACL: rx -> allow/drop -> tx allowed on port 0. Ctrl+C to stop.\n");
    firewall_loop(0, acl);

    printf("Stopping...\n");
    rte_acl_free(acl);
    rte_eth_dev_stop(0);
    rte_eal_cleanup();
    return 0;
}
