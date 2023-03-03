#include "cfg.h"

void cfg_tolambda(cfg* grammar) {
	unsigned flg = 1;
	struct cfg_sym* const end = grammar->nterms + grammar->nterm_cnt;
	while (flg != 0) {
		flg = 0;
		for (struct cfg_sym* curr = grammar->nterms; curr != end; ++curr) {
			if (curr->nterm.lambda == 0) {
				struct cfg_rule* const rend = curr->nterm.rules + curr->nterm.r_cnt;
				for (struct cfg_rule* rcur = curr->nterm.rules; rcur != rend; ++rcur) {
					if (rcur->sym_cnt == 0) {
						curr->nterm.lambda = 1;
						flg = 1;
						break;
					}
					struct cfg_sym* const* const send = rcur->syms + rcur->sym_cnt;
					unsigned char inner = 1;
					for (struct cfg_sym* const* scur = rcur->syms; scur != send; ++scur) {
						if ((*scur) == NULL || (*scur)->type == CFG_T_TERM || (*scur)->nterm.lambda == 0) {
							inner = 0;
							break;
						}
					}
					if (inner != 0) {
						curr->nterm.lambda = 1;
						flg = 1;
						break;
					}
				}
			}
		}
	}
} 
