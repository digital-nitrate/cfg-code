#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "cfg.h"
#include "cfg_io.h"

#define HASH_BASE (15U)
#define RSET_BASE (1U)
#define RULE_BASE (1U)
#define TERM_BASE (1U)
#define NTERM_BASE (1U)

#define TUS_BASE (1U)
#define NUS_BASE (1U)

#define FI_BASE (1U)
#define FO_BASE (1U)

#define IFI_BASE (1U)
#define IFO_BASE (1U)

#define LAMB_BASE (1U)

#define HASH_LOAD (0.7)

#define BUFFER_SZ (128U)

static char const arrow[] = "->";
static char const mid[] = "|";
static char const eol[] = "$";
static char const lambda[] = "lambda";

struct hash_bin {
	uintmax_t hash;
	cfg_sid id;
};

struct hash_table {
	cfg grammar;
	struct hash_bin* bins;
	size_t bcnt;
};

__attribute__((nonnull, warn_unused_result)) static int hash_init(struct hash_table* table) {
	struct hash_bin* const bins = malloc((sizeof *bins) * HASH_BASE);
	if (bins == NULL) goto ErrorBins;
	if (DYNARR_INIT(term)(&(table->grammar.terms), TERM_BASE)) goto CleanBins;
	if (DYNARR_INIT(nterm)(&(table->grammar.nterms), NTERM_BASE)) goto CleanTerms;
	if (DYNARR_INIT(sid)(&(table->grammar.lambda), LAMB_BASE)) goto CleanNTerms;
	struct hash_bin* const end = bins + HASH_BASE;
	for (struct hash_bin* curr = bins; curr != end; ++curr) curr->id.id = ID_NONE;
	table->bins = bins;
	table->bcnt = HASH_BASE;
	table->grammar.start.id = ID_NONE;
	return 0;
	CleanNTerms: DYNARR_FINI(nterm)(&(table->grammar.nterms));
	CleanTerms: DYNARR_FINI(term)(&(table->grammar.terms));
	CleanBins: free(bins);
	ErrorBins: return 1;
}

__attribute__((nonnull, pure, warn_unused_result)) static uintmax_t hash_comp(char const* name, size_t len) {
	uintmax_t val = 1337U;
	char const* const end = name + len;
	for (char const* curr = name; curr != end; ++curr) {
		val = 13 * val + (uintmax_t)*curr;
	}
	return val;
}

__attribute__((nonnull, warn_unused_result)) static cfg_sid hash_ld(struct hash_table* restrict table, char const* restrict name, size_t len, unsigned char term) {
	if (table->grammar.nterms.usg + table->grammar.terms.usg >= (size_t)(HASH_LOAD * (double)table->bcnt)) {
		size_t bcnt = table->bcnt * 2 - 1;
		struct hash_bin* bins = malloc((sizeof *bins) * bcnt);
		if (bins == NULL) goto ErrorRet;
		struct hash_bin* const bend = bins + bcnt;
		for (struct hash_bin* curr = bins; curr != bend; ++curr) curr->id = (cfg_sid){.id = ID_NONE};
		struct hash_bin* const iend = table->bins + table->bcnt;
		for (struct hash_bin* curr = table->bins; curr != iend; ++curr) {
			if (curr->id.id != ID_NONE) {
				uintmax_t loc = curr->hash % bcnt;
				while (bins[loc].id.id != ID_NONE) loc = (loc + 1) % bcnt;
				bins[loc] = *curr;
			}
		}
		free(table->bins);
		table->bins = bins;
		table->bcnt = bcnt;
	}
	uintmax_t hash = hash_comp(name, len);
	uintmax_t loc = hash % table->bcnt;
	while (1) {
		if (table->bins[loc].id.id == ID_NONE) break;
		char const* nm = table->bins[loc].id.term ? table->grammar.terms.data[table->bins[loc].id.id].name : table->grammar.nterms.data[table->bins[loc].id.id].name;
		if (strcmp(nm, name) == 0) return table->bins[loc].id;
		loc = (loc + 1) % table->bcnt;
	}
	if (term) {
		if (DYNARR_CHK(term)(&(table->grammar.terms))) goto ErrorRet;
		struct cfg_term* sym = table->grammar.terms.data + table->grammar.terms.usg;
		if (DYNARR_INIT(rid)(&(sym->used), TUS_BASE)) goto ErrorRet;
		if (DYNARR_INIT(sid)(&(sym->fiset_inv), IFI_BASE)) goto ErrorTermUsed;
		if (DYNARR_INIT(sid)(&(sym->foset_inv), IFO_BASE)) goto ErrorTermFiset;
		char* ncpy = malloc(len);
		if (ncpy == NULL) goto ErrorTermFoset;
		memcpy(ncpy, name, len);
		sym->name = ncpy;
		table->bins[loc].id = (cfg_sid){.term = 1, .id = (unsigned int)table->grammar.terms.usg};
		++(table->grammar.terms.usg);
		if (0) {
			ErrorTermFoset: DYNARR_FINI(sid)(&(sym->foset_inv));
			ErrorTermFiset: DYNARR_FINI(sid)(&(sym->fiset_inv));
			ErrorTermUsed: DYNARR_FINI(rid)(&(sym->used));
			goto ErrorRet;
		}
	} else {
		if (DYNARR_CHK(nterm)(&(table->grammar.nterms))) goto ErrorRet;
		struct cfg_nterm* sym = table->grammar.nterms.data + table->grammar.nterms.usg;
		if (DYNARR_INIT(rule)(&(sym->rules), RSET_BASE)) goto ErrorRet;
		if (DYNARR_INIT(rid)(&(sym->used), NUS_BASE)) goto ErrorRules;
		if (DYNARR_INIT(sid)(&(sym->fiset), FI_BASE)) goto ErrorUsed;
		if (DYNARR_INIT(sid)(&(sym->foset), FO_BASE)) goto ErrorFiset;
		char* ncpy = malloc(len);
		if (ncpy == NULL) goto ErrorFoset;
		memcpy(ncpy, name, len);
		sym->name = ncpy;
		sym->lambda = 0;
		sym->ll1 = NULL;
		table->bins[loc].id = (cfg_sid){.term = 0, .id = (unsigned int)table->grammar.nterms.usg};
		++(table->grammar.nterms.usg);
		if (0) {
			ErrorFoset: DYNARR_FINI(sid)(&(sym->foset));
			ErrorFiset: DYNARR_FINI(sid)(&(sym->fiset));
			ErrorUsed: DYNARR_FINI(rid)(&(sym->used));
			ErrorRules: DYNARR_FINI(rule)(&(sym->rules));
			goto ErrorRet;
		}
	}
	table->bins[loc].hash = hash;
	return table->bins[loc].id;

	ErrorRet: return (cfg_sid){.id = ID_NONE};
}

#define RD_NTERM 0
#define RD_TERM 1
#define RD_EOL 2
#define RD_LAMBDA 3
#define RD_MID 4
#define RD_ARROW 5
#define RD_NL 6
#define RD_EF 7

struct size_type {
	size_t size;
	unsigned char type;
};

__attribute__((nonnull)) static struct size_type read_symbol(char* restrict buffer, size_t buffer_sz, FILE* restrict input, size_t* restrict line, size_t* restrict col) {
	struct size_type out;
	int c = fgetc(input);
	++*col;
	while (c == ' ' || c == '\t' || c == '\r') {
		c = fgetc(input);
		++*col;
	}
	if (c == EOF) {
		out.type = RD_EF;
		return out;
	} else if (c == '\n') {
		*col = 0;
		++*line;
		out.type = RD_NL;
		return out;
	}
	out.type = RD_TERM;
	out.size = 0;
	while (c != ' ' && c != '\n' && c != '\t' && c != '\r' && c != EOF && out.size != buffer_sz - 1) {
		if (c >= 'A' && c <= 'Z') out.type = RD_NTERM;
		buffer[out.size] = (char)c;
		++(out.size);
		c = fgetc(input);
		++*col;
	}
	ungetc(c, input);
	--*col;
	buffer[out.size] ='\0';
	++(out.size);
	if (out.type == RD_TERM) {
		if (memcmp(buffer, arrow, out.size) == 0) out.type = RD_ARROW;
		else if (memcmp(buffer, mid, out.size) == 0) out.type = RD_MID;
		else if (memcmp(buffer, eol, out.size) == 0) out.type = RD_EOL;
		else if (memcmp(buffer, lambda, out.size) == 0) out.type = RD_LAMBDA;
	}
	return out;
}

struct io_result cfg_io_read(cfg* restrict grammar, FILE* restrict input) {
	struct io_result result = {.type = RES_OK, .line = 1, .col = 0};
	struct hash_table table;
	if (hash_init(&table)) return (struct io_result){.type = RES_MEM};
	char buffer[BUFFER_SZ];
	struct size_type sym;
	cfg_sid active = (cfg_sid){.id = ID_NONE};
	sym.type = RD_NL;
	while (1) {
		while (sym.type == RD_NL) sym = read_symbol(buffer, BUFFER_SZ, input, &(result.line), &(result.col));
		if (sym.type == RD_EF) break;
		if (sym.size == BUFFER_SZ) {result.type=RES_FULLBUF;goto CleanTable;}
		if (sym.type == RD_NTERM) {
			active = hash_ld(&table, buffer, sym.size, 0);
			if (active.id == ID_NONE) {result.type=RES_MEM;goto CleanTable;}
			sym = read_symbol(buffer, BUFFER_SZ, input, &(result.line), &(result.col));
			if (sym.type != RD_ARROW) {result.type=RES_ARROW;goto CleanTable;}
		} else if (sym.type == RD_MID) {
			if (active.id == ID_NONE) {result.type=RES_S;goto CleanTable;}
		} else {result.type=RES_MID;goto CleanTable;}
		if (DYNARR_CHK(rule)(&(table.grammar.nterms.data[active.id].rules))) {result.type=RES_MEM;goto CleanTable;}
		struct cfg_rule* rule = table.grammar.nterms.data[active.id].rules.data + table.grammar.nterms.data[active.id].rules.usg;
		unsigned int id = (unsigned int)(rule - table.grammar.nterms.data[active.id].rules.data);
		if (DYNARR_INIT(sid)(&(rule->syms), RULE_BASE)) {result.type=RES_MEM;goto CleanTable;}
		sym = read_symbol(buffer, BUFFER_SZ, input, &(result.line), &(result.col));
		if (sym.type == RD_EF || sym.type == RD_NL || sym.type == RD_MID || sym.type == RD_ARROW) {result.type=RES_SYM;goto CleanRule;}
		if (sym.type == RD_LAMBDA) {
			if (table.grammar.nterms.data[active.id].lambda == 0) {
				if (DYNARR_CHK(sid)(&(table.grammar.lambda))) {result.type=RES_MEM;goto CleanRule;}
				table.grammar.lambda.data[table.grammar.lambda.usg] = active;
				++(table.grammar.lambda.usg);
				table.grammar.nterms.data[active.id].lambda = 1;
			}
			sym = read_symbol(buffer, BUFFER_SZ, input, &(result.line), &(result.col));
			if (sym.type != RD_EF && sym.type != RD_NL && sym.type != RD_MID) {result.type=RES_END;goto CleanRule;}
		} else {
			if (sym.type == RD_EOL) {
				if (table.grammar.start.id != ID_NONE && table.grammar.start.id != active.id) {result.type=RES_STR;goto CleanRule;}
				table.grammar.start = active;
			}
			cfg_sid con = hash_ld(&table, buffer, sym.size, sym.type == RD_NTERM ? 0 : 1);
			if (con.id == ID_NONE) {result.type=RES_MEM;goto CleanRule;}
			DYNARR(rid)* used = con.term ? &(table.grammar.terms.data[con.id].used) : &(table.grammar.nterms.data[con.id].used);
			if (DYNARR_CHK(rid)(used)) {result.type=RES_MEM;goto CleanRule;}
			used->data[used->usg] = (cfg_rid){.sid = active, .id = id, .loc = rule->syms.usg};
			++(used->usg);
			rule->syms.data[rule->syms.usg] = con;
			++(rule->syms.usg);
			while (1) {
				sym = read_symbol(buffer, BUFFER_SZ, input, &(result.line), &(result.col));
				if (sym.type == RD_EF || sym.type == RD_NL || sym.type == RD_MID) break;
				if (sym.type == RD_LAMBDA || sym.type == RD_ARROW) {result.type=RES_REP;goto CleanRule;}
				if (DYNARR_CHK(sid)(&(rule->syms))) {result.type=RES_MEM;goto CleanRule;}
				if (sym.type == RD_EOL) {
					if (table.grammar.start.id != ID_NONE && table.grammar.start.id != active.id) {result.type=RES_STR;goto CleanRule;}
					table.grammar.start = active;
				}
				cfg_sid con = hash_ld(&table, buffer, sym.size, sym.type == RD_NTERM ? 0 : 1);
				if (con.id == ID_NONE) {result.type=RES_MEM;goto CleanRule;}
				DYNARR(rid)* used = con.term ? &(table.grammar.terms.data[con.id].used) : &(table.grammar.nterms.data[con.id].used);
				if (DYNARR_CHK(rid)(used)) {result.type=RES_MEM;goto CleanRule;}
				used->data[used->usg] = (cfg_rid){.sid = active, .id = id, .loc = rule->syms.usg};
				++(used->usg);
				rule->syms.data[rule->syms.usg] = con;
				++(rule->syms.usg);
			}
		}
		++(table.grammar.nterms.data[active.id].rules.usg);
		continue;
		CleanRule: DYNARR_FINI(sid)(&(rule->syms));
		goto CleanTable;
	}
	free(table.bins);
	if (table.grammar.nterms.usg == 0 || table.grammar.start.id == ID_NONE) {result.type=RES_SLO;goto CleanGrammar;}
	struct cfg_nterm* const nend = table.grammar.nterms.data + table.grammar.nterms.usg;
	for (struct cfg_nterm* ncur = table.grammar.nterms.data; ncur != nend; ++ncur) {
		unsigned int* ll1 = malloc((sizeof *ll1) * table.grammar.terms.usg);
		if (ll1 == NULL) {result.type=RES_MEM;goto CleanGrammar;}
		unsigned int* const ll1_end = ll1 + table.grammar.terms.usg;
		for (unsigned int* lc = ll1; lc != ll1_end; ++lc) *lc = UINT_MAX;
		ncur->ll1 = ll1;
	}
	*grammar = table.grammar;
	return result;
	CleanTable: free(table.bins);
	CleanGrammar: cfg_free(&(table.grammar));
	return result;
}
