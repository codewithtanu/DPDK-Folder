#include <generic/rte_cycles.h>
#include <rte_common.h>
#include <rte_launch.h>
#include <stdio.h>
#include <rte_lcore.h>
#include <rte_eal.h>
#include <stdlib.h>
// #include <rte_delay.h>
int lcore_hello(void *arg){
   unsigned id=rte_lcore_id();//returns the lcore id
   printf("Hello from core %u\n",id);
   printf("Lcore_hello\n");
   return 0;
}

int lcore_hello1(void *arg){
   unsigned id=rte_lcore_id();//returns the lcore id
   printf("Hello from core %u\n",id);
   // void *ar=NULL;
   rte_delay_ms(5000);
   printf("Lcore_hello1\n");
      return 0;
}

int main(int argc,char** argv){

 int ret;
 ret=rte_eal_init(argc,argv);

 if(ret<0){
    rte_exit(EXIT_FAILURE,"EAL Initialization Failed\n");
 }

  int id1=2;
  int id2=3;
  printf("Main lcore : %d\n",rte_lcore_id());
  //lcore_hello(NULL);
  rte_eal_remote_launch(lcore_hello,NULL,id1);
  rte_eal_remote_launch(lcore_hello1,NULL,id2);
 // rte_delay_ms(10000);
//   rte_eal_mp_wait_lcore();
  printf("Exit Main Thread....\n");
  return 0;
}