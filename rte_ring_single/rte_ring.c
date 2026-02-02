#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_ring_core.h>
#include <stdio.h>
#include <rte_ring.h>
#include <rte_mbuf.h>

#define RING_SIZE 1024
#define RING_NAME "my_ring"

struct rte_ring * rte_ring1=NULL;

void create_ring(){
  
    rte_ring1=rte_ring_create(RING_NAME,RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);
    
    if(!rte_ring1){
        printf("Ring Creation Failed\n");
        return;
    }
    

    printf("Ring Created Successfully\n");

}

int main(int argc,char** argv){
   int ret;
   ret=rte_eal_init(argc,argv);
   if(ret<0){
    printf("EAL Initialization Failed\n");
    return 1;
   }
   printf("Initialization Successfull\n");
   create_ring();
  
   int data=2;
   
   ret=rte_ring_enqueue(rte_ring1, &data);
   
   if(ret<0){
    printf("error in queuing the packet failed\n");
    return 1;
   }
    
   printf("enqueued the packet : %d\n",data);
   int *out_data;

   ret=rte_ring_dequeue(rte_ring1, (void**)&out_data);

   if(ret<0){
    printf("error in dequeuing the packet failed\n");
    return 1;
   }
   
   printf("Dequeued the packet : %d\n",*out_data);


   return 0;
}