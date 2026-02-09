#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <rte_eal.h>
#include <stdlib.h>
#include <time.h>
#include <rte_lcore.h>

#define MSG_NAME "get_stats"
#define MAX_RETRIES 3

int main(int argc, char **argv)
{
    /* Add a small random delay before initialization to avoid race conditions */
    srand(time(NULL) ^ getpid());
    usleep((rand() % 500) * 1000); // 0-500ms delay

    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("Error in EAL Init\n");
        return 1;
    }

    /* Generate unique sender ID based on process ID */
    int sender_id = getpid() % 1000;
    
    /* Get the lcore this process is running on */
    unsigned lcore_id = rte_lcore_id();
    
    printf("==============================================\n");
    printf("[SECONDARY-%d] Started on lcore %u\n", sender_id, lcore_id);
    printf("==============================================\n");

    /* Wait a bit for primary to be fully ready */
    sleep(1);

    /* Send multiple requests with delays */
    for (int i = 1; i <= 3; i++) {
        printf("\n[SECONDARY-%d] Sending request #%d to primary...\n", 
               sender_id, i);

        struct rte_mp_msg msg;
        struct rte_mp_reply reply;
        struct timespec ts = {.tv_sec = 10, .tv_nsec = 0}; // Increased timeout

        memset(&msg, 0, sizeof(msg));
        memset(&reply, 0, sizeof(reply));

        snprintf(msg.name, sizeof(msg.name), "%s", MSG_NAME);
        
        /* Include sender ID and lcore in the payload */
        char payload[256];
        snprintf(payload, sizeof(payload), 
                 "Request from SECONDARY-%d (lcore %u), iteration %d", 
                 sender_id, lcore_id, i);
        msg.len_param = strlen(payload) + 1;
        memcpy(msg.param, payload, msg.len_param);

        /* Retry logic for sending request */
        int retry = 0;
        int success = 0;
        
        while (retry < MAX_RETRIES && !success) {
            if (retry > 0) {
                printf("[SECONDARY-%d] Retry attempt %d/%d...\n", 
                       sender_id, retry, MAX_RETRIES);
                sleep(1);
            }

            if (rte_mp_request_sync(&msg, &reply, &ts) < 0) {
                printf("[SECONDARY-%d] Failed to send request (attempt %d)\n", 
                       sender_id, retry + 1);
                retry++;
            } else {
                success = 1;
            }
        }

        if (!success) {
            printf("[SECONDARY-%d] All retries failed for request #%d\n", 
                   sender_id, i);
            continue;
        }

        printf("[SECONDARY-%d] Received %d replies:\n", 
               sender_id, reply.nb_received);
        
        for (int j = 0; j < reply.nb_received; j++) {
            printf("  └─ Reply %d: %s\n", j, (char *)reply.msgs[j].param);
        }

        if (reply.msgs)
            free(reply.msgs);

        /* Random delay between requests (1-3 seconds) */
        sleep(5);
    }

    printf("\n[SECONDARY-%d] All requests completed. Exiting.\n", sender_id);
    printf("==============================================\n");

    /* Clean shutdown */
    rte_eal_cleanup();

    return 0;
}