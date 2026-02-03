#include <generic/rte_pause.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_ring.h>
#include <rte_ring_core.h>
#include <stdio.h>
#include <unistd.h>

#define RING1 "ring1"
#define RING2 "ring2"
#define RING_SIZE 1024

int main(int argc, char **argv) {

  int ret;

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    printf("ERROR in Intializing EAL\n");
    return 1;
  }

  struct rte_ring *ring1 = NULL;
  struct rte_ring *ring2 = NULL;

  ring1 = rte_ring_create(RING1, RING_SIZE, rte_socket_id(),
                          RING_F_SC_DEQ | RING_F_SP_ENQ);
  if (!ring1) {
    printf("Error In RING1 creation\n");
    return 1;
  }

  ring2 = rte_ring_create(RING2, RING_SIZE, rte_socket_id(),
                          RING_F_SC_DEQ | RING_F_SP_ENQ);
  if (!ring2) {
    printf("Error in RING2 creation\n");
    return 1;
  }

  for (int i = 0; i < 10; i++) {
    int *msg = rte_malloc(NULL, sizeof(int), 0);

    *msg = i;

    while (rte_ring_enqueue(ring1, (void *)msg)) {
      rte_pause();
    }

    printf("PRIMARY : Sent %d msg to Secondary\n", *msg);
    sleep(1);

    void *reply;
    while (rte_ring_dequeue(ring2, &reply) < 0) {
      rte_pause();
    }

    int *received = reply;

    printf("PRIMARY : got %d msg from Secondary\n", *msg);
    rte_free(received);
  }

  int *stop = rte_malloc(NULL, sizeof(int), 0);
  *stop = -1;

  rte_ring_enqueue(ring1, stop);
    
  printf("Primary Stopped.....\n");
  return 0;
}
