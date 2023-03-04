#ifndef DYNARR_H
#define DYNARR_H 1

#include <stddef.h>

#define DYNARR(alias) struct _dynarr_##alias
#define DYNARR_TYPE(alias) _dynarr_##alias##_type
#define DYNARR_INIT(alias) _dynarr_##alias##_init
#define DYNARR_CHK(alias) _dynarr_##alias##_chk
#define DYNARR_FINI(alias) _dynarr_##alias##_fini

#define DYNARR_DECL(type, alias) \
	typedef type DYNARR_TYPE(alias);\
	DYNARR(alias) {\
		DYNARR_TYPE(alias)* data;\
		size_t usg;\
		size_t cap;\
	};\
	extern int DYNARR_INIT(alias)(DYNARR(alias)*, size_t) __attribute__((nonnull, warn_unused_result));\
	extern int DYNARR_CHK(alias)(DYNARR(alias)*) __attribute__((nonnull, warn_unused_result));\
	extern void DYNARR_FINI(alias)(DYNARR(alias)*) __attribute__((nonnull));

#endif
