#ifndef CFG_H
#define CFG_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "dynarr.h"

#define ID_NONE ((1U << 31) - 1)

typedef struct {
	unsigned int term : 1, id : 31;
} cfg_sid;
DYNARR_DECL(cfg_sid, sid)

typedef struct {
	cfg_sid sid;
	unsigned int id;
	size_t loc;
} cfg_rid;
DYNARR_DECL(cfg_rid, rid)

struct cfg_rule {
	DYNARR(sid) syms;
	size_t tmp;
};
DYNARR_DECL(struct cfg_rule, rule)

struct cfg_nterm {
	DYNARR(rule) rules;
	DYNARR(rid) used;
	DYNARR(sid) fiset;
	DYNARR(sid) foset;
	char* name;
	unsigned char lambda;
};
DYNARR_DECL(struct cfg_nterm, nterm)

struct cfg_term {
	DYNARR(rid) used;
	char* name;
};
DYNARR_DECL(struct cfg_term, term)

typedef struct cfg {
	DYNARR(nterm) nterms;
	DYNARR(term) terms;
	DYNARR(sid) lambda;
	cfg_sid start;
} cfg;

extern void cfg_free(cfg*) __attribute__((nonnull));

extern int cfg_lfiso(cfg*) __attribute__((nonnull, warn_unused_result));

#ifdef __cplusplus
}
#endif

#endif
