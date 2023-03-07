#include <limits.h>

#include "cfg.h"

__attribute__((nonnull, warn_unused_result)) static int bld_lambda(cfg* grammar) {
	struct cfg_nterm const* const nend = grammar->nterms.data + grammar->nterms.usg;
	for (struct cfg_nterm const* ncur = grammar->nterms.data; ncur != nend; ++ncur) {
		struct cfg_rule* const rend = ncur->rules.data + ncur->rules.usg;
		for (struct cfg_rule* rcur = ncur->rules.data; rcur != rend; ++rcur) {
			rcur->tmp = 0;
		}
	}
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
			rule->tmp = (size_t)(scur - rule->syms.data);
			if (scur == send) {
				if (DYNARR_CHK(sid)(&(grammar->lambda))) return 1;
				grammar->lambda.data[grammar->lambda.usg] = rcur->sid;
				++(grammar->lambda.usg);
				apl->lambda = 1;
			}
		}
		++curr;
	}
	return 0;
}

__attribute__((nonnull, warn_unused_result)) static int bld_fiset_unused(cfg const* grammar, struct cfg_term* tcur, DYNARR(rid) const* used, unsigned int id) {
	cfg_rid const* const uend = used->data + used->usg;
	for (cfg_rid const* ucur = used->data; ucur != uend; ++ucur) {
		struct cfg_nterm* const lhs = grammar->nterms.data + ucur->sid.id;
		struct cfg_rule const* const rhs = lhs->rules.data + ucur->id;
		if (ucur->loc > rhs->tmp) continue;
		unsigned int pt = lhs->ll1[id];
		if (pt == UINT_MAX) {
			lhs->ll1[id] = ucur->id;
		} else if (pt != ucur->id) {
			lhs->ll1[id] = UINT_MAX - 1;
		}
		if (lhs->fiset.usg != 0 && lhs->fiset.data[lhs->fiset.usg - 1].id == id) continue;
		if (DYNARR_CHK(sid)(&(tcur->fiset_inv))) return 1;
		if (DYNARR_CHK(sid)(&(lhs->fiset))) return 1;
		tcur->fiset_inv.data[tcur->fiset_inv.usg] = ucur->sid;
		++(tcur->fiset_inv.usg);
		lhs->fiset.data[lhs->fiset.usg] = (cfg_sid){.term = 1, .id = id};
		++(lhs->fiset.usg);
	}
	return 0;
}

__attribute__((nonnull, warn_unused_result)) static int bld_foset_trav(cfg const* grammar, struct cfg_term* tcur, unsigned int id, cfg_sid const* rend, cfg_sid const* rcur) {
	while (rcur != rend && rcur->term == 0) {
		DYNARR(sid)* foset = &(grammar->nterms.data[rcur->id].foset);
		if (foset->usg == 0 || foset->data[foset->usg - 1].id != id) {
			if (DYNARR_CHK(sid)(&(tcur->foset_inv))) return 1;
			if (DYNARR_CHK(sid)(foset)) return 1;
			tcur->foset_inv.data[tcur->foset_inv.usg] = *rcur;
			++(tcur->foset_inv.usg);
			foset->data[foset->usg] = (cfg_sid){.term = 1, .id = id};
			++(foset->usg);
		}
		if ((grammar->nterms.data[rcur->id].fiset.usg != 0 && grammar->nterms.data[rcur->id].fiset.data[grammar->nterms.data[rcur->id].fiset.usg - 1].id == id) || grammar->nterms.data[rcur->id].lambda == 0) break;
		--rcur;
	}
	return 0;
}

__attribute__((nonnull, warn_unused_result)) static int bld_foset_unused(cfg const* grammar, struct cfg_term* tcur, DYNARR(rid) const* used, unsigned int id) {
	cfg_rid const* const uend = used->data + used->usg;
	for (cfg_rid const* ucur = used->data; ucur != uend; ++ucur) {
		if (ucur->loc == 0) continue;
		struct cfg_nterm const* const lhs = grammar->nterms.data + ucur->sid.id;
		struct cfg_rule const* const rhs = lhs->rules.data + ucur->id;
		if (bld_foset_trav(grammar, tcur, id, rhs->syms.data - 1, rhs->syms.data + ucur->loc - 1)) return 1;
	}
	return 0;
}

__attribute__((nonnull, warn_unused_result)) static int bld_fifosets(cfg* grammar) {
	struct cfg_nterm const* const nend = grammar->nterms.data + grammar->nterms.usg;
	for (struct cfg_nterm const* ncur = grammar->nterms.data; ncur != nend; ++ncur) {
		struct cfg_rule* const rend = ncur->rules.data + ncur->rules.usg;
		for (struct cfg_rule* rcur = ncur->rules.data; rcur != rend; ++rcur) {
			cfg_sid const* const lend = rcur->syms.data + rcur->syms.usg;
			cfg_sid const* loc = rcur->syms.data + rcur->tmp;
			while (loc != lend && loc->term == 0 && grammar->nterms.data[loc->id].lambda != 0) ++loc;
			rcur->tmp = (size_t)(loc - rcur->syms.data);
		}
	}
	struct cfg_term const* const tend = grammar->terms.data + grammar->terms.usg;
	for (struct cfg_term* tcur = grammar->terms.data; tcur != tend; ++tcur) {
		unsigned int id = (unsigned int)(tcur - grammar->terms.data);
		if (bld_fiset_unused(grammar, tcur, &(tcur->used), id)) return 1;
		for (size_t curr = 0; curr != tcur->fiset_inv.usg; ++curr) {
			if (bld_fiset_unused(grammar, tcur, &(grammar->nterms.data[tcur->fiset_inv.data[curr].id].used), id)) return 1;
		}
		if (bld_foset_unused(grammar, tcur, &(tcur->used), id)) return 1;
		cfg_sid const* const fiend = tcur->fiset_inv.data + tcur->fiset_inv.usg;
		for (cfg_sid const* ficur = tcur->fiset_inv.data; ficur != fiend; ++ficur) {
			if (bld_foset_unused(grammar, tcur, &(grammar->nterms.data[ficur->id].used), id)) return 1;
		}
		for (size_t curr = 0; curr != tcur->foset_inv.usg; ++curr) {
			struct cfg_nterm const* const sym = &(grammar->nterms.data[tcur->foset_inv.data[curr].id]);
			struct cfg_rule const* const aend = sym->rules.data + sym->rules.usg;
			for (struct cfg_rule const* acur = sym->rules.data; acur != aend; ++acur) {
				if (acur->tmp == acur->syms.usg) {
					if (sym->ll1[id] == UINT_MAX) {
						sym->ll1[id] = (unsigned int)(acur - sym->rules.data);
					} else if (sym->ll1[id] != (unsigned int)(acur - sym->rules.data)) {
						sym->ll1[id] = UINT_MAX - 1;
					}
				}
				if (bld_foset_trav(grammar, tcur, id, acur->syms.data - 1, acur->syms.data + acur->syms.usg - 1)) return 1;
			}
		}
	}
	return 0;
}

int cfg_lfifo(cfg* grammar) {
	if (bld_lambda(grammar)) return 1;
	if (bld_fifosets(grammar)) return 1;
	return 0;
}
