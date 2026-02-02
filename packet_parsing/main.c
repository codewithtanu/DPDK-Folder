#include <stdio.h>
#include <rte_ethdev.h>

#define nl printf("\n");

struct rte_mempool * create_pool();


int
initialize_port(uint16_t port_id, struct rte_mempool *mbuf_pool);
void process_packet(uint16_t port_id);

int
main(int argc, char **argv)
{
    struct rte_mempool *mbuf_pool;
    uint16_t port_id;
    int nb_ports;
    int ret;

    /* EAL init */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    /* Create mbuf pool */
    mbuf_pool = create_pool();


    nb_ports=rte_eth_dev_count_avail();
    printf("Number of ports are:%d\n",nb_ports);

    port_id=0;
    initialize_port(port_id,mbuf_pool);

    printf("RX-only DPDK app started\n");

    nl
    nl
    while(1)
    {
        process_packet(port_id);
        
    }
    return 0;
}

