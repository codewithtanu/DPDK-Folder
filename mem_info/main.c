#include <rte_ether.h>
#include <rte_mbuf_core.h>
#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ethdev.h>

#define NUM_MBUFS 1024
#define MBUF_CACHE_SIZE 256
#define BUF_SIZE RTE_MBUF_DEFAULT_BUF_SIZE
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024


void print_mbuf_info(struct rte_mbuf *m)
{
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
    int port_id=0;
    //struct rte_mbuf *m;

    if (rte_eal_init(argc, argv) < 0) {
        printf("EAL init failed\n");
        return -1;
    }

     struct rte_eth_conf port_conf={0};
   const uint16_t rx_queue=1;
   const uint16_t tx_queue=1;
   int retval;
   uint16_t q;
   
   if (!rte_eth_dev_is_valid_port(port_id))
        return -1;
        
  printf("\nPort has been validated......\n");
        
  retval=rte_eth_dev_configure(port_id,rx_queue,tx_queue,&port_conf);

  mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                      NUM_MBUFS,
                                      MBUF_CACHE_SIZE,
                                      0,
                                      BUF_SIZE,
                                      rte_socket_id());
  
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
            mbuf_pool
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

  

    if (mbuf_pool == NULL) {
        printf("Cannot create mbuf pool\n");
        return -1;
    }

    struct rte_mbuf* pkts[32];

    while(1){
        int num_rx=rte_eth_rx_burst(0,0,pkts,32);
        for(int i=0;i<num_rx;i++){
            print_mbuf_info(pkts[i]);
        }
    }

    
    return 0;
}
