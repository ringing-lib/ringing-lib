// -*- C++ -*- psline.cpp - print out lines for methods
// Copyright (C) 2001 Martin Bright <M.Bright@dpmms.cam.ac.uk>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <ringing/common.h>

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <strstream.h>
#else
#include <iostream>
#include <strstream>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/print_ps.h>
#include <ringing/print_pdf.h>
#include <ringing/printm.h>
#include <ringing/mslib.h>
#include <ringing/cclib.h>

#if !HAVE_ARGP_PARSE
#error Sorry, at the moment you need the argp library to build this program.
#endif
#include <argp.h>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

enum format_t { ps, eps, pdf };

struct arguments {
  const char* output_file;
  const char* library_name;
  const char* method_name;
  int bells;
  format_t format;
  string title; text_style title_style;
  char *font; int font_size; colour col;
  bool custom_lines;
  map<bell, printrow::options::line_style> lines;
  bool custom_rules;
  list<pair<int,int> > rules;
  bool numbers;
  printmethod::number_mode_t number_mode;
  int pn_mode;
  int placebells;
  bool landscape;
  dimension width, height;
  bool fit;
  bool vgap_mode;
  dimension fitwidth, fitheight;
  dimension xspace, yspace;
  dimension hgap, vgap;
  int rows_per_column;
  int leads_per_column;
  int columns_per_set;
  int total_leads;
  int total_rows;
};

void parse_int(struct argp_state *state, const string& arg, int& i)
{
  istrstream is(arg.data(), arg.length()); 
  is >> i;
  if(!is || is.get() != EOF)
    argp_error(state, "Invalid integer argument: \"%s\"", arg.c_str());
}

void parse_dimension(struct argp_state *state, const string& arg, dimension& d)
{
  try {
    d.read(arg);
  }
  catch(dimension::bad_format) {
    argp_error(state, "Invalid dimension: \"%s\"", arg.c_str());
  }
}

void parse_colour(struct argp_state* state, const string& arg, colour& col)
{
  istrstream is(arg.data(), arg.length());
  int i; char c;
  is >> i;
  if(!is || ((c = is.get()) != EOF && c != '-'))
    argp_error(state, "Invalid colour: \"%s\"", arg.c_str());
  if(i < 0 || i > 100)
    argp_error(state, "Colour out of range: %d", i);
  if(is.eof()) { col.grey = true; col.red = i/100; return; }
  col.red = i/100;
  is >> i;
  if(!is || ((c = is.get()) != '-'))
    argp_error(state, "Invalid colour: \"%s\"", arg.c_str());
  if(i < 0 || i > 100)
    argp_error(state, "Colour out of range: %d", i);
  col.green = i/100;
  is >> i;
  if(!is || (is.get() != EOF))
    argp_error(state, "Invalid colour: \"%s\"", arg.c_str());
  if(i < 0 || i > 100)
    argp_error(state, "Colour out of range: %d", i);
  col.blue = i/100;
  col.grey = false;
}

string next_bit(const char*& s)
{
  const char* t = s;
  while(*t != '\0' && *t != ',') t++;
  string result(s,t);
  s = (*t == '\0') ? 0 : t+1;
  return result;
}

const char* argp_program_version = RINGING_VERSION;
static char doc[] = "psline -- print out lines for methods in PostScript.\v"
"Most options have sensible defaults.  In particular, unless you specify"
" otherwise, the image will be fitted to the page; and lines will be drawn"
" for the treble and for the bell which makes the lead end place."
"  Options requiring a DIMENSION may be specified in many different ways,"
" as for example \"1 3/8 in\", \"1/2pt\" or \"1.3cm\".  Options requiring"
" a COLOUR may be specified as either an integer between 0 and 100,"
" signifying a grey level; or as three integers between 0 and 100, separated"
" by minus signs (`-'), specifying red, green and blue levels.";
static char args_doc[] = "LIBRARY METHOD\nBELLS:PLACE-NOTATION";
static struct argp_option options[] = {
  { 0, 0, 0, 0, "General options:" },
  { "output-file", 'o', "FILE", 0, 
    "Output to FILE instead of standard output" },
  { "eps", 'e', 0, 0, "Generate an Encapsulated PostScript (EPS) file" },
  { "pdf", 'P', 0, 0, "Generate a Portable Document Format (PDF) file" },
  { "title", 't', "TITLE[,FONT[,SIZE[,COLOUR]]]", OPTION_ARG_OPTIONAL,
    "Print TITLE above the method, using the font, size and colour"
    " specified.  In the string TITLE, the character `$' stands for the"
    " full name of the method.  If no arguments are specifed, print the"
    " name of the method as the title." },
  { 0, 0, 0, 0, "Style options:" },
  { "font", 'f', "FONT", 0, "Use PostScript font FONT.  If this option"
    " is not specified, the font defaults to Helvetica." },
  { "font-size", 's', "SIZE", 0, "Use font size SIZE, in points" },
  { "colour", 'c', "COLOUR", 0, "Print everything except lines in COLOUR" },
  { "line", 'l', "BELL[x][,COLOUR[,DIMENSION]]", OPTION_ARG_OPTIONAL, 
    "Draw a line for BELL"
    " with colour COLOUR and thickness DIMENSION.  If `x' is included after"
    " BELL, draw the line only when that bell is passing another bell which"
    " has a line drawn.  If no arguments are given, don't draw any lines."
    "  This option may be used multiple times." },
  { "no-numbers", 'n', 0, 0, "Don't print numbers:  print only lines" },
  { "place-bells", 'b', "BELL|x", OPTION_ARG_OPTIONAL, "Print place bells for"
    " BELL.  If BELL is not specified, print place bells for the first"
    " working bell which has a line drawn.  If BELL is `x', don't print"
    " place bells." },
  { "place-notation", 'p', "first|all|none", OPTION_ARG_OPTIONAL, "Print place"
    " notation for the first lead, every lead, or no leads.  The default is"
    " to print place notation for the first lead." },
  { "rule", 'r', "A[,B]", OPTION_ARG_OPTIONAL, "Print rule-offs"
    " (thin horizontal lines) after the Ath change in each lead, and every B"
    " changes after that.  For example, use \"-r2,6\" for Stedman.  If no"
    " argument is given, don't draw any rule-offs."
    "  This option may be used multiple times." },
  { "miss-numbers", 'm', "always|never|lead|column", OPTION_ARG_OPTIONAL,
    "Miss out the numbers for bells which have lines drawn:  always, never,"
    " except at lead heads, or except at the beginning and end of a column."
    " If no argument is given, the default is `lead'." },
  { 0, 0, 0, 0, "Layout options:" },
  { "landscape", 'L', 0, 0, 
    "Print in landscape orientation instead of portrait (not for EPS files)" },
  { "paper-size", 'S', "DIMENSION,DIMENSION", 0, "Set the paper size to the"
    " width and height given (not for EPS files).  The default is A4." },
  { "fit", 'F', "DIMENSION,DIMENSION", OPTION_ARG_OPTIONAL, "Fit the image"
    " to the width and height specified, or to the page (less a half-inch"
    " margin) if no argument is given" },
  { "across-first", 'a', 0, 0, "Print the second lead to the right of the"
    " first, instead of below the first." },
  { "xspace", 'x', "DIMENSION", 0, "Set the horizontal distance between"
    " consecutive bells in a row to DIMENSION" },
  { "yspace", 'y', "DIMENSION", 0, "Set the vertical distance between"
    " consecutive rows to DIMENSION" },
  { "hgap", 'h', "DIMENSION", 0, "Set the horizontal gap between successive"
    " columns to DIMENSION" },
  { "vgap", 'v', "DIMENSION", 0, "Set the vertical gap between successive sets"
    " of columns to DIMENSION" },
  { "leads-per-column", 'i', "NUMBER", 0, "Print NUMBER leads per column" },
  { "rows-per-column", 'I', "NUMBER", 0, "Print NUMBER rows per column" },
  { "columns-across", 'k', "NUMBER", 0, 
    "Print NUMBER columns across the page" },
  { "total-leads", 'j', "NUMBER", 0, "Print NUMBER leads in total" },
  { "total-rows", 'J', "NUMBER", 0, "Print NUMBER rows in total" },
  { 0 }
};

static error_t parser (int key, char *arg, struct argp_state *state)
{
  arguments* args = static_cast<arguments*>(state->input);
  const char* s;
  switch(key) {
    case ARGP_KEY_ARG :
      if(state->arg_num == 0)
	args->library_name = arg;
      else if(state->arg_num == 1)
	args->method_name = arg;
      else argp_usage(state);
      break;
    case ARGP_KEY_END :
      if(state->arg_num == 0)
	argp_usage(state);
      else if(state->arg_num == 1) { // Place notation
	s = args->library_name;
	while(*s != '\0' && *s != ':') ++s;
	if(*s == '\0') argp_usage(state);
	parse_int(state, string(args->library_name, s), args->bells);
	args->library_name = s + 1;
      };
      break;
    case 'e' :
      args->format = eps;
      break;
    case 'P' :
      args->format = pdf;
      break;
    case 'n' :
      args->numbers = false;
      break;
    case 'a':
      args->vgap_mode = true;
      break;
    case 'L':
      args->landscape = true;
      break;
    case 'o' :
      args->output_file = arg;
      break;
    case 'f' :
      args->font = arg;
      break;
    case 's' :
      parse_int(state, arg, args->font_size);
      break;
    case 'i' :
      parse_int(state, arg, args->leads_per_column);
      args->rows_per_column = 0;
      break;
    case 'I' :
      parse_int(state, arg, args->rows_per_column);
      args->leads_per_column = 0;
      break;
    case 'j' :
      parse_int(state, arg, args->total_leads);
      args->total_rows = 0;
      break;
    case 'J' :
      parse_int(state, arg, args->total_rows);
      args->total_leads = 0;
      break;
    case 'k' :
      parse_int(state, arg, args->columns_per_set);
      break;
    case 'x' :
      args->fit = false;
      parse_dimension(state, arg, args->xspace);
      break;
    case 'y' :
      args->fit = false;
      parse_dimension(state, arg, args->yspace);
      break;
    case 'h' :
      parse_dimension(state, arg, args->hgap);
      break;
    case 'v' :
      parse_dimension(state, arg, args->vgap);
      break;
    case 'F' :
      args->fit = true;
      if(arg) {
	s = arg; 
	parse_dimension(state, next_bit(s), args->fitwidth);
	if(s == 0)
	  argp_error(state, "Not enough arguments: \"%s\"", arg);
	parse_dimension(state, next_bit(s), args->fitheight);
	if(s != 0)
	  argp_error(state, "Too many arguments: \"%s\"", arg);
      }
      break;
    case 'S' :
      s = arg;
      parse_dimension(state, next_bit(s), args->width);
      if(s == 0)
	argp_error(state, "Not enough arguments: \"%s\"", arg);
      parse_dimension(state, next_bit(s), args->height);
      if(s != 0)
	argp_error(state, "Too many arguments: \"%s\"", arg);
      break;
    case 'c' :
      parse_colour(state, arg, args->col);
      break;
    case 'l' :
      args->custom_lines = true;
      if(arg) {
	bell b;
	try {
	  b.from_char(*arg);
	}
	catch(bell::invalid e) {
	  argp_error(state, "Invalid bell: '%c'", *arg);
	}
	printrow::options::line_style st;
	st.width.n = 1; st.width.d = 2; st.width.u = dimension::points;
	st.col.grey = false; st.col.red = st.col.green = 0; st.col.blue = 1.0;
	st.crossing = false;
	if(arg[1] != '\0') {
	  s = arg+1; 
	  if(*s == 'x' || *s == 'X') { st.crossing = true; ++s; }
	  if(*s != '\0') {
	    if(*s++ != ',')
	      argp_error(state, "Invalid bell: \"%s\"", arg);
	    parse_colour(state, next_bit(s), st.col);
	    if(s != 0) parse_dimension(state, next_bit(s), st.width);
	    if(s != 0) argp_error(state, "Too many arguments: \"%s\"", arg);
	  }
	}
	args->lines[b] = st;
      } else
	args->lines.clear();
      break;
    case 'b' :
      if(arg) {
	if(*arg == 'X' || *arg == 'x')
	  args->placebells = -1;
	else {
	  bell b;
	  try {
	    b.from_char(*arg);
	  }
	  catch(bell::invalid e) {
	    argp_error(state, "Invalid bell: '%c'", *arg);
	  }
	  args->placebells = b;
	}
      }
      break;
    case 't' :
      if(arg) {
	s = arg;
	args->title = next_bit(s);
	if(s != 0) {
	  args->title_style.font = next_bit(s);
	  if(s != 0) {
	    parse_int(state, next_bit(s), args->title_style.size);
	    if(s != 0) {
	      parse_colour(state, next_bit(s), args->title_style.col);
	      if(s != 0)
		argp_error(state, "Too many arguments: \"%s\"", arg);
	    }
	  }
	}
      } else
	args->title = "$";
      break;
    case 'm' :
      if(arg) {
	if(arg == string("always")) 
	  args->number_mode = printmethod::miss_always;
	else if(arg == string("never")) 
	  args->number_mode = printmethod::miss_never;
	else if(arg == string("lead")) 
	  args->number_mode = printmethod::miss_lead;
	else if(arg == string("column")) 
	  args->number_mode = printmethod::miss_column;
	else
	  argp_error(state, "Unrecognised argument: \"%s\"", arg);
      } else
	args->number_mode = printmethod::miss_lead;
      break;	     
    case 'p' :
      if(arg) {
	if(arg == string("none"))
	  args->pn_mode = printmethod::pn_none;
	else if(arg == string("first"))
	  args->pn_mode = printmethod::pn_first;
	else if(arg == string("all"))
	  args->pn_mode = printmethod::pn_all;
      } else
	args->pn_mode = printmethod::pn_all;
      break;
    case 'r' :
      args->custom_rules = true;
      if(arg) {
	int a, b = 0;
	s = arg;
	parse_int(state, next_bit(s), a);
	if(s != 0) {
	  parse_int(state, next_bit(s), b);
	  if(s != 0)
	    argp_error(state, "Too many arguments: \"%s\"", arg);
	}
	args->rules.push_back(pair<int,int>(a,b));
      } else
	args->rules.clear();
      break;
    default :
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  argp our_argp = { options, parser, args_doc, doc };
  arguments args = { 0 };

  // Set up some default arguments
  args.width.n = 210; args.width.d = 1; args.width.u = dimension::mm;
  args.height.n = 297; args.height.d = 1; args.height.u = dimension::mm;
  args.col.grey = true; args.col.red = 0;
  args.placebells = -2;
  args.numbers = true;
  args.format = ps;
  args.fit = true;
  args.title_style.font = "Helvetica";
  args.title_style.size = 18;
  args.title_style.col.grey = true;
  args.title_style.col.red = 0;
  args.number_mode = printmethod::miss_lead;
  args.pn_mode = -1;

  // Parse the arguments
  argp_parse(&our_argp, argc, argv, 0, 0, &args);

  method m;

  try {
    if(args.method_name) {
      // Load the method
      mslib::registerlib();
      cclib::registerlib();
      library l(args.library_name);
      if(!l.good()) {
	cerr << argv[0] << ": Can't open library " 
	     << args.library_name << endl;
	return 1;
      }
      m = l.load(args.method_name);
    } else {
      m = method(args.library_name, args.bells);
    }

    // Set up our options
    printmethod pm(m);
    pm.defaults();

    // Set up the things which affect the fitting (and other things too)
    if(args.total_leads)
      pm.total_rows = args.total_leads * m.length();
    else if(args.total_rows)
      pm.total_rows = args.total_rows;
    if(args.custom_lines) pm.opt.lines = args.lines;
    if(args.custom_rules) pm.rules = args.rules;
    if(args.placebells == -2) {
      bell b;
      row r = m.lh();
      pm.placebells = -1;
      for(b = 0; b < m.bells(); b = b + 1)
	if(r[b] != b && pm.opt.lines.find(b) != pm.opt.lines.end()) {
	  if(pm.placebells == -1)
	    pm.placebells = b;
	  else {
	    pm.placebells = -1;
	    break;
	  }
	}
    } else
      pm.placebells = args.placebells;
    pm.opt.flags = args.numbers ? printrow::options::numbers : 0;
    if(args.pn_mode == -1)
      pm.pn_mode = args.numbers ? printmethod::pn_first : printmethod::pn_none;
    else
      pm.pn_mode = static_cast<printmethod::pn_mode_t>(args.pn_mode);
    
    // Fit to the space given
    if(args.fit && args.fitwidth == 0) {
      args.fitwidth.set_float(args.width.in_points() - 72, 1);
      args.fitheight.set_float(args.height.in_points() - 72, 1);
    }
    if(args.landscape) {
      swap(args.width, args.height);
      swap(args.fitwidth, args.fitheight);
    }
    if(!args.title.empty())
      args.fitheight.set_float(args.fitheight.in_points() 
			       - args.title_style.size * 2, 1);
    if(args.fit) pm.fit_to_space(args.fitwidth, args.fitheight, 
				 args.vgap_mode, args.numbers ? 1 : 2);

    // Set up the things which override the fitting, and everything else
    if(args.font) pm.opt.style.font = args.font;
    if(args.font_size) pm.opt.style.size = args.font_size;
    pm.opt.style.col = args.col;
    if(args.xspace != 0) pm.opt.xspace = args.xspace;
    if(args.yspace != 0) pm.opt.yspace = args.yspace;
    if(args.hgap != 0) pm.hgap = args.hgap;
    if(args.vgap != 0) pm.vgap = args.vgap;
    if(args.leads_per_column)
      pm.rows_per_column = args.leads_per_column * m.length();
    else if(args.rows_per_column)
      pm.rows_per_column = args.rows_per_column;
    if(args.columns_per_set)
      pm.columns_per_set = args.columns_per_set;
    pm.number_mode = args.number_mode;

    // Position the output correctly
    if(args.format == eps) {
      pm.xoffset.set_float(pm.opt.xspace.in_points()/2, 1); 
      pm.yoffset.set_float(pm.total_height() - pm.opt.yspace.in_points()/2, 1);
    } else {
      // Centre the output on the page
      pm.xoffset.set_float((args.width.in_points() - pm.total_width() 
			    + pm.opt.xspace.in_points())/2, 1);
      pm.yoffset.set_float((args.height.in_points() + pm.total_height() 
			    - (args.title.empty() ? 0 
			       : args.title_style.size * 2)
			    - pm.opt.yspace.in_points())/2, 1);
    }

    // Find a stream to write to
    ostream* os = &cout;
    ofstream ofs;
    if(args.output_file) { // Open the output file
      ofs.open(args.output_file, ios::binary);
      if(!ofs.good()) {
	cerr << argv[0] << ": Can't open output file " << args.output_file
	     << endl;
	return 1;
      }
      os = &ofs;
    }

    dimension titlex, titley;
    titlex.set_float(pm.xoffset.in_points() 
		     + (pm.total_width() - pm.opt.xspace.in_points())/2, 1);
    titley.set_float(pm.yoffset.in_points() + pm.opt.yspace.in_points()/2
		     + args.title_style.size, 1);
    int i = args.title.find('$');
    if(i != args.title.npos) args.title.replace(i, 1, m.fullname());

    // Create a printpage object
    printpage* pp;

    // Print the method!
    switch(args.format) {
      case eps :
	pp = new printpage_ps(*os, 0, 0, int(pm.total_width()), 
			      int(pm.total_height() 
				   + (args.title.empty() ? 0 
				      : args.title_style.size * 2)));
	break;
      case ps:
	if(args.landscape) {
	  pp = new printpage_ps(*os, args.height);
	} else {
	  pp = new printpage_ps(*os);
	} 
	break;
      case pdf:
	pp = new printpage_pdf(*os);
	break;
    }

    // Print the method!
    pp->text(args.title, titlex, titley, 
	    text_style::centre, args.title_style);
    pm.print(*pp);
    delete pp;
  }
  catch(exception& e) {
    cerr << argv[0] << ": " << e.what() << endl;
    return 1;
  }
  return 0;
}
