#include <netinet/in.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_acl.h>

#define NUM_FIELDS 5
#define MAX_RULES 10

#define ACTION_ALLOW 1
#define ACTION_DROP 0

#define MAX_CATEGORIES 1

enum {
    PROTO,
    SRC_IP,
    DST_IP,
    SRC_PORT,
    DST_PORT
};


typedef struct  pkt {
    struct rte_ipv4_hdr ip_hdr;
    struct rte_tcp_hdr tcp_hdr;
} pkt;

struct acl_rule {
    struct rte_acl_rule_data data;
    struct rte_acl_field field[NUM_FIELDS];
};


/* DPDK ACL requires first field to be 1 byte. Field order: PROTO, then IPs, then ports. */
static struct rte_acl_field_def field_defs[NUM_FIELDS] = {
    {
        .type = RTE_ACL_FIELD_TYPE_BITMASK,
        .size = sizeof(uint8_t),
        .field_index = PROTO,
        .input_index = 0,
        .offset = offsetof(pkt, ip_hdr.next_proto_id),
    },
    {
        .type = RTE_ACL_FIELD_TYPE_MASK,
        .size = sizeof(uint32_t),
        .field_index = SRC_IP,
        .input_index = 1,
        .offset = offsetof(pkt, ip_hdr.src_addr),
    },
    {
        .type = RTE_ACL_FIELD_TYPE_MASK,
        .size = sizeof(uint32_t),
        .field_index = DST_IP,
        .input_index = 2,
        .offset = offsetof(pkt, ip_hdr.dst_addr),
    },
    {
        .type = RTE_ACL_FIELD_TYPE_RANGE,
        .size = sizeof(uint16_t),
        .field_index = SRC_PORT,
        .input_index = 3,
        .offset = offsetof(pkt, tcp_hdr.src_port),
    },
    {
        .type = RTE_ACL_FIELD_TYPE_RANGE,
        .size = sizeof(uint16_t),
        .field_index = DST_PORT,
        .input_index = 4,
        .offset = offsetof(pkt, tcp_hdr.dst_port),
    },
};

int main(int argc, char** argv) {
    int ret;

    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("error in rte_eal_init\n");
        return 1;
    }

    argc -= ret;
    argv += ret;

    struct rte_acl_param acl_param = {
        .name = "ACL_PARAM",
        .socket_id = rte_socket_id(),
        .max_rule_num = MAX_RULES,
        .rule_size = RTE_ACL_RULE_SZ(RTE_DIM(field_defs)),
    };

    struct rte_acl_ctx* acl_ctx = rte_acl_create(&acl_param);
    if (!acl_ctx) {
        printf("error in creating the acl context\n");
        rte_eal_cleanup();
        return 2;
    }

    struct acl_rule rules[2];
    memset(rules, 0, sizeof(rules));



    // Rule 0: Allow TCP from 192.168.1.0/24
    rules[0].data.priority = 1;
    rules[0].data.category_mask = 1;
    rules[0].data.userdata = ACTION_ALLOW;

    /* Rules must be in HOST byte order. MASK type uses prefix length (24 = /24), not bitmask. */
    rules[0].field[SRC_IP].value.u32 = RTE_IPV4(192, 168, 1, 0);
    rules[0].field[SRC_IP].mask_range.u32 = 24;

    rules[0].field[DST_IP].value.u32 = 0;
    rules[0].field[DST_IP].mask_range.u32 = 0;  /* 0 = wildcard, match any */

    rules[0].field[SRC_PORT].value.u16 = 0;
    rules[0].field[SRC_PORT].mask_range.u16 = 65535;  /* RANGE: [0, 65535] = any port */

    rules[0].field[DST_PORT].value.u16 = 0;
    rules[0].field[DST_PORT].mask_range.u16 = 65535;

    rules[0].field[PROTO].value.u8 = IPPROTO_TCP;
    rules[0].field[PROTO].mask_range.u8 = 0xFF;

    // // Rule 1: Drop all others
    // rules[1].data.priority = 0;
    // rules[1].data.category_mask = 1;
    // rules[1].data.userdata = ACTION_DROP;

    // for (int i = 0; i < NUM_FIELDS; i++) {
    //     // Set value and mask for all fields
    //     rules[1].field[i].value.u64 = 0;      // zero value
    //     rules[1].field[i].mask_range.u64 = 0; // wildcard
    // }

    


    ret = rte_acl_add_rules(acl_ctx, (struct rte_acl_rule*)rules, 1);
    if (ret != 0) {
        printf("some error while adding rules\n");
        rte_acl_free(acl_ctx);
        rte_eal_cleanup();
        return 3;
    }


    struct rte_acl_config cfg = {
        .num_categories = MAX_CATEGORIES,
        .num_fields = NUM_FIELDS,
    };
    memcpy(&cfg.defs, field_defs, sizeof(field_defs));

    ret = rte_acl_build(acl_ctx, &cfg);
    if (ret != 0) {
        printf("error while building acl\n");
        rte_acl_free(acl_ctx);
        rte_eal_cleanup();
        return 4;
    }

    printf("ACL built successfully\n");


    
    pkt pkt1;
    memset(&pkt1, 0, sizeof(pkt1));

    pkt1.ip_hdr.src_addr = rte_cpu_to_be_32(RTE_IPV4(192,168,1,10));
    pkt1.tcp_hdr.src_port = rte_cpu_to_be_16(1234);
    pkt1.tcp_hdr.dst_port = rte_cpu_to_be_16(80);
    pkt1.ip_hdr.next_proto_id = IPPROTO_TCP;


    const uint8_t* data[1];
    data[0] = (uint8_t*)&pkt1;
    uint32_t results[1];

    ret = rte_acl_classify(acl_ctx, data, results, 1, MAX_CATEGORIES);
    if (ret != 0) {
        printf("incorrect arguments given to the classify function\n");
    } else {
        printf("value of result is %d\n", results[0]);
        if (results[0] == ACTION_ALLOW) {
            printf("packet was allowed\n");
        } else {
            printf("packet was not allowed\n");
        }
    }

    rte_acl_free(acl_ctx);
    rte_eal_cleanup();
    printf("closing\n");

    return 0;
}