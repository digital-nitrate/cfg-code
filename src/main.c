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
	if (cfg_lfiso(&grammar)) {
		cfg_free(&grammar);
		fputs("Error: Could not handle cfg\n", stderr);
		return EXIT_FAILURE;
	}
	cfg_io_write(&grammar, stdout);
	cfg_free(&grammar);
	return EXIT_SUCCESS;
}
