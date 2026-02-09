#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_reorder.h>

#define NUM_MBUFS 1024
#define MBUF_CACHE_SIZE 32
#define REORDER_SIZE 16
#define BURST 32

/* Helper to set sequence number using the internal offset */
static inline void set_seqn(struct rte_mbuf *m, uint32_t seqn)
{
    /* Use the first dynfield for sequence number */
    *rte_reorder_seqn(m) = seqn;
}

/* Store sequence number in packet data */
static struct rte_mbuf *create_packet(struct rte_mempool *mp, uint32_t seq)
{
    struct rte_mbuf *m = rte_pktmbuf_alloc(mp);
    if (!m)
        return NULL;

    uint32_t *data = rte_pktmbuf_mtod(m, uint32_t *);
    *data = seq;
    m->data_len = sizeof(uint32_t);
    m->pkt_len  = sizeof(uint32_t);
    
    /* Set the reorder sequence number */
    set_seqn(m, seq);

    return m;
}

int main(int argc, char **argv)
{
    struct rte_mempool *mbuf_pool;
    struct rte_reorder_buffer *reorder_buf;
    struct rte_mbuf *mbufs[BURST];
    unsigned i, nb_drain;

    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                        NUM_MBUFS,
                                        MBUF_CACHE_SIZE,
                                        0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE,
                                        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    reorder_buf = rte_reorder_create("REORDER_BUF",
                                     rte_socket_id(),
                                     REORDER_SIZE);
                                     
    if (!reorder_buf)
        rte_exit(EXIT_FAILURE, "Cannot create reorder buffer\n");

    printf("Simulating out-of-order packet arrival:\n");

    uint32_t arrival_seq[] = {0, 2, 1, 4, 3};

    for (i = 0; i < sizeof(arrival_seq) / sizeof(arrival_seq[0]); i++) {
        struct rte_mbuf *m = create_packet(mbuf_pool, arrival_seq[i]);

        printf("Insert seq=%u\n", arrival_seq[i]);
        
        if (rte_reorder_insert(reorder_buf, m) < 0) {
            printf("Insert failed for seq=%u\n", arrival_seq[i]);
            rte_pktmbuf_free(m);
        }
    }

    printf("\nDraining reordered packets:\n");

    int total_drained = 0;
    while (1) {
        nb_drain = rte_reorder_drain(reorder_buf, mbufs, BURST);
        
        if (nb_drain == 0)
            break;

        printf("nb_drains:%d\n",nb_drain);
            
        total_drained += nb_drain;
        printf("nb_drain:%d\n", nb_drain);
        
        for (i = 0; i < nb_drain; i++) {
            uint32_t *seq = rte_pktmbuf_mtod(mbufs[i], uint32_t *);
            printf("Output seq=%u\n", *seq);
            rte_pktmbuf_free(mbufs[i]);
        }
    }
    
    printf("Total packets drained: %d\n", total_drained);

    return 0;
}

