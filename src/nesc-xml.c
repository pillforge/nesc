/* This file is part of the nesC compiler.
   Copyright (C) 2002 Intel Corporation

The attached "nesC" software is provided to you under the terms and
conditions of the GNU General Public License Version 2 as published by the
Free Software Foundation.

nesC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with nesC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <ctype.h>
#include "parser.h"
#include "nesc-xml.h"
#include "semantics.h"
#include "nesc-semantics.h"
#include "constants.h"
#include "init.h"

/* Pick an indentation amount */
#define INDENT 2

static region xml_region;
static FILE *xml_file;
static int indent_level;
static dd_list tags;
static bool at_line_start;

void xindent(void)
{
  indent_level += INDENT;
}

void xunindent(void)
{
  indent_level -= INDENT;
}

static void output_indentation(void)
{
  int i;

  for (i = 0; i < indent_level; i++) putc(' ', xml_file);
}

static void output_indent_if_needed(void)
{
  if (at_line_start)
    {
      at_line_start = FALSE;
      output_indentation();
    }
}

void xnewline(void)
{
  putc('\n', xml_file);
  at_line_start = TRUE;
}

void xstartline(void)
{
  if (!at_line_start) xnewline();
}

void xstartline_noindent(void)
{
  xstartline();
  at_line_start = FALSE;
}

void xvprintf(char *format, va_list args)
{
  output_indent_if_needed();
  vfprintf(xml_file, format, args);
}

void xprintf(char *format, ...)
{
  va_list args;

  va_start(args, format);
  xvprintf(format, args);
  va_end(args);
}

void xqputs(const char *s)
{
  /* Output a string quoted to match XML AttValue rules */
  while (*s)
    {
      switch (*s)
	{
	case '\n': case '"': case '<': case '&': 
	  xprintf("&#%d;", (unsigned char)*s);
	  break;
	default:
	  putc(*s, xml_file);
	  break;
	}
      s++;
    }
}

void xwqputs(const wchar_t *s)
{
  /* Output a wide-char string quoted to match XML AttValue rules */
  while (*s)
    {
      if ((unsigned char)*s == *s && isprint(*s) &&
	  !(*s == '"' || *s == '<' || *s == '&'))
	putc(*s, xml_file);
      else 
	xprintf("&#%d;", *s);
      s++;
    }
}


void xputs(const char *s)
{
  output_indent_if_needed();
  fputs(s, xml_file);
}


/* Leaks until xml_end. */
static void push_tag(const char *tag)
{
  dd_add_first(xml_region, tags, (char *)tag);
}

static const char *pop_tag(void)
{
  dd_list_pos top = dd_first(tags);
  const char *tag = DD_GET(const char *, top);

  dd_remove(top);

  return tag;
}

void xml_tag_start(const char *tag)
{
  push_tag(tag);
  xprintf("<%s", tag);
}

void xml_tag(const char *tag)
{
  xml_tag_start(tag);
  xml_tag_end();
}

void xml_tag_end(void)
{
  xputs(">");
}

void xml_qtag(const char *tag)
{
  xprintf("<%s/>", tag);
}

void xml_tag_end_pop(void)
{
  pop_tag();
  xputs("/>");
}

void xml_pop(void)
{
  xprintf("</%s>", pop_tag());
}

void xml_attr(const char *name, const char *val)
{
  xprintf(" %s=\"", name);
  xqputs(val);
  xputs("\"");
}

void xml_attr_int(const char *name, largest_int val)
{
  xprintf(" %s=\"%lld\"", name, val);
}

void xml_attr_ptr(const char *name, void *val)
{
  xprintf(" %s=\"%p\"", name, val);
}

void xml_attr_noval(const char *name)
{
  xprintf(" %s=\"\"", name);
}

void xml_attr_bool(const char *name, bool val)
{
  if (val)
    xml_attr_noval(name);
}

void xml_attr_cval(const char *name, cval val)
{
  bool unknown = FALSE;

  xprintf(" %s=\"", name);

  if (cval_isunsigned(val))
    xprintf("I:%llu", cval_uint_value(val));
  else if (cval_isinteger(val))
    xprintf("I:%lld", cval_sint_value(val));
  else if (cval_isunknown(val))
    unknown = TRUE;
  else if (cval_isfloating(val))
    /* XXX: hacky version */
    xprintf("F:%.20Le", cval_float_value(val));
  else if (cval_isaddress(val))
    {
      data_declaration ddecl = cval_ddecl(val);

      /* XXX: We don't (yet) support strings with an offset */
      if (ddecl && ddecl->kind == decl_magic_string && cval_knownbool(val))
	{
	  xputs("S:");
	  xwqputs(ddecl->chars);
	}
      else
	unknown = TRUE;
    }
  else if (cval_istop(val))
    xputs("V:");
  else
    unknown = TRUE;

  if (unknown)
    xputs("U:");
  putc('"', xml_file);
}


void xml_start(FILE *f)
{
  xml_region = newregion();
  xml_file = f;
  indent_level = 0;
  at_line_start = TRUE;
  tags = dd_new_list(xml_region);
}

void xml_end(void)
{
  deleteregion_ptr(&xml_region);
  xml_file = NULL;
}

/* Convenient shortcuts */

void indentedtag_start(const char *name)
{
  xstartline();
  xml_tag_start(name);
  xindent();
}

void indentedtag(const char *name)
{
  xstartline();
  xml_tag(name);
  xindent();
  xnewline();
}

void indentedtag_pop(void)
{
  xstartline();
  xunindent();
  xml_pop();
  xnewline();
}

/* Standard nesC xml elements */
void nxml_ddecl_ref(data_declaration ddecl)
{
  switch (ddecl->kind)
    {
    case decl_variable: xml_tag_start("variable-ref"); break;
    case decl_constant: xml_tag_start("constant-ref"); break;
    case decl_function: xml_tag_start("function-ref"); break;
    case decl_typedef: xml_tag_start("typedef-ref"); break;
    case decl_interface_ref: xml_tag_start("interface-ref"); break;
    case decl_component_ref: xml_tag_start("component-ref"); break;
    default: assert(0);
    }
  xml_attr("name", ddecl->name);
  if (ddecl->container || ddecl->container_function)
    xml_attr_noval("scoped");
  xml_attr_ptr("ref", ddecl);
  xml_tag_end_pop();
}

void nxml_ninstance_ref(nesc_declaration ndecl)
{
  assert (ndecl->kind == l_component);
  xml_tag_start("component-ref");
  xml_attr("name", ndecl->instance_name);
  //xml_attr_ptr("ref", ndecl);
  xml_tag_end_pop();
}

static void dump_arguments(expression arguments, dhash_table tags)
{
  expression arg;

  indentedtag("arguments");
  scan_expression (arg, arguments)
    {
      if (is_type_argument(arg))
	nxml_type(CAST(type_argument, arg)->asttype->type, tags);
      else if (arg->cst)
	{
	  xml_tag_start("constant");
	  xml_attr_cval("value", arg->cst->cval);
	  xml_tag_end_pop();
	  xnewline();
	}
      else
	{
	  xml_qtag("unknown-value");
	  xnewline();
	}
    }
  indentedtag_pop();
}

void nxml_ndefinition_ref(nesc_declaration ndecl, dhash_table defs,
			  dhash_table tags)
{
  nesc_declaration orig = original_component(ndecl);

  if (defs && !orig->dumped)
    {
      dhadd(defs, orig);
      orig->dumped = TRUE;
    }

  xstartline();
  if (orig->kind == l_interface)
    xml_tag_start("interfacedef-ref");
  else
    xml_tag_start("componentdef-ref");
  xml_attr("name", orig->name);
  if (!ndecl->arguments)
    xml_tag_end_pop();
  else
    {
      xml_tag_end();
      xindent();
      dump_arguments(ndecl->arguments, tags);
      xunindent(); xstartline(); xml_pop();
    }
  xnewline();
}

void nxml_tdecl_ref(tag_declaration tdecl)
{
  char tag[20];

  sprintf(tag, "%s-ref", tagkind_name(tdecl->kind));
  xml_tag_start(tag);
  if (tdecl->name)
    xml_attr("name", tdecl->name);
  if (tdecl->container/* || tdecl->container_function*/)
    xml_attr_noval("scoped");
  xml_attr_ptr("ref", tdecl);
  xml_tag_end_pop();
}

static void nxml_value_base(ivalue value, dhash_table tags)
{
  indentedtag_start("value-base");
  xml_attr_cval("value", value->u.base.value);
  xml_tag_end();
  nxml_type(value->type, tags);
  indentedtag_pop();
}

static void nxml_value_array(ivalue value, dhash_table tags)
{
  ivalue_array elem;

  indentedtag("value-array");
  nxml_type(value->type, tags);
  for (elem = value->u.array; elem; elem = elem->next)
    {
      indentedtag_start("array-element");
      xml_attr_int("from", elem->from);
      xml_attr_int("to", elem->to);
      xml_tag_end();
      nxml_value(elem->value, tags);
      indentedtag_pop();
    }
  indentedtag_pop();
}

static void nxml_value_structured(ivalue value, dhash_table tags)
{
  ivalue_field field;

  indentedtag("value-structured");
  nxml_type(value->type, tags);
  for (field = value->u.structured; field; field = field->next)
    {
      indentedtag_start("structured-element");
      xml_attr("field", field->field->name);
      xml_attr_ptr("ref", field->field);
      xml_tag_end();
      nxml_value(field->value, tags);
      indentedtag_pop();
    }
  indentedtag_pop();
}

void nxml_value(ivalue value, dhash_table tags)
{
  switch (value->kind)
    {
    case iv_base: nxml_value_base(value, tags); break;
    case iv_array: nxml_value_array(value, tags); break;
    case iv_structured: nxml_value_structured(value, tags); break;
    default: assert(0);
    }
}