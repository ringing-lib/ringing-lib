// -*- C++ -*- psline.cpp - print out lines for methods
// Copyright (C) 2001-2 Martin Bright <M.Bright@dpmms.cam.ac.uk>

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
#include <ringing/printm.h>
#include <ringing/print_ps.h>
#include <ringing/print_pdf.h>
#include <ringing/mslib.h>
#include <ringing/cclib.h>
#include "args.h"

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

enum format_t { ps, eps, pdf };

struct arguments {
  string output_file;
  string library_name;
  string method_name;
  int bells;
  format_t format;
  string title; text_style title_style;
  string font; int font_size; colour col;
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
  int sets_per_page;
  int total_leads;
  int total_rows;
};

arguments args;
string progname;

bool parse_int(const string& arg, int& i)
{
  istrstream is(arg.data(), arg.length()); 
  is >> i;
  if(!is || is.get() != EOF) {
    cerr << "Invalid integer argument: \"" << arg << "\"\n";
    return false;
  }
  return true;
}

bool parse_dimension(const string& arg, dimension& d)
{
  try {
    d.read(arg);
  }
  catch(dimension::bad_format) {
    cerr << "Invalid dimension: \"" << arg << "\"\n";
    return false;
  }
  return true;
}

bool parse_colour(const string& arg, colour& col)
{
  istrstream is(arg.data(), arg.length());
  int i; char c;
  is >> i;
  if(!is || ((c = is.get()) != EOF && c != '-')) {
    cerr << "Invalid colour: \"" << arg << "\"\n";
    return false;
  }
  if(i < 0 || i > 100) {
    cerr << "Colour out of range: " << i << endl;
    return false;
  }
  if(is.eof()) { col.grey = true; col.red = i/100; return true; }
  col.red = i/100;
  is >> i;
  if(!is || ((c = is.get()) != '-')) {
    cerr << "Invalid colour: \"" << arg << "\"\n";
    return false;
  }
  if(i < 0 || i > 100) {
    cerr << "Colour out of range: " << i << endl;
    return false;
  }
  col.green = i/100;
  is >> i;
  if(!is || (is.get() != EOF)) {
    cerr << "Invalid colour: \"" << arg << "\"\n";
    return false;
  }
  if(i < 0 || i > 100) {
    cerr << "Colour out of range: " << i << endl;
    return false;
  }
  col.blue = i/100;
  col.grey = false;
  return true;
}

string next_bit(const string& s, string::const_iterator& i)
{
  string::const_iterator j = i;
  while(j != s.end() && *j != ',') j++;
  string result(i, j);
  i = j; if(i != s.end()) ++i;
  return result;
}

class myopt : public option {
public:
  myopt(char c, string l, string d) : option(c, l, d) {}
  myopt(string d) : option(d) {}
  myopt(char c, string l, string d, string n, bool o = false) 
    : option(c, l, d, n, o) {}
  bool process(const string& a, const arg_parser& ap) const;
};

void setup_args(arg_parser& p)
{
  p.add(new myopt("General options:"));
  p.add(new myopt('?', "help", "Print this help message"));
  p.add(new myopt('V', "version", "Print the program version"));
  p.add(new myopt('o', "output-file", 
		  "Output to FILE instead of standard output", "FILE"));
  p.add(new myopt('e', "eps", 
		  "Generate an Encapsulated PostScript (EPS) file"));
  p.add(new myopt('P', "pdf", 
		  "Generate a Portable Document Format (PDF) file"));
  p.add(new myopt('t', "title",
    "Print TITLE above the method, using the font, size and colour"
    " specified.  In the string TITLE, the character `$' stands for the"
    " full name of the method.  If no arguments are specifed, print the"
    " name of the method as the title.", "TITLE[,FONT[,SIZE[,COLOUR]]]",
		  true));
  p.add(new myopt("Style options:"));
  p.add(new myopt('f', "font", "Use PostScript font FONT.  If this option"
    " is not specified, the font defaults to Helvetica.", "FONT"));
  p.add(new myopt('s', "font-size", "Use font size SIZE, in points", 
		  "SIZE"));
  p.add(new myopt('c', "colour", "Print everything except lines in COLOUR",
		  "COLOUR"));
  p.add(new myopt('l', "line",
    "Draw a line for BELL"
    " with colour COLOUR and thickness DIMENSION.  If `x' is included after"
    " BELL, draw the line only when that bell is passing another bell which"
    " has a line drawn.  If no arguments are given, don't draw any lines."
    "  This option may be used multiple times.",
		  "BELL[x][,COLOUR[,DIMENSION]]", true));
  p.add(new myopt('n', "no-numbers", "Don't print numbers: print only lines"));
  p.add(new myopt('b', "place-bells", "Print place bells for"
    " BELL.  If BELL is not specified, print place bells for the first"
    " working bell which has a line drawn.  If BELL is `x', don't print"
		  " place bells.", "BELL|x", true));
  p.add(new myopt('p', "place-notation", "Print place"
    " notation for the first lead, every lead, or no leads.  The default is"
    " to print place notation for the first lead.", "first|all|none",
		  true));
  p.add(new myopt('r', "rule", "Print rule-offs"
    " (thin horizontal lines) after the Ath change in each lead, and every B"
    " changes after that.  For example, use \"-r2,6\" for Stedman.  If no"
    " argument is given, don't draw any rule-offs."
    "  This option may be used multiple times.", "A[,B]", true));
  p.add(new myopt('m', "miss-numbers", 
    "Miss out the numbers for bells which have lines drawn:  always, never,"
    " except at lead heads, or except at the beginning and end of a column."
    " If no argument is given, the default is `lead'.", 
		  "always|never|lead|column", true));
  p.add(new myopt("Layout options:"));
  p.add(new myopt('L', "landscape",
    "Print in landscape orientation instead of portrait (not for EPS files)"));
  p.add(new myopt('S', "paper-size",  "Set the paper size to the"
    " width and height given (not for EPS files).  The default is A4.",
		  "DIMENSION,DIMENSION"));
  p.add(new myopt('F', "fit", "Fit the image"
    " to the width and height specified, or to the page (less a half-inch"
    " margin) if no argument is given", "DIMENSION,DIMENSION", true));
  p.add(new myopt('a', "across-first", "Print the second lead to the"
		  " right of the first, instead of below the first."));
  p.add(new myopt('x', "xspace", "Set the horizontal distance between"
		  " consecutive bells in a row to DIMENSION", "DIMENSION"));
  p.add(new myopt('y', "yspace", "Set the vertical distance between"
		  " consecutive rows to DIMENSION", "DIMENSION"));
  p.add(new myopt('h', "hgap", "Set the horizontal gap between successive"
		  " columns to DIMENSION", "DIMENSION"));
  p.add(new myopt('v', "vgap", "Set the vertical gap between successive"
		  " sets of columns to DIMENSION"));
  p.add(new myopt('i', "leads-per-column", "Print NUMBER leads per column",
		  "NUMBER"));
  p.add(new myopt('I', "rows-per-column", "Print NUMBER rows per column",
		  "NUMBER"));
  p.add(new myopt('j', "total-leads", "Print NUMBER leads in total",
		  "NUMBER"));
  p.add(new myopt('J', "total-rows", "Print NUMBER rows in total",
		  "NUMBER"));
  p.add(new myopt('k', "columns-across", "Print NUMBER columns across"
		  " the page", "NUMBER"));
  p.add(new myopt('d', "columns-down", "Print NUMBER sets of columns down"
		  " the page", "NUMBER"));
}

bool myopt::process(const string& arg, const arg_parser& ap) const
{
  string::const_iterator s;
  switch(shortname) {
    case '\0' :
      if(args.library_name.empty())
	args.library_name = arg;
      else if(args.method_name.empty())
	args.method_name = arg;
      else return false;
      break;
    case '?' :
      ap.help();
      exit(0);
    case 'V' :
      cout << "psline is from the Ringing Class Library version "
	RINGING_VERSION ".\n";
      exit(0);
    case 'e' :
      args.format = eps;
      break;
    case 'P' :
      args.format = pdf;
      break;
    case 'n' :
      args.numbers = false;
      break;
    case 'a':
      args.vgap_mode = true;
      break;
    case 'L':
      args.landscape = true;
      break;
    case 'o' :
      args.output_file = arg;
      break;
    case 'f' :
      args.font = arg;
      break;
    case 's' :
      return parse_int(arg, args.font_size);
    case 'i' :
      args.rows_per_column = 0;
      return parse_int(arg, args.leads_per_column);
    case 'I' :
      args.leads_per_column = 0;
      return parse_int(arg, args.rows_per_column);
    case 'j' :
      args.total_rows = 0;
      return parse_int(arg, args.total_leads);
    case 'J' :
      args.total_leads = 0;
      return parse_int(arg, args.total_rows);
    case 'd' :
      return parse_int(arg, args.sets_per_page);
    case 'k' :
      return parse_int(arg, args.columns_per_set);
    case 'x' :
      args.fit = false;
      return parse_dimension(arg, args.xspace);
    case 'y' :
      args.fit = false;
      return parse_dimension(arg, args.yspace);
    case 'h' :
      return parse_dimension(arg, args.hgap);
    case 'v' :
      return parse_dimension(arg, args.vgap);
    case 'F' :
      args.fit = true;
      if(!arg.empty()) {
	s = arg.begin();
	if(!parse_dimension(next_bit(arg, s), args.fitwidth)) return false;
	if(s == arg.end()) {
	  cerr << "Not enough arguments: \"" << arg << "\"\n";
	  return false;
	}
	if(!parse_dimension(next_bit(arg, s), args.fitheight)) return false;
	if(s != arg.end()) {
	  cerr << "Too many arguments: \"" << arg << "\"\n";
	  return false;
	}
      }
      break;
    case 'S' :
      s = arg.begin();
      if(!parse_dimension(next_bit(arg, s), args.width)) return false;
      if(s == arg.end()) {
	 cerr << "Not enough arguments: \"" << arg << "\"\n";
	 return false;
      }
      if(!parse_dimension(next_bit(arg, s), args.height)) return false;
      if(s != arg.end()) {
	cerr << "Too many arguments: \"" << arg << "\"\n";
	return false;
      }
      break;
    case 'c' :
      return parse_colour(arg, args.col);
    case 'l' :
      args.custom_lines = true;
      if(!arg.empty()) {
	s = arg.begin();
	bell b;
	try {
	  b.from_char(*s);
	}
	catch(bell::invalid e) {
	  cerr << "Invalid bell: '" << *s << "'\n";
	  return false;
	}
	printrow::options::line_style st;
	st.width.n = 1; st.width.d = 2; st.width.u = dimension::points;
	st.col.grey = false; st.col.red = st.col.green = 0; st.col.blue = 1.0;
	st.crossing = false;
	++s;
	if(s != arg.end()) {
	  if(*s == 'x' || *s == 'X') { st.crossing = true; ++s; }
	  if(s != arg.end()) {
	    if(*s++ != ',') {
	      cerr << "Invalid bell: \"" << arg << "\"\n";
	      return false;
	    }
	    if(!parse_colour(next_bit(arg, s), st.col)) return false;
	    if(s != arg.end() && !parse_dimension(next_bit(arg, s), st.width))
	      return false;
	    if(s != arg.end()) {
	      cerr << "Too many arguments: \"" << arg << "\"\n";
	      return false;
	    }
	  }
	}
	args.lines[b] = st;
      } else
	args.lines.clear();
      break;
    case 'b' :
      if(!arg.empty()) {
	s = arg.begin();
	if(*s == 'X' || *s == 'x')
	  args.placebells = -1;
	else {
	  bell b;
	  try {
	    b.from_char(*s);
	  }
	  catch(bell::invalid e) {
	    cerr << "Invalid bell: '" << *s << "'\n";
	    return false;
	  }
	  args.placebells = b;
	}
      }
      break;
    case 't' :
      if(!arg.empty()) {
	s = arg.begin();
	args.title = next_bit(arg, s);
	if(s != arg.end()) {
	  args.title_style.font = next_bit(arg, s);
	  if(s != arg.end()) {
	    if(!parse_int(next_bit(arg, s), args.title_style.size))
	      return false;
	    if(s != arg.end()) {
	      if(!parse_colour(next_bit(arg, s), args.title_style.col))
		return false;
	      if(s != arg.end()) {
		cerr << "Too many arguments: \"" << arg << "\"\n";
		return false;
	      }
	    }
	  }
	}
      } else
	args.title = "$";
      break;
    case 'm' :
      if(!arg.empty()) {
	if(arg == "always") 
	  args.number_mode = printmethod::miss_always;
	else if(arg == "never") 
	  args.number_mode = printmethod::miss_never;
	else if(arg == "lead") 
	  args.number_mode = printmethod::miss_lead;
	else if(arg == "column") 
	  args.number_mode = printmethod::miss_column;
	else {
	  cerr << "Unrecognised argument: \"" << arg << "\"\n";
	  return false;
	}
      } else
	args.number_mode = printmethod::miss_lead;
      break;	     
    case 'p' :
      if(!arg.empty()) {
	if(arg == "none")
	  args.pn_mode = printmethod::pn_none;
	else if(arg == "first")
	  args.pn_mode = printmethod::pn_first;
	else if(arg == "all")
	  args.pn_mode = printmethod::pn_all;
	else {
	  cerr << "Unrecognised argument: \"" << arg << "\"\n";
	  return false;
	}
      } else
	args.pn_mode = printmethod::pn_all;
      break;
    case 'r' :
      args.custom_rules = true;
      if(!arg.empty()) {
	int a, b = 0;
	s = arg.begin();
	if(!parse_int(next_bit(arg, s), a)) return false;
	if(s != arg.end()) {
	  if(!parse_int(next_bit(arg, s), b)) return false;
	  if(s != arg.end())
	    cerr << "Too many arguments: \"" << arg << "\"\n";
	}
	args.rules.push_back(pair<int,int>(a,b));
      } else
	args.rules.clear();
      break;
    default :
      cerr << "Unrecognised argument.  This shouldn't happen.\n";
      return false;
  }
  return true;
}

int main(int argc, char *argv[])
{
  // Set up some default arguments
  args.width.n = 210; args.width.d = 1; args.width.u = dimension::mm;
  args.height.n = 297; args.height.d = 1; args.height.u = dimension::mm;
  args.col.grey = true; args.col.red = 0;
  args.placebells = -2;
  args.numbers = true;
  args.format = ps;
  args.fit = true;
  args.vgap_mode = false;
  args.title_style.font = "Helvetica";
  args.title_style.size = 18;
  args.title_style.col.grey = true;
  args.title_style.col.red = 0;
  args.number_mode = printmethod::miss_lead;
  args.pn_mode = -1;
  args.rows_per_column = 0;
  args.leads_per_column = 0;
  args.columns_per_set = 0;
  args.sets_per_page = 0;
  args.total_leads = 0;
  args.total_rows = 0;
  args.custom_lines = false;
  args.custom_rules = false;

  // Parse the arguments
  {
    arg_parser p(argv[0], new myopt(0, "", 
      "psline -- print out lines for methods in PostScript or PDF.\v"
"Most options have sensible defaults.  In particular, unless you specify"
" otherwise, the image will be fitted to the page; and lines will be drawn"
" for the treble and for the bell which makes the lead end place."
"  Options requiring a DIMENSION may be specified in many different ways,"
" as for example \"1 3/8 in\", \"1/2pt\" or \"1.3cm\".  Options requiring"
" a COLOUR may be specified as either an integer between 0 and 100,"
" signifying a grey level; or as three integers between 0 and 100, separated"
" by minus signs (`-'), specifying red, green and blue levels.",
			   "LIBRARY METHOD\nBELLS:PLACE-NOTATION"));
    setup_args(p);

    if(!p.parse(argc, argv)) {
      p.usage();
      return 1;
    }

    if(args.method_name.empty()) { // Place notation
      string::iterator s = args.library_name.begin();
      while(s != args.library_name.end() && *s != ':') ++s;
      if(s == args.library_name.end() 
	 || !parse_int(string(args.library_name.begin(), s), args.bells)) 
        { p.usage(); return 1; }
      args.library_name = string(++s, args.library_name.end());
    };
  }

  method m;

  try {
    if(!args.method_name.empty()) {
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
    if(!args.font.empty()) pm.opt.style.font = args.font;
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
    if(args.sets_per_page)
      pm.sets_per_page = args.sets_per_page;
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
    if(!args.output_file.empty()) { // Open the output file
      ofs.open(args.output_file.c_str(), ios::binary);
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
    switch(args.format) {
      case eps :
	pp = new printpage_ps(*os, 0, 0, int(pm.total_width()), 
			      int(pm.total_height() 
				   + (args.title.empty() ? 0 
				      : args.title_style.size * 2)));
	break;
      case ps:
	if(args.landscape)
	  pp = new printpage_ps(*os, args.height);
	else
	  pp = new printpage_ps(*os);
	break;
      case pdf:
	if(args.landscape)
	  pp = new printpage_pdf(*os, args.height, args.width, true);
	else
	  pp = new printpage_pdf(*os, args.width, args.height);
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
