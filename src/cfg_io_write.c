#include <stdio.h>

#include "cfg.h"
#include "cfg_io.h"

struct io_result cfg_io_write(cfg const* restrict grammar, FILE* restrict output) {
	fprintf(output, "Grammar Non-Terminals\n%s", grammar->nterms->name);
	struct cfg_sym const* const nterm_end = grammar->nterms + grammar->nterm_cnt;
	for (struct cfg_sym const* curr = grammar->nterms + 1; curr != nterm_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	fprintf(output, "\nGrammar Symbols\n%s", grammar->nterms->name);
	for (struct cfg_sym const* curr = grammar->nterms + 1; curr != nterm_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	struct cfg_sym const* const term_end = grammar->terms + grammar->term_cnt;
	for (struct cfg_sym const* curr = grammar->terms; curr != term_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	fputs(", $\n\nGrammar Rules\n", output);
	size_t counter = 1;
	for (struct cfg_sym const* curr = grammar->nterms; curr != nterm_end; ++curr) {
		struct cfg_rule const* rend = curr->nterm.rules + curr->nterm.r_cnt;
		for (struct cfg_rule const* rcur = curr->nterm.rules; rcur != rend; ++rcur) {
			fprintf(output, "(%zu)   %s ->", counter, curr->name);
			if (rcur->sym_cnt != 0) {
				struct cfg_sym* const* const send = rcur->syms + rcur->sym_cnt;
				for (struct cfg_sym* const* scur = rcur->syms; scur != send; ++scur) {
					fprintf(output, " %s", (*scur == NULL) ? "$" : (*scur)->name);
				}
			} else {
				fputs(" lambda", output);
			}
			fputc('\n', output);
			++counter;
		}
	}
	fprintf(output, "\nGrammar Start Symbol of Goal: %s\n", grammar->start->name);
	return (struct io_result){.type = RES_OK};
}


