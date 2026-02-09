#include <stdio.h>
#include <string.h>
#include <rte_eal.h>
#include <unistd.h>
#include <stdatomic.h>

#define MSG_NAME "get_stats"

static atomic_int request_count = 0;

static int
mp_callback(const struct rte_mp_msg *msg, const void *peer)
{
    int req_num = atomic_fetch_add(&request_count, 1) + 1;
    
    printf("[PRIMARY] Request #%d received from secondary\n", req_num);
    printf("           Message: %s\n", msg->name);
    printf("           Payload: %s\n", (char *)msg->param);

    struct rte_mp_msg reply;
    memset(&reply, 0, sizeof(reply));

    snprintf(reply.name, sizeof(reply.name), "%s", MSG_NAME);

    char stats[256];
    snprintf(stats, sizeof(stats), 
             "Response#%d: packets=%d, bytes=%d", 
             req_num, req_num * 1024, req_num * 65536);
    
    reply.len_param = strlen(stats) + 1;
    memcpy(reply.param, stats, reply.len_param);

    printf("[PRIMARY] Sending reply to secondary: %s\n\n", stats);

    if (rte_mp_reply(&reply, peer) < 0) {
        printf("[PRIMARY] Failed to send reply\n");
        return -1;
    }
    
    return 0;
}

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0){
        printf("Error in EAL Init\n");
        return 1;
    }

    printf("==============================================\n");
    printf("[PRIMARY] Multi-Sender Receiver Started\n");
    printf("==============================================\n");
    printf("Registering callback for '%s'...\n\n", MSG_NAME);

    if (rte_mp_action_register(MSG_NAME, mp_callback) < 0){
        printf("Failed to register MP action\n");
        return 1;
    }

    printf("[PRIMARY] Ready to receive messages from multiple secondaries...\n");
    printf("----------------------------------------------\n\n");

    while (1)
        sleep(5);

    return 0;
}