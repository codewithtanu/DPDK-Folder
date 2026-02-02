
// #include "ring.h"
// #include <rte_ring.h>

// extern uint16_t port_id;
// extern struct rte_ring *parse_to_tx;
// int transmit_packet(void *arg)
// {
//     printf("TX Lcore: %u\n", rte_lcore_id());
//     int tx_packet = 0;

//     while (1) {

//         struct rte_mbuf *packets[BURST_SIZE];
//         uint16_t nb_tx =
//             rte_ring_dequeue_burst(parse_to_tx, (void**)packets, BURST_SIZE,NULL);

//         if (nb_tx > 0) {
//             tx_packet+=nb_tx;
//             unsigned int enq =
//                 rte_eth_tx_burst(port_id,0,packets,nb_tx);

//             /* FREE packets that could NOT be enqueued */
//             for (unsigned int i = enq; i < nb_tx; i++) {
//                 rte_pktmbuf_free(packets[i]);
//             }
//         }



//         if(tx_packet>=25){
//             printf("Max Transmit Has been Reached %d\n",tx_packet);
//             break;
//         }
//     }

//     return 0;
// }

#include "ring.h"
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>

extern uint16_t port_id;
extern struct rte_ring *parse_to_tx;

int transmit_packet(void *arg)
{
    printf("TX lcore: %u\n", rte_lcore_id());
    int tx_packet = 0;

    while (1) {

        struct rte_mbuf *packets[BURST_SIZE];

        unsigned int nb_tx =
            rte_ring_dequeue_burst(parse_to_tx,
                                   (void **)packets,
                                   BURST_SIZE,
                                   NULL);

        if (nb_tx == 0) {
            //rte_pause();  // prevent 100% CPU spin
            continue;
        }

        unsigned int sent =
            rte_eth_tx_burst(port_id, 0, packets, nb_tx);

        tx_packet += sent;
        printf("Sent %d Packets to NIC\n",tx_packet);
        /* Free unsent packets */
        for (unsigned int i = sent; i < nb_tx; i++) {
            rte_pktmbuf_free(packets[i]);
        }

        if (tx_packet >= 25) {
            printf("Max Transmit Has been Reached: %d\n", tx_packet);
            break;
        }
    }

    return 0;
}
