#include <rte_lcore.h>
#include <stdio.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#define POOL_NAME  "MEM_POOL"
#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 250

struct rte_mempool * create_pool(){
   struct rte_mempool * mem_pool=NULL;

    mem_pool=rte_pktmbuf_pool_create(
            POOL_NAME,
            NUM_MBUFS,
            MBUF_CACHE_SIZE,
            0,
            RTE_MBUF_DEFAULT_BUF_SIZE,
            rte_socket_id()
    );

    if(mem_pool==NULL){
        rte_exit(EXIT_FAILURE,"Memory Pool creation Failed.....\n");
    }
    printf("Memory Pool Creation is Successfull......\n");
    return mem_pool;
}