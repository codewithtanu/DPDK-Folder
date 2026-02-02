#include <stdio.h>

#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>



#define NUM_MBUF 8192 // NO OF BUF 
#define MBUF_CACHE_SIZE 250 // each cache in lcore to mbuf = 250
/*
    Hugepage memory
		│
		├── struct rte_mempool        → ~192 bytes (ONCE)
		│
		├── mbuf #0
		│   ├── struct rte_mbuf       → ~128 bytes
		│   └── data buffer           → 2048 bytes
		│
		├── mbuf #1
		│   ├── struct rte_mbuf
		│   └── data buffer
		│
		├── ...
		│
		└── mbuf #8191
		
		struct rte_mbuf (metadata)
				│
				└── buf_addr  ───────────────► [ data buffer (2048 bytes) ]
								                  ↑
								                  │
								              data_off
								              
				buf_addr
				│
				├── headroom (for prepending headers)
				│
				├── Ethernet header
				├── IP header
				├── TCP/UDP header
				├── payload



*/
int main(int argc,char** argv){
	struct rte_mempool* m_pool;
	struct rte_mbuf* mbuf;
	int ret;
	
	ret=rte_eal_init(argc,argv);
	
	if(ret<0){
	  printf("Something Went Wrong\n");
	  return 1;
	}
	                                // RTE_MBUF_DEFAULT_BUF_SIZE = default size of data buffer is 2048 contains header + payload
	                                // mbuf metadata = 128
	  m_pool = rte_mempool_create(
        "MEM_POOL",
        NUM_MBUF,                                  /* number of objects */
        RTE_MBUF_DEFAULT_BUF_SIZE +                /* data buffer */
        sizeof(struct rte_mbuf),                   /* mbuf metadata */
        MBUF_CACHE_SIZE,                           /* per-lcore cache */
        0,                                         /* private data size */
        rte_pktmbuf_pool_init,                     /* mempool init */
        NULL,
        rte_pktmbuf_init,                          /* object init */
        NULL,
        rte_socket_id(),                           /* NUMA socket */
        0
    );
	
	printf("Size of Mem_Pool:%d && dfault : %d\n",(int)sizeof(struct rte_mempool),RTE_MBUF_DEFAULT_BUF_SIZE);//size - 192
	
	if(m_pool==NULL){
	 printf("Something Went Wrong in Mem Pool\n");
	  return 1;
	}
	printf("Memory Pool Created Successfully\n");
	
	mbuf=rte_pktmbuf_alloc(m_pool);
	
	if (mbuf== NULL) {
    printf("No free mbufs available\n");
    return 1;
    }
    
    printf("Memory for the buffer is being allocated successfully\n");
    
    char msg[] = "hello";
	uint8_t *data = rte_pktmbuf_mtod(mbuf, uint8_t *);
	memcpy(data, msg, sizeof(msg));
	mbuf->data_len = sizeof(msg);
	mbuf->pkt_len  = sizeof(msg);
     
    uint8_t* data2=rte_pktmbuf_mtod(mbuf,uint8_t*);
    int len=mbuf->data_len;
    for(int i=0;i<len;i++){
        printf("%c",data2[i]);
    } 
    printf("\n");
	return 0;	
} 

