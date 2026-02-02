#include <stdio.h>
#include <inttypes.h>

#include <rte_ethdev.h>
#include <rte_ether.h>
#include <sys/types.h>

#define nl printf("\n");

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define RX_QUEUE 1
#define TX_QUEUE 1


int
initialize_port(uint16_t port_id, struct rte_mempool *mbuf_pool)
{
    struct rte_eth_conf port_conf = {0};
    int ret;

    if (!rte_eth_dev_is_valid_port(port_id))
        return -1;

    /* Configure port */
    ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0)
        return ret;
 
    printf("Configuration for the port has been done...\n");       

    
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

    struct rte_eth_dev_info dev_info;
    int ret_val = rte_eth_dev_info_get(0, &dev_info);
    if(ret_val<0){
        printf("error fetching dev info\n");
        return 1;
    }

    printf("max rx queues allowed is %d\n", dev_info.max_rx_queues);

    printf("TX and RX Queue has been set mapped to the port...\n");   


    /* Start port */
    ret = rte_eth_dev_start(port_id);

    printf("Port has been started now...\n");
    if (ret < 0)
        return ret;

    /* Promiscuous mode (important for TAP) */
    rte_eth_promiscuous_enable(port_id);

    /* Print MAC */
    struct rte_ether_addr addr;
    rte_eth_macaddr_get(port_id, &addr);

    printf("Port %u MAC: %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
           ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 "\n",
           port_id,
           RTE_ETHER_ADDR_BYTES(&addr));
    
    printf("Port %u has been Initialized and been Started.......\n",port_id);
    nl
    nl
    return 0;
}

