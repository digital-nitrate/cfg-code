#ifndef CFG_H
#define CFG_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

enum cfg_type {
	CFG_T_TERM,
	CFG_T_NTERM,
};

struct cfg_rule {
	struct cfg_sym** syms;
	size_t sym_cnt;
};

struct cfg_nterm {
	struct cfg_rule* rules;
	unsigned char* fiset;
	size_t r_cnt;
	unsigned char lambda;
};

struct cfg_sym {
	char* name;
	union {
		struct cfg_nterm nterm;
	};
	enum cfg_type type;
};

typedef struct cfg {
	struct cfg_sym* nterms;
	struct cfg_sym* terms;
	struct cfg_sym* start;
	size_t nterm_cnt;
	size_t term_cnt;
} cfg;

extern void cfg_free(cfg*) __attribute__((nonnull));

#ifdef __cplusplus
}
#endif

#endif
