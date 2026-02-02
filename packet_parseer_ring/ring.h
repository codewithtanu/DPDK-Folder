#ifndef RING_H
#define RING_H

#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf_core.h>
#include <rte_ring.h>
#include <stdint.h>
#include <stdio.h>
#include <rte_lcore.h>
#include <rte_ring_core.h>
#include <rte_eal.h>
#include <rte_launch.h>

#define RING_RX_NAME "RX_TO_PARSER"
#define RING_TX_NAME "PARSE_TO_TX"
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define BURST_SIZE 32
#define MEM_POOL "MY_POOL"
#define NUM_MBUF 8192 // NO OF BUF 
#define MBUF_CACHE_SIZE 250
#define RX_QUEUE 4
#define TX_QUEUE 4
#define NEW_LINE printf("\n");

int receive_packet(void *arg);
int packet_parser(void *arg);
int transmit_packet(void *arg);

#endif