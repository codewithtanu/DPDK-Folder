/*
Simple example: IPv4 fragment reassembly using DPDK

Build:
  gcc reassembly.c -o reassembly $(pkg-config --cflags --libs libdpdk)

Run:
  sudo ./reassembly -l 0-1 --proc-type=primary --no-pci
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_ip_frag.h>
#include <rte_cycles.h>
#include <unistd.h>
#define NB_MBUF 8192
#define MBUF_CACHE 256
#define MAX_FRAGS 4

int main(int argc, char **argv)
{
    struct rte_mempool *mbuf_pool;
    struct rte_ip_frag_tbl *frag_tbl;
    struct rte_ip_frag_death_row death_row;

    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    printf("EAL initialized\n");

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NB_MBUF,
                                        MBUF_CACHE, 0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE,
                                        rte_socket_id());
    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    printf("Mbuf pool created\n");

    /* Create fragment table */
    frag_tbl = rte_ip_frag_table_create(1024, 1024, 1024,
                                        rte_get_tsc_hz(),
                                        rte_socket_id());
    if (!frag_tbl)
        rte_exit(EXIT_FAILURE, "Cannot create frag table\n");

    memset(&death_row, 0, sizeof(death_row));

    printf("Fragment table created. Waiting for fragments...\n");

    /* ---- Simulation: normally fragments come from RX ---- */
    while (1) {
        struct rte_mbuf *frag = NULL;
        struct rte_mbuf *reassembled;
        struct rte_ipv4_hdr *ip_hdr;

        /* In real app: frag = rte_eth_rx_burst(...) */
        /* Here we just sleep because we don't have real traffic */
        sleep(1);
        continue;

        /* Example processing (not reached in this dummy loop) */
        ip_hdr = rte_pktmbuf_mtod_offset(frag, struct rte_ipv4_hdr *,
                                         sizeof(struct rte_ether_hdr));

        reassembled = rte_ipv4_frag_reassemble_packet(
            frag_tbl,
            &death_row,
            frag,
            rte_rdtsc(),
            ip_hdr);

        if (reassembled == NULL) {
            /* Not complete yet */
            continue;
        }

        printf("Packet reassembled! Total length: %u bytes\n",
               rte_pktmbuf_pkt_len(reassembled));

        rte_pktmbuf_free(reassembled);

        /* Free expired fragments */
        rte_ip_frag_free_death_row(&death_row, MAX_FRAGS);
    }

    return 0;
}