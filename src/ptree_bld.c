#include <stdlib.h>
#include <limits.h>

#include "cfg_io.h"
#include "ptree.h"

static cfg_sid const* ptree_bld_inner(ptree* restrict tree, cfg_sid own, cfg const* restrict grammar, cfg_sid const* restrict str, cfg_sid const* restrict end) {
	if (str == end || str->term == 0) return NULL;
	if (own.term) {
		if (str->id != own.id) return NULL;
		++str;
	} else {
		struct cfg_nterm const* const sym = &(grammar->nterms.data[own.id]);
		unsigned int rule = sym->ll1[str->id];
		if (rule == UINT_MAX || rule == UINT_MAX - 1) return NULL;
		struct cfg_rule const* const r = &(sym->rules.data[rule]);
		size_t const cnt = r->syms.usg;
		ptree* inner = malloc((sizeof *inner) * cnt);
		if (inner == NULL) return NULL;
		for (size_t i = 0; i < cnt; ++i) {
			str = ptree_bld_inner(inner + i, r->syms.data[i], grammar, str, end);
			if (str == NULL) {
				for (size_t j = 0; j < i; ++j) ptree_free(inner + j, grammar);
				free(inner);
				return NULL;
			};
		}
		tree->con = inner;
		tree->rule = rule;
	}
	tree->own = own;
	return str;
}

int ptree_bld(ptree* restrict tree, cfg const* restrict grammar, cfg_sid const* restrict str, size_t len) {
	if (ptree_bld_inner(tree, grammar->start, grammar, str, str + len) != str + len) return 1;
	return 0;
}
