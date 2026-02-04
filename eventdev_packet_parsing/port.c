 #include <stdio.h>
#include <rte_ethdev.h> 
#include "port.h"
 int port_init(){
 struct rte_mempool* mem_pool=NULL;
    mem_pool=rte_pktmbuf_pool_create("MEM_POOL",8192,256,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
    if(!mem_pool){
        printf("Error in Memory Pool Creation\n");
        return 1;
    } 

    printf("Memory Pool is Created\n");

    struct rte_eth_conf port_conf={0};
 
    if(rte_eth_dev_configure(0,1, 1, &port_conf)<0){
        printf("Error in Port Configure\n");
        return 1;
    }

    if(rte_eth_rx_queue_setup(0, 0, 1024, rte_socket_id(),NULL,mem_pool)<0){
        printf("Error in RX Queue Setup\n");
        return 1;
    }

    if(rte_eth_tx_queue_setup(0, 0, 1024, rte_socket_id(),NULL)<0){
        printf("Error in TX Queue Setup\n");
        return 1;
    }

    if(rte_eth_dev_start(0)<0){
        printf("Error in Port Start\n");
        return 1;
    }

    printf("Ethernet Port has been Started\n");
    return 0;   
}