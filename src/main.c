#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"
#include "cfg_io.h"

#include "ptree.h"
#include "ptree_io.h"

int main(int argc, char** argv) {
	if (argc < 3) {
		fputs("Format: ./cfg-code <src> <out> [<tstream> ...]\n", stderr);
		return EXIT_FAILURE;
	}
	FILE* in = fopen(argv[1], "rb");
	if (in == NULL) {
		fprintf(stderr, "Error: Could not open \"%s\"\n", argv[1]);
		return EXIT_FAILURE;
	}
	cfg grammar;
	const_hash map;
	struct io_result res;
	res = cfg_io_read(&grammar, &map, in);
	fclose(in);
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
		free(map.bins);
		fputs("Error: Memory failure building lfifo\n", stderr);
		return EXIT_FAILURE;
	}
	FILE* out = fopen(argv[2], "wb");
	if (out == NULL) {
		cfg_free(&grammar);
		free(map.bins);
		fprintf(stderr, "Error: Could not open \"%s\"\n", argv[2]);
		return EXIT_FAILURE;
	}
	cfg_io_write(&grammar, out);
	for (int i = 3; i < argc; ++i) {
		ptree tree;
		FILE* str = fopen(argv[i], "rb");
		if (str == NULL) {
			fprintf(out, "Error: Could not open \"%s\"\n", argv[i]);
			continue;
		}
		struct tokstream toks = cfg_io_tstream(&grammar, &map, str);
		if (toks.toks == NULL) {
			fclose(str);
			fprintf(out, "Error: Could not build token stream from file \"%s\"\n", argv[i]);
			continue;
		}
		int res = ptree_bld(&tree, &grammar, toks.toks, toks.len);
		if (res) {
			fprintf(out, "Failure Building Tree For \"%s\"\n", argv[i]);
		} else {
			ptree_write(&tree, &grammar, out);
			ptree_free(&tree, &grammar);
			fputc('\n', out);
		}
		free(toks.toks);
		fclose(str);
	}
	fclose(out);
	free(map.bins);
	cfg_free(&grammar);
	return EXIT_SUCCESS;
}
