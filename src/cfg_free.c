#include <stdlib.h>

#include "cfg.h"

void cfg_free(cfg* grammar) {
	struct cfg_nterm* const nterm_end = grammar->nterms.data + grammar->nterms.usg;	
	for (struct cfg_nterm* curr = grammar->nterms.data; curr != nterm_end; ++curr) {
		free(curr->name);
		DYNARR_FINI(sid)(&(curr->fiset));
		DYNARR_FINI(sid)(&(curr->foset));
		DYNARR_FINI(rid)(&(curr->used));
		struct cfg_rule* const rule_end = curr->rules.data + curr->rules.usg;
		for (struct cfg_rule* rcur = curr->rules.data; rcur != rule_end; ++rcur) {
			DYNARR_FINI(sid)(&(rcur->syms));
		}
		DYNARR_FINI(rule)(&(curr->rules));
	}
	DYNARR_FINI(nterm)(&(grammar->nterms));
	struct cfg_term* const term_end = grammar->terms.data + grammar->terms.usg;
	for (struct cfg_term* curr = grammar->terms.data; curr != term_end; ++curr) {
		free(curr->name);
		DYNARR_FINI(rid)(&(curr->used));
		DYNARR_FINI(sid)(&(curr->fiset_inv));
	}
	DYNARR_FINI(term)(&(grammar->terms));
	DYNARR_FINI(sid) (&(grammar->lambda));
}
