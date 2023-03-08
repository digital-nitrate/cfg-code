#include <stdlib.h>

#include "ptree.h"

void ptree_free(ptree* restrict tree, cfg const* restrict grammar) {
	if (tree->own.term == 0) {
		struct ptree* const end = tree->con + grammar->nterms.data[tree->own.id].rules.data[tree->rule].syms.usg;
		for (struct ptree* curr = tree->con; curr != end; ++curr) ptree_free(curr, grammar);
		free(tree->con);
	}
}
