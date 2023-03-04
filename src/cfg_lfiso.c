#include "cfg.h"

int cfg_lfiso(cfg* grammar) {
	(void)grammar;
	size_t curr = 0;
	while (curr != grammar->lambda.usg) {
		struct cfg_nterm* sym = grammar->nterms.data + grammar->lambda.data[curr].id;
		cfg_rid const* const rend = sym->used.data + sym->used.usg;
		for (cfg_rid const* rcur = sym->used.data; rcur != rend; ++rcur) {
			struct cfg_nterm* apl = grammar->nterms.data + rcur->sid.id;
			if (apl->lambda == 0) {
				struct cfg_rule const* rule = apl->rules.data + rcur->id;
				cfg_sid const* const send = rule->syms.data + rule->syms.usg;
				cfg_sid const* scur = rule->syms.data;
				while (scur != send) {
					if (scur->term || grammar->nterms.data[scur->id].lambda == 0) break;
					++scur;
				}
				if (scur == send) {
					if (DYNARR_CHK(sid)(&(grammar->lambda))) return 1;
					grammar->lambda.data[grammar->lambda.usg] = rcur->sid;
					++(grammar->lambda.usg);
					apl->lambda = 1;
				}
			}
		}
		++curr;
	}
	return 0;
}
