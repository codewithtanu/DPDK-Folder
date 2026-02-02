
#include "ring.h"

extern uint16_t port_id;
extern struct rte_ring *rx_to_parser;
int receive_packet(void *arg)
{
    NEW_LINE
    NEW_LINE
    printf("RX lcore: %u\n", rte_lcore_id());
    int rx_packet = 0;

    while (1) {

        struct rte_mbuf *packets[BURST_SIZE];
        uint16_t nb_rx =
            rte_eth_rx_burst(port_id, 0, packets, BURST_SIZE);

        if (nb_rx > 0) {

            unsigned int enq =
                rte_ring_enqueue_burst(rx_to_parser,
                                        (void **)packets,
                                        nb_rx,
                                        NULL);

            /* FREE packets that could NOT be enqueued */
            for (unsigned int i = enq; i < nb_rx; i++) {
                rte_pktmbuf_free(packets[i]);
            }

            rx_packet += enq;

            printf("Received %u packets | Total %d\n",
                   enq, rx_packet);
        }

        if (rx_packet >= 25) {
            printf("Reached RX limit\n");
            break;
        }
    }
     NEW_LINE
    NEW_LINE
    return 0;
}
