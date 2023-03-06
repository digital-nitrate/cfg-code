#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"
#include "cfg_io.h"

int main(void) {
	cfg grammar;
	struct io_result res;
	res = cfg_io_read(&grammar, stdin);
	if (res.type != RES_OK) {
		char const* cause;
		switch (res.type) {
			case RES_MEM:
				cause = "Not Enough Memory";
				break;
			case RES_FULLBUF:
				cause = "Symbol Over Max Length";
				break;
			case RES_ARROW:
				cause = "Expected \"->\"";
				break;
			case RES_MID:
				cause = "Expected \"|\"";
				break;
			case RES_SYM:
				cause = "Expected Symbol";
				break;
			case RES_END:
				cause = "Expected \"|\", \"\\n\", or EOF";
				break;
			case RES_STR:
				cause = "More Than 1 Start Symbol";
				break;
			case RES_SLO:
				cause = "No Start Symbol";
				break;
			case RES_REP:
				cause = "Expected Nonempty Symbol";
				break;
			case RES_S:
				cause = "No Associated Nonterminal";
				break;
			default:
				cause = "";
		}
		fprintf(stderr, "Error %zu,%zu: %s\n", res.line, res.col, cause);
		return EXIT_FAILURE;
	}
	if (cfg_lfifo(&grammar)) {
		cfg_free(&grammar);
		fputs("Error: Memory failure building lfifo\n", stderr);
		return EXIT_FAILURE;
	}
	cfg_io_write(&grammar, stdout);
	cfg_free(&grammar);
	return EXIT_SUCCESS;
}
