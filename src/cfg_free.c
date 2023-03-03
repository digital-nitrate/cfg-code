#include <stdlib.h>

#include "cfg.h"

void cfg_free(cfg* grammar) {
	struct cfg_sym* const nterm_end = grammar->nterms + grammar->nterm_cnt;	
	for (struct cfg_sym* curr = grammar->nterms; curr != nterm_end; ++curr) {
		free(curr->name);
		free(curr->nterm.fiset);
		struct cfg_rule* const rule_end = curr->nterm.rules + curr->nterm.r_cnt;
		for (struct cfg_rule* rcur = curr->nterm.rules; rcur != rule_end; ++rcur) {
			free(rcur->syms);	
		}
		free(curr->nterm.rules);
	}
	free(grammar->nterms);
	struct cfg_sym* const term_end = grammar->terms + grammar->term_cnt;
	for (struct cfg_sym* curr = grammar->terms; curr != term_end; ++curr) {
		free(curr->name);
	}
	free(grammar->terms);
}
