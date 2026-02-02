#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include <rte_mempool.h>

int port_init(uint16_t port_id, struct rte_mempool *mbuf_pool);

#endif

