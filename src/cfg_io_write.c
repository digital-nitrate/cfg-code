#include <stdio.h>

#include "cfg.h"
#include "cfg_io.h"

void cfg_io_write(cfg const* restrict grammar, FILE* restrict output) {
	fprintf(output, "Grammar Non-Terminals\n%s", grammar->nterms.data->name);
	struct cfg_nterm const* const nterm_end = grammar->nterms.data + grammar->nterms.usg;
	for (struct cfg_nterm const* curr = grammar->nterms.data + 1; curr != nterm_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	fprintf(output, "\nGrammar Symbols\n%s", grammar->nterms.data->name);
	for (struct cfg_nterm const* curr = grammar->nterms.data + 1; curr != nterm_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	struct cfg_term const* const term_end = grammar->terms.data + grammar->terms.usg;
	for (struct cfg_term const* curr = grammar->terms.data; curr != term_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	fputs("\n\nGrammar Rules\n", output);
	size_t counter = 1;
	for (struct cfg_nterm const* curr = grammar->nterms.data; curr != nterm_end; ++curr) {
		struct cfg_rule const* rend = curr->rules.data + curr->rules.usg;
		for (struct cfg_rule const* rcur = curr->rules.data; rcur != rend; ++rcur) {
			fprintf(output, "(%zu)\t%s ->", counter, curr->name);
			if (rcur->syms.usg != 0) {
				cfg_sid const* const send = rcur->syms.data + rcur->syms.usg;
				for (cfg_sid const* scur = rcur->syms.data; scur != send; ++scur) {
					fprintf(output, " %s", (scur->term) ? grammar->terms.data[scur->id].name : grammar->nterms.data[scur->id].name);
				}
			} else {
				fputs(" lambda", output);
			}
			fputc('\n', output);
			++counter;
		}
	}
	fprintf(output, "\nGrammar Start Symbol or Goal: %s\n\n", grammar->nterms.data[grammar->start.id].name);
	for (struct cfg_nterm const* curr = grammar->nterms.data; curr != nterm_end; ++curr) {
		fprintf(output, "%s:\n\tDerivesToLambda: %s\n\tFirstSet:", curr->name, curr->lambda ? "true" : "false");
		cfg_sid const* const fiend = curr->fiset.data + curr->fiset.usg;
		for (cfg_sid const* inn = curr->fiset.data; inn != fiend; ++inn) {
			fprintf(output, " %s", grammar->terms.data[inn->id].name);
		}
		fputs("\n\tFollowSet:", output);
		cfg_sid const* const foend = curr->foset.data + curr->foset.usg;
		for (cfg_sid const* inn = curr->foset.data; inn != foend; ++inn) {
			fprintf(output, " %s", grammar->terms.data[inn->id].name);
		}
		fputc('\n', output);
	}
}
