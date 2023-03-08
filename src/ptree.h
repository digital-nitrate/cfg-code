#ifndef PTREE_H
#define PTREE_H 1

#include "cfg.h"

typedef struct ptree ptree;

struct ptree {
	struct ptree* con;
	cfg_sid own;
	unsigned int rule;
};

extern int ptree_bld(ptree* restrict, cfg const* restrict, unsigned int const* restrict, size_t) __attribute__((nonnull, warn_unused_result));
extern void ptree_free(ptree* restrict, cfg const* restrict) __attribute__((nonnull));

#endif
