#include <stdio.h>

#include "ptree.h"
#include "ptree_io.h"

void ptree_write(ptree const* restrict tree, cfg const* restrict grammar, FILE* restrict output) {
	if (tree->own.term) {
		fputs(grammar->terms.data[tree->own.id].name, output);
	} else {
		struct cfg_nterm const* const sym = &(grammar->nterms.data[tree->own.id]);
		fprintf(output, "%s(", sym->name);
		size_t const usg = sym->rules.data[tree->rule].syms.usg;
		if (usg != 0) {
			struct ptree const* const end = tree->con + usg;
			ptree_write(tree->con, grammar, output);
			for (struct ptree const* curr = tree->con + 1; curr != end; ++curr) {
				fputc(',', output);
				ptree_write(curr, grammar, output);
			}
		}
		fputc(')', output);
	}
}
