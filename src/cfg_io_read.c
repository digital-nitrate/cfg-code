#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "cfg.h"
#include "cfg_io.h"

#define HASH_BASE (15U)
#define RSET_BASE (8U)
#define RULE_BASE (8U)
#define TERM_BASE (16U)
#define NTERM_BASE (16U)

#define HASH_LOAD (0.7)

#define BUFFER_SZ (128U)

static char const arrow[] = "->";
static char const mid[] = "|";
static char const eol[] = "$";
static char const lambda[] = "lambda";

struct hash_bin {
	uintmax_t hash;
	struct cfg_sym* sym;
	size_t r_cap;
};

struct hash_table {
	cfg grammar;
	struct hash_bin* bins;
	struct hash_bin* save;
	size_t bcnt;
	size_t ntcap;
	size_t tcap;
};

__attribute__((nonnull, warn_unused_result)) static int dynarr_chk_sym(size_t usg, size_t* restrict cap, struct cfg_sym** restrict data) {
	if (usg == *cap) {
		size_t ncap = *cap * 2;
		struct cfg_sym* ndata = realloc(*data, (sizeof *ndata) * ncap);
		if (ndata == NULL) return 1;
		*data = ndata;
		*cap = ncap;
	}
	return 0;
}

__attribute__((nonnull, warn_unused_result)) static int hash_init(struct hash_table* table) {
	struct hash_bin* const bins = malloc((sizeof *bins) * HASH_BASE);
	if (bins == NULL) goto ErrorBins;
	struct cfg_sym* const terms = malloc((sizeof *terms) * TERM_BASE);
	if (terms == NULL) goto ErrorTerms;
	struct cfg_sym* const nterms = malloc((sizeof *nterms) * NTERM_BASE);
	if (nterms == NULL) goto ErrorNTerms;
	struct hash_bin* const end = bins + HASH_BASE;
	for (struct hash_bin* curr = bins; curr != end; ++curr) curr->sym = NULL;
	table->bins = bins;
	table->bcnt = HASH_BASE;
	table->tcap = TERM_BASE;
	table->save = NULL;
	table->ntcap = NTERM_BASE;
	table->grammar.nterms = nterms;
	table->grammar.terms = terms;
	table->grammar.start = NULL;
	table->grammar.nterm_cnt = 0;
	table->grammar.term_cnt = 0;
	return 0;
	ErrorNTerms: free(terms);
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

__attribute__((nonnull, warn_unused_result)) static struct hash_bin* hash_ld(struct hash_table* restrict table, char const* restrict name, size_t len, enum cfg_type type) {
	if (table->grammar.nterm_cnt + table->grammar.term_cnt >= (size_t)(HASH_LOAD * (double)table->bcnt)) {
		size_t bcnt = table->bcnt * 2 - 1;
		struct hash_bin* bins = malloc((sizeof *bins) * bcnt);
		if (bins == NULL) return NULL;
		struct hash_bin* const bend = bins + bcnt;
		for (struct hash_bin* curr = bins; curr != bend; ++curr) curr->sym = NULL;
		struct hash_bin* const iend = table->bins + table->bcnt;
		struct hash_bin* nsave = NULL;
		for (struct hash_bin* curr = table->bins; curr != iend; ++curr) {
			if (curr->sym != NULL) {
				uintmax_t loc = curr->hash % bcnt;
				while (bins[loc].sym != NULL) loc = (loc + 1) % bcnt;
				bins[loc] = *curr;
				if (curr == table->save) nsave = bins + loc;
			}
		}
		free(table->bins);
		table->bins = bins;
		table->bcnt = bcnt;
		table->save = nsave;
	}
	uintmax_t hash = hash_comp(name, len);
	uintmax_t loc = hash % table->bcnt;
	while (1) {
		if (table->bins[loc].sym == NULL) break;
		if (memcmp(table->bins[loc].sym->name, name, len) == 0) return table->bins + loc;
		loc = (loc + 1) % table->bcnt;
	}
	char* ncpy = malloc(len);
	if (ncpy == NULL) return NULL;
	switch (type) {
		case CFG_T_NTERM: {
			if (dynarr_chk_sym(table->grammar.nterm_cnt, &(table->ntcap), &(table->grammar.nterms))) {
				free(ncpy);
				return NULL;
			}
			struct cfg_rule* rules = malloc((sizeof *rules) * RSET_BASE);
			if (rules == NULL) {
				free(ncpy);
				return NULL;
			}
			table->bins[loc].sym = table->grammar.nterms + table->grammar.nterm_cnt;
			table->bins[loc].sym->nterm.rules = rules;
			table->bins[loc].sym->nterm.r_cnt = 0;
			table->bins[loc].r_cap = RSET_BASE;
			++(table->grammar.nterm_cnt);
			break;
		}
		case CFG_T_TERM:
			if (dynarr_chk_sym(table->grammar.term_cnt, &(table->tcap), &(table->grammar.terms))) {
				free(ncpy);
				return NULL;
			}
			table->bins[loc].sym = table->grammar.terms + table->grammar.term_cnt;
			++(table->grammar.term_cnt);
			break;
	}
	memcpy(ncpy, name, len);
	table->bins[loc].sym->name = ncpy;
	table->bins[loc].sym->type = type;
	table->bins[loc].hash = hash;
	return table->bins + loc;
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
	sym.type = RD_NL;
	while (1) {
		if (sym.type == RD_EF) break;
		if (sym.type == RD_NL) {
			sym = read_symbol(buffer, BUFFER_SZ, input);
		}
		if (sym.type == RD_NL) continue;
		if (sym.type == RD_EF) break;
		if (sym.size == BUFFER_SZ) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		if (sym.type == RD_NTERM) {
			table.save = hash_ld(&table, buffer, sym.size, CFG_T_NTERM);
			if (table.save == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
			sym = read_symbol(buffer, BUFFER_SZ, input);
			if (sym.type != RD_ARROW) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		} else if (sym.type != RD_MID) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		if (table.save->sym->nterm.r_cnt == table.save->r_cap) {
			size_t rnew = table.save->r_cap * 2;
			struct cfg_rule* rules = realloc(table.save->sym->nterm.rules, (sizeof *rules) * rnew);
			if (rules == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
			table.save->r_cap = rnew;
			table.save->sym->nterm.rules = rules;
		}
		struct cfg_rule* rule = table.save->sym->nterm.rules + table.save->sym->nterm.r_cnt;
		struct cfg_sym** syms = malloc((sizeof *syms) * RULE_BASE);
		if (syms == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		rule->syms = syms;
		rule->sym_cnt = 0;
		size_t rule_cap = RULE_BASE;
		sym = read_symbol(buffer, BUFFER_SZ, input);
		if (sym.type == RD_EF || sym.type == RD_NL || sym.type == RD_MID || sym.type == RD_ARROW) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
		if (sym.type == RD_LAMBDA) {
			free(rule->syms);
			rule->syms = NULL;
			sym = read_symbol(buffer, BUFFER_SZ, input);
			if (sym.type != RD_EF && sym.type != RD_NL && sym.type != RD_MID) {/*TODO Fail*/}
		} else {
			if (sym.type == RD_EOL) {
				if (table.grammar.start != NULL && table.grammar.start != table.save->sym) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				table.grammar.start = table.save->sym;
				rule->syms[rule->sym_cnt] = NULL;
			} else {
				struct hash_bin* con = hash_ld(&table, buffer, sym.size, sym.type == RD_NTERM ? CFG_T_NTERM : CFG_T_TERM);
				if (con == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				rule->syms[0] = con->sym;
				++(rule->sym_cnt);
			}
			while (1) {
				sym = read_symbol(buffer, BUFFER_SZ, input);
				if (sym.type == RD_EF || sym.type == RD_NL || sym.type == RD_MID) break;
				if (sym.type == RD_LAMBDA || sym.type == RD_ARROW) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
				if (rule->sym_cnt == rule_cap) {
					rule_cap = 2 * rule_cap;
					struct cfg_sym** rs = realloc(rule->syms, (sizeof *rs) * rule_cap);
					if (rs == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
					rule->syms = rs;
				}
				if (sym.type == RD_EOL) {
					if (table.grammar.start != NULL && table.grammar.start != table.save->sym) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
					table.grammar.start = table.save->sym;
					rule->syms[rule->sym_cnt] = NULL;
				} else {
					struct hash_bin* con = hash_ld(&table, buffer, sym.size, sym.type == RD_NTERM ? CFG_T_NTERM : CFG_T_TERM);
					if (con == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
					rule->syms[rule->sym_cnt] = con->sym;
				}
				++(rule->sym_cnt);
			}
		}
		++(table.save->sym->nterm.r_cnt);
	}
	if (table.grammar.nterm_cnt == 0 || table.grammar.start == NULL) {/*TODO Fail*/ return (struct io_result){.type=RES_MEM};}
	free(table.bins);
	struct cfg_sym* const nt_end = table.grammar.nterms + table.grammar.nterm_cnt;
	for (struct cfg_sym* curr = table.grammar.nterms; curr != nt_end; ++curr) {
		unsigned char* fiset = malloc((sizeof *fiset) * table.grammar.term_cnt);
		if (fiset == NULL) {/*TODO*/}
		unsigned char* const fe = fiset + table.grammar.term_cnt;
		for (unsigned char* fc = fiset; fc != fe; ++fc) *fc = 0;
		curr->nterm.fiset = fiset;
		curr->nterm.lambda = 0;
	}
	*grammar = table.grammar;
	return (struct io_result){.type = RES_OK};
}
