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
};

struct io_result {
	enum result_type type;
	size_t line;
	size_t col;
};

extern struct io_result cfg_io_read(cfg* restrict, FILE* restrict) __attribute__((nonnull, warn_unused_result));
extern void cfg_io_write(cfg const* restrict, FILE* restrict) __attribute__((nonnull));

#ifdef __cplusplus
}
#endif

#endif
