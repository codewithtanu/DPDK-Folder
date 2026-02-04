#ifndef PARSE_H
#define PARSE_H

#include <rte_mbuf.h>

int parsing_logic(struct rte_mbuf *m, bool mode);

#endif
