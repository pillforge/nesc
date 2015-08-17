// This file is NOT part of the nesC compiler.

#include "nesc-module-data.h"

// include list is copied from 'nesc-component.c'
#include "parser.h"
#include "nesc-component.h"
#include "nesc-semantics.h"
#include "nesc-interface.h"
#include "nesc-configuration.h"
#include "nesc-module.h"
#include "nesc-decls.h"
#include "nesc-paths.h"
#include "nesc-env.h"
#include "nesc-cg.h"
#include "semantics.h"
#include "c-parse.h"
#include "edit.h"
#include "nesc-abstract.h"
#include "attributes.h"
#include <stdio.h>

//#include "regions.h"
//#include "dd_list.h"
//#include "nconfig.h"
//#include "c-lex.h"
//#include "nesc-ndoc.h"
//#include "AST_types.h"
//#include "nesc-decls.h"
//#include "env.h"
//#include <stdio.h>

static int debug = 1;

void process_module_for_data(nesc_declaration mod) {
	if (debug && strcmp(mod->name, "BlinkC")) return;
	if (debug) printf("Processing module %s\n", mod->name);

	env_scanner scanifs;
	const char *ifname;
	void *ifentry;
	env_scan(mod->env->id_env, &scanifs);

	while (env_next(&scanifs, &ifname, &ifentry)) {
		if (debug) printf("  %s\n", ifname);
		data_declaration idecl = ifentry;

		if (idecl->kind == decl_interface_ref) {
			env_scanner scanfns;
			const char *fnname;
			void *fnentry;
			interface_scan(idecl, &scanfns);

			while (env_next(&scanfns, &fnname, &fnentry)) {
				data_declaration fndecl = fnentry;
				if (fndecl->use_summary & c_fncall) {
					if (debug) printf("    %s\n", fnname);
					dd_list_pos ause;
					dd_scan (ause, fndecl->nuses) {
						use u = DD_GET(use, ause);
					}
				}
			}
		}
	}

	if (debug) printf("\n");
}

// Should be called after 'nuses' populated
void get_data(dd_list modules) {
	if (debug) printf("get_data()\n");
	dd_list_pos mod;
	dd_scan (mod, modules)
		process_module_for_data(DD_GET(nesc_declaration, mod));
}
