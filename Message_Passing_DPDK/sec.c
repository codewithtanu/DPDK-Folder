#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <rte_eal.h>

#define MSG_NAME "tarun"

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("Error in EAL Init\n");
        return 1;
    }

    printf("[SECONDARY-] Sending request to primary...\n");

    struct rte_mp_msg msg;
    struct rte_mp_reply reply;
    struct timespec ts = {.tv_sec = 5, .tv_nsec = 0}; 

    memset(&msg, 0, sizeof(msg));
    memset(&reply, 0, sizeof(reply));

    

    snprintf(msg.name, sizeof(msg.name), "%s", MSG_NAME);

    if (rte_mp_request_sync(&msg, &reply, &ts) < 0) {
        printf("[SECONDARY] Failed to send request or receive reply\n");
        return 1;
    }

   

    sleep(10);

    printf("[SECONDARY] Received %d replies:\n", reply.nb_received);
    
    for (int i = 0; i < reply.nb_received; i++) {
        printf("  Reply %d: %s\n", i, (char *)reply.msgs[i].param);
    }

    printf("[SECONDARY] Done!\n");

    return 0;
}