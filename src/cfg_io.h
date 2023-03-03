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
	RES_BAD,
};

struct io_result {
	enum result_type type;
};

extern struct io_result cfg_io_read(cfg* restrict, FILE* restrict) __attribute__((nonnull, warn_unused_result));
extern struct io_result cfg_io_write(cfg const* restrict, FILE* restrict) __attribute__((nonnull, warn_unused_result));

#ifdef __cplusplus
}
#endif

#endif
