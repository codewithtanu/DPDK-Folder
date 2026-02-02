/*
         sudo ip tuntap add dev tap0 mode tap user $USER multi_queue
		sudo ip link set tap0 up


		// to run the application
		sudo ./sample -l 0 --no-huge --vdev=net_tap0,iface=tap0
   
*/
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

static int
port_init(uint16_t port_id, struct rte_mempool *mbuf_pool)
{

/*
             So DPDK needs a generic configuration structure that:

				describes what you want

				lets the driver decide how to implement it

				That structure is rte_eth_conf.
*/
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_queues = 1;
    const uint16_t tx_queues = 1;
    int retval;
    uint16_t q;

    /* Check if port is valid */
    if (!rte_eth_dev_is_valid_port(port_id))
        return -1;

    /* Configure the Ethernet device 
     
       rte_eth_dev_configure() configures a DPDK Ethernet port by validating and applying RX/TX queue counts and port features (RX/TX modes, offloads, RSS, etc.), preparing the device for queue setup but not yet enabling packet transmission or reception.
    */
  
    retval = rte_eth_dev_configure(port_id, rx_queues, tx_queues, &port_conf);
    if (retval < 0)
        return retval;

    /* Setup RX queue
       It creates and initializes ONE RX queue for a port and attaches a mempool to it so the NIC can receive packets into mbufs.

		After this call:

		The RX queue exists

		RX descriptors are allocated

		The RX queue knows where to get mbufs from

		But packets are NOT received yet

		Packets start flowing oPRINT_NEW_LINEy after:

		rte_eth_dev_start()
		
		| Function                  | Refers to   | Meaning                        |
		| ------------------------- | ----------- | ------------------------------ |
		| `rte_socket_id()`         | CPU / lcore | Where *this thread* is running |
		| `rte_eth_dev_socket_id()` | NIC / port  | Where *the device* is located  |

		| Function                  | Returns                        | Depends on               |
		| ------------------------- | ------------------------------ | ------------------------ |
		| `rte_socket_id()`         | NUMA node of **current lcore** | Which CPU is running     |
		| `rte_eth_dev_socket_id()` | NUMA node of **NIC**           | PCI placement            |
		| `SOCKET_ID_ANY`           | No fixed NUMA                  | Virtual / unknown device |

    
     */
    for (q = 0; q < rx_queues; q++) {
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
    for (q = 0; q < tx_queues; q++) {
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

    /* Start the Ethernet port */
    retval = rte_eth_dev_start(port_id);
    if (retval < 0)
        return retval;

    /* Enable promiscuous mode (optional) */
    rte_eth_promiscuous_enable(port_id);

    return 0;
}
int main(int argc, char **argv)
{
    struct rte_mempool *mbuf_pool;
    uint16_t port_id;
    int ret;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        8192,
        250,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Initialize all ports */
    RTE_ETH_FOREACH_DEV(port_id) {
        if (port_init(port_id, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", port_id);
    }

    printf("All ports initialized successfully\n");

    return 0;
}

