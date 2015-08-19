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
int first_function;
int first_caller;
int first_decl;

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

void print_a_beg() {
	fprintf(fp, " [\n");
}

void print_end(int new_line) {
	if (new_line)
		fprintf(fp, "\n");
	print_indent();
	fprintf(fp, "}");
}

void print_a_end() {
	fprintf(fp, "\n");
	print_indent();
	fprintf(fp, "]");
}

void print_a_caller(char *type, const char *iname, const char *fname) {
	if (first_caller) {
		first_caller = FALSE;
	} else {
		fprintf(fp, ",\n");
	}
	print_indent();
	fprintf(fp, "[\"%s\", \"%s\", \"%s\"]", type, iname, fname);
}

void process_function_for_data(const char *fnname, data_declaration fndecl) {
	if (fndecl->use_summary & c_fncall) {
		if (debug) printf("    %s\n", fnname);
		dd_list_pos ause;
		print_key(&first_function, fnname);
		print_a_beg();
		indent_length += 2;
		first_caller = TRUE;
		dd_scan (ause, fndecl->nuses) {
			use u = DD_GET(use, ause);
			if (u && u->fn && u->fn->name && u->fn->interface && u->fn->interface->name) {
				char *ftype;
				switch(fndecl->ftype) {
				case function_event:
					if (debug) printf("      Signalled by %s of %s\n", u->fn->name, u->fn->interface->name);
					ftype = "signal";
					break;
				case function_command:
					if (debug) printf("      Called by %s of %s\n", u->fn->name, u->fn->interface->name);
					ftype = "call";
					break;
				default:
					break;
				}
				print_a_caller(ftype, u->fn->interface->name, u->fn->name);
			}
		}
		indent_length -= 2;
		print_a_end();
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
		first_function = TRUE;
		while (env_next(&scanfns, &fnname, &fnentry)) {
			process_function_for_data(fnname, fnentry);
		}
		indent_length -= 2;
		print_end(TRUE);

	}
}

void process_declaration(declaration d) {
	if (d->kind == kind_data_decl) {
		data_decl dd = (data_decl)d;
		declaration vd;
		scan_declaration (vd, dd->decls) {
			variable_decl vdd = CAST(variable_decl, vd);
			data_declaration vdecl = vdd->ddecl;
			if (debug) printf("  %s\n", vdecl->name);
			print_key(&first_decl, vdecl->name);
			print_beg();
			print_end(FALSE);
		}
	}
}

void process_module_for_variables (const char *name, declaration dlist) {
	declaration d;
	char key[strlen(name) + 12];
	strcpy(key, name);
	strcat(key, "__variables");
	print_key(&first_interface, key);
	print_beg();
	indent_length += 2;
	scan_declaration (d, dlist)
		process_declaration(d);
	indent_length -= 2;
	print_end(TRUE);
}

void process_module_for_data(nesc_declaration mod) {
//	if (debug && strcmp(mod->name, "BlinkC")) return;
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
	first_decl = TRUE;
	process_module_for_variables(mod->name, CAST(module, mod->impl)->decls);
	indent_length -= 2;
	print_end(TRUE);

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
