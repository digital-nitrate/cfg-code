#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"
#include "cfg_io.h"

int main(void) {
	cfg grammar;
	struct io_result res;
	res = cfg_io_read(&grammar, stdin);
	if (res.type != RES_OK) {
		fputs("Error: Could not read cfg\n", stderr);
		return EXIT_FAILURE;
	}
	cfg_tolambda(&grammar);
	fputs("Lambda List:\n", stdout);
	struct cfg_sym const* const end = grammar.nterms + grammar.nterm_cnt;
	for (struct cfg_sym const* curr = grammar.nterms; curr != end; ++curr) {
		if (curr->nterm.lambda) {
			fprintf(stdout, "%s\n", curr->name);
		}
	}
	res = cfg_io_write(&grammar, stdout);
	cfg_free(&grammar);
	if (res.type != RES_OK) {
		fputs("Error: Could not write cfg\n", stderr);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
