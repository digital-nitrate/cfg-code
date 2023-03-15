#ifndef CFG_IO_H
#define CFG_IO_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "cfg.h"

enum result_type {
	RES_OK,
	RES_MEM,
	RES_FULLBUF,
	RES_ARROW,
	RES_MID,
	RES_SYM,
	RES_END,
	RES_STR,
	RES_SLO,
	RES_REP,
	RES_S,
};

struct io_result {
	enum result_type type;
	size_t line;
	size_t col;
};

typedef struct const_hash {
    cfg_sid* bins;
	size_t bcnt;
} const_hash;

struct tokstream {
	cfg_sid* toks;
	size_t len;
};

extern struct io_result cfg_io_read(cfg* restrict, const_hash* restrict, FILE* restrict) __attribute__((nonnull, warn_unused_result));
extern void cfg_io_write(cfg const* restrict, FILE* restrict) __attribute__((nonnull));
extern struct tokstream cfg_io_tstream(cfg const* restrict, const_hash const* restrict, FILE* restrict) __attribute__((nonnull, warn_unused_result));

#ifdef __cplusplus
}
#endif

#endif
