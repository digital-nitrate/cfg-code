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
	res = cfg_io_write(&grammar, stdout);
	cfg_free(&grammar);
	if (res.type != RES_OK) {
		fputs("Error: Could not write cfg\n", stderr);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
