#include <stdio.h>

#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>



#define NUM_MBUF 8192 // NO OF BUF 
#define MBUF_CACHE_SIZE 250

int main(int argc,char** argv){
	struct rte_mempool* m_pool;
	int ret;
	
	ret=rte_eal_init(argc,argv);
	
	if(ret<0){
	  printf("Something Went Wrong\n");
	  return 1;
	}
	  
	m_pool=rte_pktmbuf_pool_create("MEM_POOL",NUM_MBUF,MBUF_CACHE_SIZE,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
	
	printf("Size of Mem_Pool:%d\n",(int)sizeof(struct rte_mempool));
	
	if(m_pool==NULL){
	 printf("Something Went Wrong in Mem Pool\n");
	  return 1;
	}
	printf("Memory Pool Created Successfully\n");
	return 0;	
} 

