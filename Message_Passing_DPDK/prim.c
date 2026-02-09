#include <rte_lcore.h>
#include <stdio.h>
#include <string.h>
#include <rte_eal.h>
#include <unistd.h>

#define MSG_NAME "tarun"

static int
mp_callback(const struct rte_mp_msg *msg, const void *peer)
{
    /*
      peer contains:
        identity of sender process
        (socket endpoint info)

        in rte_mp_msg struct param is the array and size of
        each index is 1 byte and if we store the value 4 in it 
        it will store it as raw bytes 00000100
    */
    printf("[PRIMARY] Received request: %s\n", msg->name);

    struct rte_mp_msg reply;
    memset(&reply, 0, sizeof(reply));

    snprintf(reply.name, sizeof(reply.name), "%s", MSG_NAME);

    const char *stats = "tarun";
    reply.len_param = strlen(stats) + 1;
    memcpy(reply.param, stats, reply.len_param);

    // Send reply
    if (rte_mp_reply(&reply, peer) < 0)
        printf("[PRIMARY] Failed to send reply\n");
    
    return 0;
}

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0){
        printf("Error in EAL Init\n");
        return 1;
    }

    printf("[PRIMARY] Started. Registering callback...\n");

    if (rte_mp_action_register(MSG_NAME, mp_callback) < 0){
        printf("Failed to register MP action");
        return 1;
    }
    

    printf("[PRIMARY] Waiting for messages...\n");

    while (1)
        sleep(10);

    return 0;
}


