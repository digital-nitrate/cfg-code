#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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
	if (DYNARR_INIT(term)(&(table->grammar.terms), TERM_BASE)) goto ErrorTerms;
	if (DYNARR_INIT(nterm)(&(table->grammar.nterms), NTERM_BASE)) goto ErrorNTerms;
	struct hash_bin* const end = bins + HASH_BASE;
	for (struct hash_bin* curr = bins; curr != end; ++curr) curr->id.id = ID_NONE;
	table->bins = bins;
	table->bcnt = HASH_BASE;
	table->grammar.start.id = ID_NONE;
	return 0;
	ErrorNTerms: DYNARR_FINI(term)(&(table->grammar.terms));
	ErrorTerms: free(bins);
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
		if (memcmp(table->bins[loc].id.term ? table->grammar.terms.data[table->bins[loc].id.id].name : table->grammar.nterms.data[table->bins[loc].id.id].name, name, len) == 0) return table->bins[loc].id;
		loc = (loc + 1) % table->bcnt;
	}
	if (term) {
		if (DYNARR_CHK(term)(&(table->grammar.terms))) goto ErrorRet;
		struct cfg_term* sym = table->grammar.terms.data + table->grammar.terms.usg;
		if (DYNARR_INIT(rid)(&(sym->used), TUS_BASE)) goto ErrorRet;
		char* ncpy = malloc(len);
		if (ncpy == NULL) goto ErrorTermUsed;
		memcpy(ncpy, name, len);
		sym->name = ncpy;
		table->bins[loc].id = (cfg_sid){.term = 1, .id = (unsigned int)table->grammar.terms.usg};
		++(table->grammar.terms.usg);
		if (0) {
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

enum rd_type {
	RD_NTERM,
	RD_TERM,
	RD_EOL,
	RD_LAMBDA,
	RD_MID,
	RD_ARROW,
	RD_NL,
	RD_EF,
};

struct size_type {
	size_t size;
	enum rd_type type;
};

__attribute__((nonnull)) static struct size_type read_symbol(char* restrict buffer, size_t buffer_sz, FILE* restrict input) {
	struct size_type out;
	int c = fgetc(input);
	while (c == ' ' || c == '\t' || c == '\r') c = fgetc(input);
	if (c == EOF) {
		out.type = RD_EF;
		return out;
	} else if (c == '\n') {
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
	}
	ungetc(c, input);
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
	struct hash_table table;
	if (hash_init(&table)) return (struct io_result){.type = RES_MEM};
	char buffer[BUFFER_SZ];
	struct size_type sym;
	cfg_sid active = (cfg_sid){.id = ID_NONE};
	sym.type = RD_NL;
	while (1) {
		while (sym.type == RD_NL) sym = read_symbol(buffer, BUFFER_SZ, input);
		if (sym.type == RD_EF) break;
		if (sym.size == BUFFER_SZ) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		if (sym.type == RD_NTERM) {
			active = hash_ld(&table, buffer, sym.size, 0);
			if (active.id == ID_NONE) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
			sym = read_symbol(buffer, BUFFER_SZ, input);
			if (sym.type != RD_ARROW) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		} else if (sym.type != RD_MID) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		if (DYNARR_CHK(rule)(&(table.grammar.nterms.data[active.id].rules))) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		struct cfg_rule* rule = table.grammar.nterms.data[active.id].rules.data + table.grammar.nterms.data[active.id].rules.usg;
		rule->owner = active;
		if (DYNARR_INIT(sid)(&(rule->syms), RULE_BASE)) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		sym = read_symbol(buffer, BUFFER_SZ, input);
		if (sym.type == RD_EF || sym.type == RD_NL || sym.type == RD_MID || sym.type == RD_ARROW) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		if (sym.type == RD_LAMBDA) {
			sym = read_symbol(buffer, BUFFER_SZ, input);
			if (sym.type != RD_EF && sym.type != RD_NL && sym.type != RD_MID) {/*TODO Fail*/}
		} else {
			if (sym.type == RD_EOL) {
				if (table.grammar.start.id != ID_NONE && table.grammar.start.id != active.id) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				table.grammar.start = active;
			}
			cfg_sid con = hash_ld(&table, buffer, sym.size, sym.type == RD_NTERM ? 0 : 1);
			if (con.id == ID_NONE) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
			rule->syms.data[0] = con;
			++(rule->syms.usg);
			while (1) {
				sym = read_symbol(buffer, BUFFER_SZ, input);
				if (sym.type == RD_EF || sym.type == RD_NL || sym.type == RD_MID) break;
				if (sym.type == RD_LAMBDA || sym.type == RD_ARROW) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				if (DYNARR_CHK(sid)(&(rule->syms))) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				if (sym.type == RD_EOL) {
					if (table.grammar.start.id != ID_NONE && table.grammar.start.id != active.id) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
					table.grammar.start = active;
				}
				cfg_sid con = hash_ld(&table, buffer, sym.size, sym.type == RD_NTERM ? 0 : 1);
				if (con.id == ID_NONE) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				rule->syms.data[rule->syms.usg] = con;
				++(rule->syms.usg);
			}
		}
		++(table.grammar.nterms.data[active.id].rules.usg);
	}
	if (table.grammar.nterms.usg == 0 || table.grammar.start.id == ID_NONE) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
	free(table.bins);
	*grammar = table.grammar;
	return (struct io_result){.type = RES_OK};
}
