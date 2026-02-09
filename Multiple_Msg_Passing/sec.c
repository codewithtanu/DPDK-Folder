#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <rte_eal.h>
#include <stdlib.h>
#include <time.h>

#define MSG_NAME "get_stats"

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("Error in EAL Init\n");
        return 1;
    }

    int sender_id = getpid() % 1000;
    
    printf("==============================================\n");
    printf("[SECONDARY-%d] Started\n", sender_id);
    printf("==============================================\n");

    /* Send multiple requests with delays */
    for (int i = 1; i <= 3; i++) {
        printf("\n[SECONDARY-%d] Sending request #%d to primary...\n", 
               sender_id, i);

        struct rte_mp_msg msg;
        struct rte_mp_reply reply;
        struct timespec ts = {.tv_sec = 5, .tv_nsec = 0};

        memset(&msg, 0, sizeof(msg));
        memset(&reply, 0, sizeof(reply));

        snprintf(msg.name, sizeof(msg.name), "%s", MSG_NAME);
        
        /* Include sender ID in the payload */
        char payload[256];
        snprintf(payload, sizeof(payload), 
                 "Request from SECONDARY-%d, iteration %d", 
                 sender_id, i);
        msg.len_param = strlen(payload) + 1;
        memcpy(msg.param, payload, msg.len_param);

        /* Send synchronous request */
        if (rte_mp_request_sync(&msg, &reply, &ts) < 0) {
            printf("[SECONDARY-%d] Failed to send request or receive reply\n", 
                   sender_id);
            continue;
        }

        printf("[SECONDARY-%d] Received %d replies:\n", 
               sender_id, reply.nb_received);                                           
        for (int j = 0; j < reply.nb_received; j++) {
            printf("  └─ Reply %d: %s\n", j, (char *)reply.msgs[j].param);
        }

        free(reply.msgs);

        /* Random delay between requests (1-3 seconds) */
        sleep(5);
    }

    printf("\n[SECONDARY-%d] All requests completed. Exiting.\n", sender_id);
    printf("==============================================\n");

    return 0;
}