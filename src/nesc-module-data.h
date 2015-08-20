// This file is NOT part of the nesC compiler.

#ifndef NESC_MODULE_DATA
#define NESC_MODULE_DATA

#include "regions.h"
#include "dd_list.h"

void get_data(dd_list modules);
void set_calls_filename(char *filename);
int get_calls_defined();

#endif
