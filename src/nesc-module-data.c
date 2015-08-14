// This file is NOT part of the nesC compiler.

#include "nesc-module-data.h"

#include "regions.h"
#include "dd_list.h"
#include "nconfig.h"
#include "c-lex.h"
#include "nesc-ndoc.h"
#include "AST_types.h"
#include "nesc-decls.h"

static int debug = 1;

void process_module_for_data(nesc_declaration mod) {
	if (debug) printf("Processing module %s\n", mod->name);
}

void get_data(dd_list modules) {
	if (debug) printf("get_data()\n");
	dd_list_pos mod;
	dd_scan (mod, modules)
		process_module_for_data(DD_GET(nesc_declaration, mod));
}
