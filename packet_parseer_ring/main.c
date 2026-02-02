#include "ring.h"

struct rte_ring *rx_to_parser;// used between rx burst & parser
struct rte_ring* parse_to_tx;  
int receive_packet(void* arg);
int packet_parser(void *arg);
int transmit_packet(void *arg);

uint16_t port_id=0;

int main(int argc,char ** argv){

    NEW_LINE
    NEW_LINE
     int ret;
     struct rte_ethdev *x;
   ret=rte_eal_init(argc,argv);
   if(ret<0){
    printf("EAL Initialization Failed\n");
    return 1;
   }
   printf("Initialization Successfull\n");
   struct rte_mempool* mem_pool=NULL;
   mem_pool=rte_pktmbuf_pool_create(MEM_POOL,NUM_MBUF,MBUF_CACHE_SIZE,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
   
   if(!mem_pool){
    printf("Mem Pool Creation Failed\n");
    return 1;
   }

   printf("Memory Pool Created Successfully\n");

       struct rte_eth_conf port_conf = {0};

    if (rte_eth_dev_configure(port_id, RX_QUEUE, TX_QUEUE, &port_conf) < 0)
        rte_exit(EXIT_FAILURE, "Port configure failed\n");


        struct rte_eth_dev_info dev_info;
        rte_eth_dev_info_get(port_id, &dev_info);

printf("Max RX queues: %u\n", dev_info.max_rx_queues);

    struct rte_mempool *mbuf_pool =
        rte_pktmbuf_pool_create("MBUF_POOL", 8192,
                                256, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "MBUF pool create failed\n");

   
     for(int i=0;i<RX_QUEUE;i++){
             ret = rte_eth_rx_queue_setup(
        port_id,
        i,
        RX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        NULL,
        mbuf_pool
    );
    if (ret < 0)
        return ret;
     }

       for(int i=0;i<TX_QUEUE;i++){
        ret = rte_eth_tx_queue_setup(
        port_id,
        i,
        TX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        NULL
    );

    if (ret < 0)
        return ret;

     }
    if (rte_eth_dev_start(port_id) < 0)
        rte_exit(EXIT_FAILURE, "Port start failed\n");

    rx_to_parser=rte_ring_create(RING_RX_NAME,RX_RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);
    parse_to_tx=rte_ring_create(RING_TX_NAME,RX_RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);

     if (!rx_to_parser)
        rte_exit(EXIT_FAILURE, "Ring creation failed\n");

    printf("Port has Been Initialized & RING_RX Queue is being Created\n");

    printf("Launching the Threads for RX & Parser\n");
    NEW_LINE
    NEW_LINE
    rte_eal_remote_launch(receive_packet, NULL, 1);
    rte_eal_remote_launch(packet_parser, NULL,2);
    rte_eal_remote_launch(transmit_packet, NULL,3);
    rte_eal_mp_wait_lcore();
    return 0;
}
