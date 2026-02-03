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

int main(int argc, char **argv) {

  int ret;

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    printf("ERROR in Intializing EAL\n");
    return 1;
  }

  struct rte_ring *ring1 = NULL;
  struct rte_ring *ring2 = NULL;

  while(!ring1){
    ring1=rte_ring_lookup(RING1);
    rte_pause();
  }
  printf("RING1 Found\n");

  while(!ring2){
    ring2=rte_ring_lookup(RING2);
    rte_pause();
  }
  printf("RING2 Found\n");

  for(;;){
    void *msg;

    while(rte_ring_dequeue(ring1,&msg)<0) rte_pause();
    
    int *val=msg;

    if(*val==-1){
        rte_free(val);
        printf("Secondary Stopped\n");
        break;
    }

    printf("Secondary Received %d msg from Primary\n",*val);
 
    int res=(*val)*10;
    rte_free(val);
    int *reply=rte_malloc(NULL,sizeof(int),0);
    *reply=res;
    while(rte_ring_enqueue(ring2, reply)<0) rte_pause();

    printf("Secondary : Sent %d msg to Primary\n",*reply);
  }
  return 0;
}