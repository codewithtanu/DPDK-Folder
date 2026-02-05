#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#define NUM_MBUFS 1024
#define MBUF_CACHE_SIZE 256
#define BUF_SIZE RTE_MBUF_DEFAULT_BUF_SIZE

void print_mbuf_info(const char *msg, struct rte_mbuf *m)
{
    printf("\n=== %s ===\n", msg);
    printf("pkt_len   = %u\n", m->pkt_len);
    // printf("pkt_len   = %u\n", m->buf_addr);
    printf("data_len  = %u\n", m->data_len);
    printf("headroom  = %u\n", rte_pktmbuf_headroom(m));
    printf("tailroom  = %u\n", rte_pktmbuf_tailroom(m));
    printf("Data offset=%u\n",m->data_off);
    printf("Buffer addr: %d\n",(int)m->buf_addr);
    struct rte_ether_hdr *eth_header=rte_pktmbuf_mtod(m,struct rte_ether_hdr*);
    printf("Ethernet Header Address:%d\n",(int)eth_header);
    printf("Data BUffer :%d\n",(int)(m->buf_addr + m->data_off));
    printf("\n \n");
}


int main(int argc, char **argv)
{
    struct rte_mempool *mbuf_pool;
    struct rte_mbuf *m;

    if (rte_eal_init(argc, argv) < 0) {
        printf("EAL init failed\n");
        return -1;
    }

  
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                        NUM_MBUFS,
                                        MBUF_CACHE_SIZE,
                                        0,
                                        BUF_SIZE,
                                        rte_socket_id());

    if (mbuf_pool == NULL) {
        printf("Cannot create mbuf pool\n");
        return -1;
    }

    m = rte_pktmbuf_alloc(mbuf_pool);
    if (!m) {
        printf("mbuf alloc failed\n");
        return -1;
    }

    print_mbuf_info("After allocation", m);


    // Append 100 bytes 
    /*
    
        +------------+-------------+------------+
        |  HEADROOM  |    DATA     |  TAILROOM  |
        |   128 B    |   100 B     | 1948 B     |
        +------------+-------------+------------+
                    ^
                data pointer


    */
    char *data = rte_pktmbuf_append(m, 100);
    if (!data) {
        printf("Append failed\n");
        return -1;
    }

    for (int i = 0; i < 100; i++)
        data[i] = i;

    print_mbuf_info("After append 100 bytes", m);


    // Trim 40 bytes from end
    /* 
            +------------+---------+----------------+
            |  HEADROOM  |  DATA   |    TAILROOM    |
            |   128 B    |  60 B   |     1988 B     |
            +------------+---------+----------------+

    */

    if (rte_pktmbuf_trim(m, 40) < 0) {
        printf("Trim failed\n");
        return -1;
    }

    print_mbuf_info("After trim 40 bytes", m);


    //Prepend 20 bytes at start
    /*
            +--------+-------------+---------+--------+
            |HEADROOM|   NEW HDR   |  DATA   |TAILROOM|
            | 108 B  |    20 B     |  60 B   |1988 B  |
            +--------+-------------+---------+--------+
                    ^
                data pointer moved left
   

    */

    char *pre = rte_pktmbuf_prepend(m, 20);
    if (!pre) {
        printf("Prepend failed (no headroom)\n");
        return -1;
    }

    for (int i = 0; i < 20; i++)
        pre[i] = 0xAA;

    print_mbuf_info("After prepend 20 bytes", m);


    // Remove 10 bytes from start 
    if (!rte_pktmbuf_adj(m, 10)) {
        printf("Adj failed\n");
        return -1;
    }

    print_mbuf_info("After adj remove 10 bytes from start", m);

    rte_pktmbuf_free(m);

    printf("\nDemo completed successfully.\n");
    return 0;
}
