#include <stdio.h>

#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <inttypes.h>


#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUF 8192 // NO OF BUFFER 
#define MBUF_CACHE_SIZE 250 // each cache in lcore to mbuf = 250

/*
         sudo ip tuntap add dev tap0 mode tap user $USER multi_queue
		sudo ip link set tap0 up


		// to run the application
		sudo ./sample -l 0 --no-huge --vdev=net_tap0,iface=tap0
   
*/


static int port_init(uint16_t port_id,struct rte_mempool* m_pool){
   struct rte_eth_conf port_conf={0};
   const uint16_t rx_queue=1;
   const uint16_t tx_queue=1;
   int retval;
   uint16_t q;
   
   if (!rte_eth_dev_is_valid_port(port_id))
        return -1;
        
  printf("\nPort has been validated......\n");
        
  retval=rte_eth_dev_configure(port_id,rx_queue,tx_queue,&port_conf);
  
  printf("Configured the NIC with the features......\n");
    if (retval < 0)
        return retval;
        
    for (q = 0; q < rx_queue; q++) {
        retval = rte_eth_rx_queue_setup(
            port_id,
            q,
            RX_RING_SIZE,
            rte_eth_dev_socket_id(port_id),// for NUMA node cache locality
            NULL,
            m_pool
        );
        if (retval < 0)
            return retval;
    }
    
     /* Setup TX queue */
    for (q = 0; q < tx_queue; q++) {
        retval = rte_eth_tx_queue_setup(
            port_id,
            q,
            TX_RING_SIZE,
            rte_eth_dev_socket_id(port_id),
            NULL
        );
        if (retval < 0)
            return retval;
    }
    
    printf("Setup the tx and rx queue for packet processing\n");
     /* Start the Ethernet port */
    retval = rte_eth_dev_start(port_id);
    if (retval != 0)
		return retval;
   printf("Now the NIC has been started to receivces the packet\n");
   
   struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port_id, &addr);
	if (retval != 0)
		return retval;
  //PRIx8 is a format specifier macro used with printf to safely print a uint8_t integer in hexadecimal.
  /*
      printf("Port %u MAC: %02x %02x %02x %02x %02x %02x\n",
       port_id,
       addr.addr_bytes[0],
       addr.addr_bytes[1],
       addr.addr_bytes[2],
       addr.addr_bytes[3],
       addr.addr_bytes[4],
       addr.addr_bytes[5]);

  
  */
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port_id, RTE_ETHER_ADDR_BYTES(&addr));
			
   rte_eth_promiscuous_enable(port_id);

   return 0;
}


int main(int argc,char** argv){
	struct rte_mempool* m_pool;
	int ret;
	uint16_t port_id,nb_ports;
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
  printf("Total mbufs: %u\n", rte_mempool_avail_count(m_pool));

   nb_ports = rte_eth_dev_count_avail();
   printf("Number of DPDK ports: %u\n", nb_ports);
	printf("Memory Pool Created Successfully\n");
    RTE_ETH_FOREACH_DEV(port_id){
      if(port_init(port_id,m_pool)!=0){
        printf("Error in Initializing the port %d\n",port_id);
        return 1;
      }
    }
    printf("All ports initialized successfully\n");
    
	return 0;	
} 

