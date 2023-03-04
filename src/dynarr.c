#include <stdlib.h>

#include "dynarr.h"

#include "cfg.h"

#define DYNARR_DEF(alias) \
	int DYNARR_INIT(alias)(DYNARR(alias)* arr, size_t sz) {\
		DYNARR_TYPE(alias)* data = malloc((sizeof *data) * sz);\
		if (data == NULL) return 1;\
		arr->data = data;\
		arr->usg = 0;\
		arr->cap = sz;\
		return 0;\
	}\
	int DYNARR_CHK(alias)(DYNARR(alias)* arr) {\
		if (arr->usg == arr->cap) {\
			size_t cap = arr->cap * 2;\
			DYNARR_TYPE(alias)* data = realloc(arr->data, (sizeof *data) * cap);\
			if (data == NULL) return 1;\
			arr->data = data;\
			arr->cap = cap;\
		}\
		return 0;\
	}\
	void DYNARR_FINI(alias)(DYNARR(alias)* arr) {\
		free(arr->data);\
	}

DYNARR_DEF(term)
DYNARR_DEF(nterm)
DYNARR_DEF(rule)
DYNARR_DEF(sid)
DYNARR_DEF(rid)
