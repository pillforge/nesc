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

FILE *fp;
int indent_length = 0;
int first_module = TRUE;
int first_interface;

void print_indent () {
	int i = 0;
	for (i = 0; i < indent_length; i++)
		fprintf(fp, " ");
}

void print_key (int *is_first, const char *name) {
	if (*is_first) {
		*is_first = FALSE;
	} else {
		fprintf(fp, ",\n");
	}
	print_indent();
	fprintf(fp, "\"%s\":", name);
}

void print_beg() {
	fprintf(fp, " {\n");
}
void print_end() {
	print_indent();
	fprintf(fp, "}");
}

void process_function_for_data(const char *fnname, data_declaration fndecl) {
	if (fndecl->use_summary & c_fncall) {
		if (debug) printf("    %s\n", fnname);
		dd_list_pos ause;
		dd_scan (ause, fndecl->nuses) {
			use u = DD_GET(use, ause);
			if (u && u->fn && u->fn->name && u->fn->interface && u->fn->interface->name) {
				switch(fndecl->ftype) {
				case function_event:
					printf("      Signalled by %s of %s\n", u->fn->name, u->fn->interface->name);
					break;
				case function_command:
					printf("      Called by %s of %s\n", u->fn->name, u->fn->interface->name);
					break;
				default:
					break;
				}
			}
		}
	}
}

void process_interface_for_data(const char *ifname, data_declaration idecl) {
	if (debug) printf("  %s\n", ifname);
	if (idecl->kind == decl_interface_ref) {
		env_scanner scanfns;
		const char *fnname;
		void *fnentry;
		interface_scan(idecl, &scanfns);
		print_key(&first_interface, ifname);
		print_beg();
		indent_length += 2;
		while (env_next(&scanfns, &fnname, &fnentry)) {
			process_function_for_data(fnname, fnentry);
		}
		indent_length -= 2;
		print_end();

	}
}


void process_module_for_data(nesc_declaration mod) {
//	if (debug && strcmp(mod->name, "SchedulerBasicP")) return;
	if (debug) printf("Processing module %s\n", mod->name);
	env_scanner scanifs;
	const char *ifname;
	void *ifentry;
	env_scan(mod->env->id_env, &scanifs);

	print_key(&first_module, mod->name);
	print_beg();
	first_interface = TRUE;
	indent_length += 2;
	while (env_next(&scanifs, &ifname, &ifentry)) {
		process_interface_for_data(ifname, ifentry);
	}
	indent_length -= 2;
	fprintf(fp, "\n");
	print_end();

	if (debug) printf("\n");
}

// Should be called after 'nuses' populated
void get_data(dd_list modules) {
	if (debug) printf("get_data()\n");
	dd_list_pos mod;
	fp = fopen("calls.json", "w");

	fprintf(fp, "{\n");
	indent_length += 2;
	dd_scan (mod, modules)
		process_module_for_data(DD_GET(nesc_declaration, mod));

	fprintf(fp, "\n}\n");
	fclose(fp);
}
