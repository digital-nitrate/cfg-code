#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "cfg.h"
#include "cfg_io.h"

void cfg_io_write(cfg const* restrict grammar, FILE* restrict output) {
	size_t mlnt = strlen(grammar->nterms.data->name);
	size_t mlt = 0;
	fprintf(output, "Grammar Non-Terminals\n%s", grammar->nterms.data->name);
	struct cfg_nterm const* const nterm_end = grammar->nterms.data + grammar->nterms.usg;
	for (struct cfg_nterm const* curr = grammar->nterms.data + 1; curr != nterm_end; ++curr) {
		fprintf(output, ", %s", curr->name);
		size_t len = strlen(curr->name);
		mlnt = len > mlnt ? len : mlnt;
	}
	fprintf(output, "\nGrammar Symbols\n%s", grammar->nterms.data->name);
	for (struct cfg_nterm const* curr = grammar->nterms.data + 1; curr != nterm_end; ++curr) {
		fprintf(output, ", %s", curr->name);
	}
	struct cfg_term const* const term_end = grammar->terms.data + grammar->terms.usg;
	for (struct cfg_term const* curr = grammar->terms.data; curr != term_end; ++curr) {
		fprintf(output, ", %s", curr->name);
		size_t len = strlen(curr->name);
		mlt = len > mlt ? len : mlt;
	}
	fputs("\n\nGrammar Rules\n", output);
	for (struct cfg_nterm const* curr = grammar->nterms.data; curr != nterm_end; ++curr) {
		struct cfg_rule const* rend = curr->rules.data + curr->rules.usg;
		for (struct cfg_rule const* rcur = curr->rules.data; rcur != rend; ++rcur) {
			fprintf(output, "(%3zu,%3zu)  %s ->", (size_t)(curr - grammar->nterms.data), (size_t)(rcur - curr->rules.data), curr->name);
			if (rcur->syms.usg != 0) {
				cfg_sid const* const send = rcur->syms.data + rcur->syms.usg;
				for (cfg_sid const* scur = rcur->syms.data; scur != send; ++scur) {
					fprintf(output, " %s", (scur->term) ? grammar->terms.data[scur->id].name : grammar->nterms.data[scur->id].name);
				}
			} else {
				fputs(" lambda", output);
			}
			fputc('\n', output);
		}
	}
	fprintf(output, "\nGrammar Start Symbol or Goal: %s\n\n", grammar->nterms.data[grammar->start.id].name);
	for (struct cfg_nterm const* curr = grammar->nterms.data; curr != nterm_end; ++curr) {
		fprintf(output, "%s:\n  DerivesToLambda: %s\n  FirstSet:", curr->name, curr->lambda ? "true" : "false");
		cfg_sid const* const fiend = curr->fiset.data + curr->fiset.usg;
		for (cfg_sid const* inn = curr->fiset.data; inn != fiend; ++inn) {
			fprintf(output, " %s", grammar->terms.data[inn->id].name);
		}
		fputs("\n  FollowSet:", output);
		cfg_sid const* const foend = curr->foset.data + curr->foset.usg;
		for (cfg_sid const* inn = curr->foset.data; inn != foend; ++inn) {
			fprintf(output, " %s", grammar->terms.data[inn->id].name);
		}
		fputc('\n', output);
	}
	fputs("\n+--", output);
	for (size_t i = 0; i < mlnt; ++i) fputc('-', output);
	fputs("++", output);
	for (size_t i = 0; i < grammar->terms.usg; ++i) {
		for (size_t j = 0; j < mlt; ++j) fputc('-', output);
		fputs("--+", output);
	}
	fprintf(output, "\n| %*s ||", (int)mlnt, "");
	for (size_t i = 0; i < grammar->terms.usg; ++i) {
		fprintf(output, " %*s |", (int)mlt, grammar->terms.data[i].name);
	}
	fputs("\n+--", output);
	for (size_t i = 0; i < mlnt; ++i) fputc('-', output);
	fputs("++", output);
	for (size_t i = 0; i < grammar->terms.usg; ++i) {
		for (size_t j = 0; j < mlt; ++j) fputc('-', output);
		fputs("--+", output);
	}
	for (size_t i = 0; i < grammar->nterms.usg; ++i) {
		fprintf(output, "\n| %*s ||", (int)mlnt, grammar->nterms.data[i].name);
		for (size_t j = 0; j < grammar->terms.usg; ++j) {
			unsigned int pt = grammar->nterms.data[i].ll1[j];
			if (pt == UINT_MAX) {
				fprintf(output, " %*s |", (int)mlt, "");
			} else if (pt == UINT_MAX - 1) {
				fprintf(output, " %*s |", (int)mlt, "*");
			} else {
				fprintf(output, " %*u |", (int)mlt, pt);
			}
		}
	}
	fputs("\n+--", output);
	for (size_t i = 0; i < mlnt; ++i) fputc('-', output);
	fputs("++", output);
	for (size_t i = 0; i < grammar->terms.usg; ++i) {
		for (size_t j = 0; j < mlt; ++j) fputc('-', output);
		fputs("--+", output);
	}
	fputc('\n', output);
}
