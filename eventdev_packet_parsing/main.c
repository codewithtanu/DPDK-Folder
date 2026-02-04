#include "rte_eventdev.h"
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_mbuf_core.h>
#include <rte_mempool.h>
#include <stdint.h>
#include <stdio.h>
#include "port.h"
#include "parse.h"

int receive_handler(void __rte_unused *args) {
  int nb_rx = 0;

  struct rte_mbuf *buf[32];
  while (1) {

    int received = rte_eth_rx_burst(0, 0, buf, 32);

    if (received == 0)
      continue;

    nb_rx += received;

    for (int i = 0; i < received; i++) {

      struct rte_event event = {0};
      event.event_type = RTE_EVENT_TYPE_ETHDEV;
      event.mbuf = buf[i];
      event.op = RTE_EVENT_OP_NEW;
      event.sched_type = RTE_SCHED_TYPE_PARALLEL;
      event.queue_id = 0;
      // int ip = parsing_logic(buf[i], 1);

      // if (ip == 4) {
      //   event.queue_id = 0;
      // } else {
      //   event.queue_id = 1;
      // }

      if (rte_event_enqueue_burst(0, 0, &event, 1) <= 0) {
        rte_pause();
      }

      printf("Event enqueued\n");
    }
  }

  return nb_rx;
}

int transmit_handler(void __rte_unused *args){
   struct rte_event out;

    while (1) {
        int ret = rte_event_dequeue_burst(
            0, 1, &out, 1, 0);

        if (ret < 1)
            continue;

        struct rte_mbuf *packet = out.mbuf;

        printf("received mbuf from ipv4 queue\n");
        parsing_logic(packet, 0);
        
        // release and enqueue it back (not required in parallel)
        out.op = RTE_EVENT_OP_RELEASE;
        ret = rte_event_enqueue_burst(
            0, 1, &out, 1);
        if (ret < 1)
            printf("failed to release ipv4 event\n");


        rte_pktmbuf_free(packet);
    }
}

int drop_handler(void __rte_unused *args){
    int num_drops=0;

    struct rte_event out; 
    while(1){

        int ret=rte_event_dequeue_burst(0, 1, &out, 1, 0);

        if(ret<1){
            continue;
        }

        printf("received mbuf from ipv6 queue, now dropping\n");

        // release and queue it back (not required in parallel)
        struct rte_mbuf* packet = out.mbuf;
        out.op=RTE_EVENT_OP_RELEASE;
        ret=rte_event_enqueue_burst(0, 2, &out, 1);
        if(ret<1){
            printf("some problem while trying to enqueue the released event in ipv6 drop\n");
        }


        rte_pktmbuf_free(packet);
        num_drops++;
    }


    return num_drops;

}


int main(int argc, char **argv) {

  int ret;
  ret = rte_eal_init(argc, argv);

  if (ret < 0) {
    printf("Error in EAL Init\n");
    return 1;
  }

  port_init();

  // Configure Event Device
  struct rte_event_dev_config dev_config = {0};
  dev_config.nb_events_limit = 4096;
  dev_config.nb_event_ports = 3;
  dev_config.nb_event_queues = 2;
  dev_config.nb_event_port_dequeue_depth = 32;
  dev_config.nb_event_port_enqueue_depth = 32;
  dev_config.nb_event_queue_flows = 1024;

  if (rte_event_dev_configure(0, &dev_config) < 0) {
    printf("Error in Configuring the device\n");
    return 1;
  }

  struct rte_event_queue_conf rx_parse = {0};
  rx_parse.schedule_type = RTE_SCHED_TYPE_ATOMIC;
  rx_parse.nb_atomic_flows = 1024;
  rx_parse.nb_atomic_order_sequences = 1024;
  rx_parse.priority = RTE_EVENT_DEV_PRIORITY_NORMAL;

  if (rte_event_queue_setup(0, 0, &rx_parse) < 0)
    rte_exit(EXIT_FAILURE, "rx_parse Queue setup failed\n");

  struct rte_event_queue_conf parse_tx = {0};
  parse_tx.schedule_type = RTE_SCHED_TYPE_PARALLEL;
  parse_tx.nb_atomic_flows = 1024;
  parse_tx.nb_atomic_order_sequences = 1024;
  parse_tx.priority = RTE_EVENT_DEV_PRIORITY_NORMAL;

  if (rte_event_queue_setup(0, 1, &parse_tx) < 0)
    rte_exit(EXIT_FAILURE, "prase_tx Queue setup failed\n");

  printf("Event dev queue has been Configured\n");

  // port setup
  struct rte_event_port_conf receive = {.new_event_threshold = 1024,
                                        .dequeue_depth = 32,
                                        .enqueue_depth = 32,
                                        .event_port_cfg =
                                            RTE_EVENT_PORT_CFG_HINT_PRODUCER};

  struct rte_event_port_conf parse = {.new_event_threshold = 1024,
                                      .dequeue_depth = 32,
                                      .enqueue_depth = 32,
                                      .event_port_cfg =
                                          RTE_EVENT_PORT_CFG_HINT_CONSUMER | RTE_EVENT_PORT_CFG_HINT_PRODUCER};

  struct rte_event_port_conf transmit = {.new_event_threshold = 1024,
                                         .dequeue_depth = 32,
                                         .enqueue_depth = 32,
                                         .event_port_cfg =
                                             RTE_EVENT_PORT_CFG_HINT_CONSUMER};

  ret = rte_event_port_setup(0, 0, &receive);
  if (ret < 0) {
    printf("Error in Reecive Port Configure\n");
    return 1;
  }

  ret = rte_event_port_setup(0, 1, &parse);
  if (ret < 0) {
    printf("Error in Parse Port Configure\n");
    return 1;
  }

  ret = rte_event_port_setup(0, 2, &transmit);
  if (ret < 0) {
    printf("Error in Transmit Port Configure\n");
    return 1;
  }

  printf("Event Port Created\n");

   uint8_t ipv4[]={0};
    uint8_t prio[]={RTE_EVENT_DEV_PRIORITY_NORMAL};
    ret=rte_event_port_link(0, 1, ipv4, prio, 1);
    if(ret<0){
        printf("ipv4 port linking failed\n");
    }

    uint8_t ipv6[]={1};
    prio[0]=RTE_EVENT_DEV_PRIORITY_LOWEST;
    ret=rte_event_port_link(0, 2, ipv6, prio, 1);
    if(ret<0){
        printf("ipv6 port linking failed\n");
    }

  printf("Ports linked\n");

  if (rte_event_dev_start(0) < 0) {
    printf("Error in Starting the Event Dev\n");
    return 1;
  }

  printf("Device Started\n");

  rte_eal_remote_launch(receive_handler, NULL, 2);
  rte_eal_remote_launch(transmit_handler, NULL, 3);
  rte_eal_remote_launch(drop_handler, NULL, 4);

    unsigned lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_wait_lcore(lcore_id);
    }

  rte_event_dev_stop(0);
  rte_event_dev_close(0);

  printf("Closed Event Dev\n");

  rte_eal_cleanup();
  printf("cleanup EAL\n");

  return 0;
}