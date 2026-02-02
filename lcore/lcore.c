#include <stdio.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_lcore.h>
#include <rte_mbuf_core.h>

/*

   __rte_unused
   
		  “I know this variable is unused, and that is intentional.”

		So:

		No warnings

		Clean compilation
*/ 
static int lcore_hello(__rte_unused void *arg){
   unsigned id=rte_lcore_id();//returns the lcore id
   printf("Hello from core %u\n",id);
   return 0;
}

int main(int argc,char** argv){
   int ret;
   unsigned lcore_id;
   
   ret=rte_eal_init(argc,argv);//initialize the eal for hugepages, lcore ,memseg,memzones
   if(ret<0){
     printf("Something went wrong\n");
     return 1;
   }
   
   printf("size of rte_mbuf %d\n", (int)sizeof(struct rte_mbuf));
   printf("EAL Consumed Args:%d\n",ret);
   lcore_hello(NULL);
   RTE_LCORE_FOREACH_WORKER(lcore_id){
                        // fun.handler, args, id
     rte_eal_remote_launch(lcore_hello,NULL,lcore_id);//runs / laucnh for each worker lcore
   }

   struct rte_node_register * x;
   
   //run for master lcore
   
   rte_eal_mp_wait_lcore();//waits for all the worker lcore
   
   return 0;
}

 
