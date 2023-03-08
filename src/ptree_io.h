#ifndef PTREE_IO_H
#define PTREE_IO_H 1

#include <stdio.h>

#include "ptree.h"

extern void ptree_write(ptree const* restrict, cfg const* restrict, FILE* restrict) __attribute__((nonnull));

#endif
