#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"
#include "cfg_io.h"

#include "ptree.h"
#include "ptree_io.h"

#define TOKEN_ARR 128

static size_t bld_arr(unsigned int* restrict tarr, size_t tlen, char const* restrict str) {
	unsigned int* const tend = tarr + tlen;
	unsigned int* tcur = tarr;
	char const* c = str;
	unsigned char flg = 0;
	unsigned int cntr = 0;
	while (1) {
		if (*c == '\0') {
			if (flg) {
				*tcur = cntr;
				++tcur;
			}
			return (size_t)(tcur - tarr);
		}
		switch (*c) {
			case ' ':
				if (flg) {
					*tcur = cntr;
					++tcur;
					if (tcur == tend) return (size_t)(tcur - tarr);
					flg = 0;
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (flg) {
					cntr = 10 * cntr + (unsigned int)(*c - '0');	
				} else {
					flg = 1;
					cntr = (unsigned int)(*c - '0');
				}
				break;
			default:
				if (flg) {
					*tcur = cntr;
					++tcur;
				}
				return (size_t)(tcur - tarr);
		}
		++c;
	}
}

int main(int argc, char** argv) {
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
	unsigned int tokens[TOKEN_ARR];
	for (int i = 1; i < argc; ++i) {
		ptree tree;
		size_t len = bld_arr(tokens, TOKEN_ARR, argv[i]);
		int res = ptree_bld(&tree, &grammar, tokens, len);
		if (res) {
			fprintf(stdout, "Failure Building Tree For \"%s\"\n", argv[i]);
		} else {
			ptree_write(&tree, &grammar, stdout);
			ptree_free(&tree, &grammar);
			fputc('\n', stdout);
		}
	}
	cfg_free(&grammar);
	return EXIT_SUCCESS;
}
