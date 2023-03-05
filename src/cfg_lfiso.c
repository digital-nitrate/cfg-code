#include "cfg.h"

__attribute__((nonnull)) static void init_tmp(cfg* grammar) {
	struct cfg_nterm const* const nend = grammar->nterms.data + grammar->nterms.usg;
	for (struct cfg_nterm const* ncur = grammar->nterms.data; ncur != nend; ++ncur) {
		struct cfg_rule* const rend = ncur->rules.data + ncur->rules.usg;
		for (struct cfg_rule* rcur = ncur->rules.data; rcur != rend; ++rcur) {
			rcur->tmp = 0;
		}
	}
}

__attribute__((nonnull, warn_unused_result)) static int bld_lambda(cfg* grammar) {
	init_tmp(grammar);
	size_t curr = 0;
	while (curr != grammar->lambda.usg) {
		struct cfg_nterm const* const sym = grammar->nterms.data + grammar->lambda.data[curr].id;
		cfg_rid const* const rend = sym->used.data + sym->used.usg;
		for (cfg_rid const* rcur = sym->used.data; rcur != rend; ++rcur) {
			struct cfg_nterm* const apl = grammar->nterms.data + rcur->sid.id;
			if (apl->lambda != 0) continue;
			struct cfg_rule* const rule = apl->rules.data + rcur->id;
			if (rcur->loc < rule->tmp) continue;
			cfg_sid const* const send = rule->syms.data + rule->syms.usg;
			cfg_sid const* scur = rule->syms.data + rule->tmp;
			while (scur != send && scur->term == 0 && grammar->nterms.data[scur->id].lambda != 0) ++scur;
			if (scur == send) {
				if (DYNARR_CHK(sid)(&(grammar->lambda))) return 1;
				grammar->lambda.data[grammar->lambda.usg] = rcur->sid;
				++(grammar->lambda.usg);
				apl->lambda = 1;
			} else {
				rule->tmp = (size_t)(scur - rule->syms.data);
			}
		}
		++curr;
	}
	return 0;
}

__attribute__((unused, nonnull, warn_unused_result)) static int bld_fiset(cfg* grammar) {
	struct cfg_term const* const tend = grammar->terms.data + grammar->terms.usg;
	for (struct cfg_term const* tcur = grammar->terms.data; tcur != tend; ++tcur) {
		init_tmp(grammar);
	}
	return 0;
}

int cfg_lfiso(cfg* grammar) {
	return bld_lambda(grammar);
}
