// -*- C++ -*- psline.cpp - print out lines for methods
// Copyright (C) 2001, 2002, 2019, 2020, 2021
// Martin Bright <martin@boojum.org.uk> and
// Richard Smith <richard@ex-parrot.com>

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
#include <fstream.h>
#else
#include <iostream>
#include <fstream>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/printm.h>
#include <ringing/print_ps.h>
#include <ringing/print_pdf.h>
#include <ringing/mslib.h>
#include <ringing/cclib.h>
#include <ringing/streamutils.h>
#include "args.h"

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

enum format_t { ps, eps, pdf };

struct arguments {
  string output_file;
  string library_name;   // Contains the first arg -- either library name or 
                         // number of bells : place notation.
  string method_name;    // Contains the second arg -- only present if 
                         // library_name really is a library name
  int bells;
  format_t format;
  string title; text_style title_style;
  string font; 
  init_val<int, 0> font_size; 
  init_val<int, 0> label_font_size; 
  colour col, label_col;
  bool custom_lines;
  map<int, printrow::options::line_style> lines;
  bool custom_rules;
  list<printmethod::rule> rules;
  bool numbers;
  printmethod::number_mode_t number_mode;
  int pn_mode;
  int placebells;
  init_val<bool, false> reverse_placebells;
  init_val<bool, false> placebells_at_rules;
  init_val<bool, false> placebell_blobs_only;
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
  int pages;
  printrow::options::line_style grid_style;
  int grid;
  string calls;
  string rounds;
  bool calls_at_rules;
  dimension calls_voffset;
};

arguments args;
string progname;

inline int divu(int a, int b) { return (a - 1) / b + 1; }

bool parse_int(const string& arg, int& i)
{
  try {
    i = lexical_cast<int>(arg);
  } catch(bad_lexical_cast const&) {
    cerr << "Invalid integer argument: \"" << arg << "\"\n";
    return false;
  }
  return true;
}

bool parse_float(const string& arg, float& f)
{
  try {
    f = lexical_cast<float>(arg);
  } catch(bad_lexical_cast const&) {
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
  try {
    string::size_type j = arg.find('-');
    int i = lexical_cast<int>( arg.substr(0,j) );
    if(i < 0 || i > 100) {
      cerr << "Colour out of range: " << i << endl;
      return false;
    }
    if(j==string::npos) { col.grey = true; col.red = i/100.0f; return true; }
    col.red = i/100.0f;
    col.null = false;
    
    string::size_type k = arg.find('-', j+1);
    if (k==string::npos) {
      cerr << "Invalid colour: \"" << arg << "\"\n";
      return false;
    }
    i = lexical_cast<int>( arg.substr(j+1, k-j-1) );
    if(i < 0 || i > 100) {
      cerr << "Colour out of range: " << i << endl;
      return false;
    }
    col.green = i/100.0f;

    i = lexical_cast<int>( arg.substr(k+1, string::npos) );
    if(i < 0 || i > 100) {
      cerr << "Colour out of range: " << i << endl;
      return false;
    }
    col.blue = i/100.0f;
    col.grey = false;
    return true;
  } 
  catch (bad_lexical_cast const&) {
    cerr << "Invalid colour: \"" << arg << "\"\n";
    return false;
  }
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
  p.set_default(new myopt('\0', "", ""));
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
  p.add(new myopt('\001', "title-font", "", "FONT"));
  p.add(new myopt('\002', "title-size", "", "SIZE"));
  p.add(new myopt('\003', "title-colour", "", "COLOUR"));
  p.add(new myopt("Style options:"));
  p.add(new myopt('f', "font", "Use PostScript font FONT.  If this option"
    " is not specified, the font defaults to Helvetica.", "FONT"));
  p.add(new myopt('s', "font-size", "Use font size SIZE, in points.  If"
    " specified, LABELSIZE is used for place bells and calling positions.", 
		  "SIZE[,LABELSIZE]"));
  p.add(new myopt('c', "colour", "Print everything except lines in COLOUR. "
    " If COLOUR2 is specified, it is used for place bells, calling positions"
    " and place notations.",
		  "COLOUR[,COLOUR2]"));
  p.add(new myopt('l', "line",
    "Draw a line for BELL"
    " with colour COLOUR and thickness DIMENSION.  If `x' is included after"
    " BELL, draw the line only when that bell is passing another bell which"
    " has a line drawn.  If no arguments are given, don't draw any lines. "
    " BELL may be `:a' for all bells, `:h' for all hunt bells, `:w' for all "
    " working bells, or `:v' for one working bell.  This option may be used "
    " multiple times.",
		  "BELL[x][,COLOUR[,DIMENSION]]", true));
  p.add(new myopt('G', "guides",
    "Draw guides indicating bell positions, in style STYLE (currently"
    " 1, 2 or 3), with colour COLOUR and (for style 1) thickness DIMENSION.",
		  "STYLE[,COLOUR[,DIMENSION]]", true));
  p.add(new myopt('n', "no-numbers", "Don't print numbers: print only lines"));
  p.add(new myopt('b', "place-bells", "Print place bells for"
    " BELL.  If BELL is not specified, print place bells for the first"
    " working bell which has a line drawn.  If BELL is `x' or `none', don't"
    " print place bells.  If BELL is `default', select the bell automatically."
    " If `rev' is appended, show reverse place bells too.  If `rules' is"
    " appended, print place bells at the rules rather than the lead ends.  If"
    " `blobs' is appended, only draw the place bell blobs.",
    "{BELL|x|none|default}[,rules][,rev]", true));
  p.add(new myopt('p', "place-notation", "Print place"
    " notation for the first lead, every lead, or no leads.  The default is"
    " to print place notation for the first lead, supressing any mirrored"
    " section due to palindromicity.  Use first-asym to for the whole of the"
    " first lead, regardlessof symmetry.  Append ,nox to omit `X' for cross"
    " changes, or ,lcx to make the cross lower-case.", 
                  "first|first-asym|all|none[,nox|,lcx]", true));
  p.add(new myopt('r', "rule", "Print rule-offs"
    " (thin horizontal lines) after the Ath change in each lead, and every B"
    " changes after that.  For example, use \"-r2,6\" for Stedman.  For a"
    " rule that is not repeated every lead, use `once' as B, for example, use"
    " \"-r112,once\" for a rule at the half course in surprise major.  If no"
    " argument is given, don't draw any rule-offs."
    "  This option may be used multiple times.", 
    "A[,B[,COLOUR[,DIMENSION]]]", true));
  p.add(new myopt('m', "miss-numbers", 
    "Miss out the numbers for bells which have lines drawn:  always, never,"
    " except at lead heads, or except at the beginning and end of a column."
    " If no argument is given, the default is `lead'.", 
		  "always|never|lead|column", true));
  p.add(new myopt('q', "calls", 
    "Calling positions for each lead.  Use a space to suppress a call at that"
    " position.  Multicharacter calling positions should be enclosed in"
    " {braces}.  Append `,rules' to print calling positions before rules"
    " rather than before the lead ends.  Append ,voffset=DIMENSION to shift"
    " calls up by that amount" ,"CALLS[,rules]"));
  p.add(new myopt('R', "rounds", "Starting row.","ROUNDS"));
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
  p.add(new myopt('g', "pages", "Fit the method onto NUMBER pages",
		  "NUMBER"));
  p.add(new myopt('x', "xspace", "Set the horizontal distance between"
		  " consecutive bells in a row to DIMENSION", "DIMENSION"));
  p.add(new myopt('y', "yspace", "Set the vertical distance between"
		  " consecutive rows to DIMENSION", "DIMENSION"));
  p.add(new myopt('h', "hgap", "Set the horizontal gap between successive"
		  " columns to DIMENSION", "DIMENSION"));
  p.add(new myopt('v', "vgap", "Set the vertical gap between successive"
		  " sets of columns to DIMENSION", "DIMENSION"));
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
  float f;
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
      s = arg.begin();
      if (!parse_float(next_bit(arg, s), f)) return false;
      args.font_size = static_cast<int>(f * 10);
      if (s != arg.end()) {
        if (!parse_float(next_bit(arg, s), f)) return false;
        args.label_font_size = static_cast<int>(f * 10);
      }
      if (s != arg.end()) {
        cerr << "Too many arguments: \"" << arg << "\"\n";
        return false;
      }
      break;
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
    case 'g' :
      return parse_int(arg, args.pages);
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
      s = arg.begin();
      if (!parse_colour(next_bit(arg, s), args.col)) return false;
      if (s != arg.end()) {
        if (!parse_colour(next_bit(arg, s), args.label_col)) return false;
      }
      if (s != arg.end()) {
        cerr << "Too many arguments: \"" << arg << "\"\n";
        return false;
      }
    case 'G' :
      if(!arg.empty()) {
	s = arg.begin();
        if(!parse_int(next_bit(arg, s), args.grid)) return false;
	if(s != arg.end() && !parse_colour(next_bit(arg, s), 
					   args.grid_style.col)) return false;
	if(s != arg.end() && !parse_dimension(next_bit(arg, s), 
					      args.grid_style.width))
	  return false;
	if(s != arg.end()) {
	  cerr << "Too many arguments: \"" << arg << "\"\n";
	  return false;
	}
      } else {
        args.grid = 1;
      }
      return true;
    case 'l' :
      args.custom_lines = true;
      if(!arg.empty()) {
	list<int> bl;
	s = arg.begin();
	if(*s == ':') {
	  ++s;
	  if(s == arg.end()) { cerr << "Invalid bell: ':'\n"; return false; }
	  switch(*s) {
	    case 'a' : bl.push_back(-1); break; // All bells
	    case 'h' : bl.push_back(-2); break; // All hunts
            case 'w' : bl.push_back(-3); break; // All working bells
            case 'v' : bl.push_back(-4); break; // One working bell
            default : 
	      cerr << "Invalid bell: ':" << *s << "'\n";
	      return false;
	  }
	  ++s;
	} else {
	  bell b;
	  do {
	    try {
	      b.from_char(*s);
	    }
	    catch(bell::invalid e) {
	      cerr << "Invalid bell: '" << *s << "'\n";
	      return false;
	    }
	    bl.push_back(b);
	    ++s;
	  } while(s != arg.end() && *s != 'x' && *s != 'X' && *s != ',');
	}
	printrow::options::line_style st;
	st.width.n = 1; st.width.d = 2; st.width.u = dimension::points;
	st.col.grey = false; st.col.red = st.col.green = 0; st.col.blue = 1.0;
	st.crossing = false; 
        // 
        st.no_dots = (bl.size() == 1 && bl.back() == -2);
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
	list<int>::iterator j;
	for(j = bl.begin(); j != bl.end(); ++j)
	  args.lines[*j] = st;
      } else
	args.lines.clear();
      break;
    case 'b' :
      if(!arg.empty()) {
	s = arg.begin();
	while (s != arg.end()) {
          string a = next_bit(arg, s);
          if(a == "X" || a == "x" || a == "none")
            args.placebells = -1;
          else if(a == "def" || a == "default")
            args.placebells = -2;
          else if(a == "rev" || a == "reverse")
            args.reverse_placebells = true;
          else if(a == "rules")
            args.placebells_at_rules = true;
          else if(a == "blobs")
            args.placebell_blobs_only = true;
          else if (a.length() == 1) {
            bell b;
            try {
              b.from_char(a[0]);
            }
            catch(bell::invalid e) {
              cerr << "Invalid bell: '" << *s << "'\n";
              return false;
            }
            args.placebells = b;
          }
          else {
            cerr << "Unrecognised argument: \"" << arg << "\"\n";
            return false;
          }
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
	    if(!parse_float(next_bit(arg, s), f))
	      return false;
	    args.title_style.size = static_cast<int>(f * 10);
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
    case '\001' :
      args.title_style.font = arg;
      break;
    case '\002' :
      if(!parse_float(arg, f)) return false;
      args.title_style.size = static_cast<int>(f * 10);
      break;
    case '\003' :
      return parse_colour(arg, args.title_style.col);
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
    case 'q' :
      s = arg.begin();
      args.calls = next_bit(arg, s);
      while (s != arg.end()) {
        string a = next_bit(arg, s);
        if (a == "rules")
          args.calls_at_rules = true;
        else if (a.substr(0,8) == "voffset=") {
          string const a2(a.substr(8));
          if (!parse_dimension(a2, args.calls_voffset)) {
            cerr << "Unable to parse -q...voffset argument: \"" 
                 << a2 << "\"\n";
            return false;
          }
        }
        else {
	  cerr << "Unrecognised argument: \"" << arg << "\"\n";
          return false;
        }
      }
      break;
    case 'R' :
      args.rounds = arg;
      break;
    case 'p' :
      if(!arg.empty()) {
	s = arg.begin();
        string a = next_bit(arg, s);
	if (a == "none")
	  args.pn_mode = printmethod::pn_none;
	else if (a == "first")
	  args.pn_mode = printmethod::pn_first;
	else if (a == "first-asym")
	  args.pn_mode = printmethod::pn_first_asym;
	else if (a == "all")
	  args.pn_mode = printmethod::pn_all;
	else {
	  cerr << "Unrecognised argument: \"" << arg << "\"\n";
	  return false;
	}
	if (s != arg.end()) {
          string a = next_bit(arg, s);
	  if (a == "nox")
	    args.pn_mode |= printmethod::pn_nox;
	  else if (a == "lcx")
	    args.pn_mode |= printmethod::pn_lcross;
	  else {
	    cerr << "Unrecognised argument: \"" << arg << "\"\n";
	    return false;
	  }
	}
      } else
	args.pn_mode = printmethod::pn_all;
      break;
    case 'r' :
      args.custom_rules = true;
      if(!arg.empty()) {
	s = arg.begin();
        printmethod::rule r;
	if(!parse_int(next_bit(arg, s), r.offset)) return false;
	if(s != arg.end()) {
          string n(next_bit(arg, s));
          // -1 means a one off line, e.g. at the half course, while 
          // 0 means repeat every lead.
          if (n == "once") r.repeat=-1;
	  else if (!parse_int(n, r.repeat)) return false;
          if (s != arg.end()) {
            if (!parse_colour(next_bit(arg, s), r.style.col)) return false;
            if (s != arg.end()) {
              if (!parse_dimension(next_bit(arg, s), r.style.width))
                return false;
              while (s != arg.end()) {
                string f(next_bit(arg, s));
                if (f == "narrow") r.flags = printrow::no_hextend; 
                else { 
                  cerr << "Unknown flag on rule: \"" << f << "\"\n";
                  return false;
                }
              }
            }
          }
	}
	args.rules.push_back(r);
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
  bell::set_symbols_from_env();

  // Set up some default arguments
  args.width.n = 210; args.width.d = 1; args.width.u = dimension::mm;
  args.height.n = 297; args.height.d = 1; args.height.u = dimension::mm;
  args.col = colour(0); // black
  args.placebells = -2;
  args.numbers = true;
  args.format = ps;
  args.fit = true;
  args.vgap_mode = false;
  args.title_style.font = "Helvetica";
  args.title_style.size = 180;
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
  args.pages = 0;
  args.custom_lines = false;
  args.custom_rules = false;
  args.grid = 0;
  args.grid_style.width.n = 1; args.grid_style.width.d = 4; 
  args.grid_style.width.u = dimension::points;
  args.grid_style.col.grey = true; args.grid_style.col.red = 0.9f;

  // Parse the arguments
  {
    arg_parser p(argv[0], 
      "psline -- print out lines for methods in PostScript or PDF.\v"
"Most options have sensible defaults.  In particular, unless you specify"
" otherwise, the image will be fitted to the page; and lines will be drawn"
" for the treble and for the bell which makes the lead end place."
"  Options requiring a DIMENSION may be specified in many different ways,"
" as for example \"1 3/8 in\", \"1/2pt\" or \"1.3cm\".  Options requiring"
" a COLOUR may be specified as either an integer between 0 and 100,"
" signifying a grey level; or as three integers between 0 and 100, separated"
" by minus signs (`-'), specifying red, green and blue levels.",
			   "LIBRARY METHOD\nBELLS:PLACE-NOTATION");
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

#if RINGING_USE_EXCEPTIONS
  try 
#endif
  {
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
      if(m.length() == 0) {
	cerr << "Method is of zero length!\n";
	return 1;
      }
    }

    // Set up our options
    printmethod pm(m);
    pm.defaults();

    // Set up the things which affect the fitting (and other things too)
    if(args.total_leads)
      pm.total_rows = args.total_leads * m.length();
    else if(args.total_rows)
      pm.total_rows = args.total_rows;
    row lh = m.lh();
    if(args.custom_lines) {
      pm.opt.lines.clear();
      map<int, printrow::options::line_style>::const_iterator j;
      bell b;
      change c = m[m.length() - 1];
      bool found_working_bell = false;
      for(b = 0; b < m.bells(); b = b + 1) {
	j = args.lines.find(b);
	if(j == args.lines.end()) {
	  if(lh[b] == b) 
	    j = args.lines.find(-2);
	  else {
	    if(!found_working_bell && c.findplace(b)) { 
	      j = args.lines.find(-4);
	      if(j == args.lines.end()) j = args.lines.find(-3);
	      found_working_bell = true;
	    } else
	      j = args.lines.find(-3);
	  }
	  if(j == args.lines.end())
	    j = args.lines.find(-1);
	}
	if(j != args.lines.end()) pm.opt.lines[b] = (*j).second;
      }
      if(!found_working_bell 
	 && (j = args.lines.find(-4)) != args.lines.end()) {
	for(b = 0; b < m.bells() && !found_working_bell; b = b + 1)
	  if(lh[b] != b) {
	    pm.opt.lines[b] = (*j).second;
	    found_working_bell = true;
	  }
      }
    }
    if(args.custom_rules) 
      pm.rules = args.rules;
    else { // Set up some default rules
      pm.rules.clear();
      if(args.numbers) {
	int cl = m.methclass();
	if(cl == method::M_TREBLE_BOB 
	   || cl == method::M_SURPRISE 
	   || cl == method::M_DELIGHT)
	  pm.rules.push_back(pair<int,int>(4,4));
	else
	  pm.rules.push_back(pair<int,int>(m.length(),0));
      }
    }
    if(args.placebells == -2) {
      bell b;
      pm.placebells = -1;
      for(b = 0; b < m.bells(); b = b + 1)
	if(lh[b] != b && pm.opt.lines.find(b) != pm.opt.lines.end()) {
	  if(pm.placebells == -1)
	    pm.placebells = b;
	  else {
	    pm.placebells = -1;
	    break;
	  }
	}
    } else
      pm.placebells = args.placebells;
    pm.reverse_placebells = args.reverse_placebells;
    pm.placebells_at_rules = args.placebells_at_rules;
    pm.placebell_blobs_only = args.placebell_blobs_only;
    pm.calls_at_rules = args.calls_at_rules;
    pm.calls_voffset = args.calls_voffset;
    pm.opt.flags = args.numbers ? printrow::options::numbers : 0;
    if(args.pn_mode == -1)
      pm.pn_mode = printmethod::pn_first;
    else
      pm.pn_mode = static_cast<printmethod::pn_mode_t>(args.pn_mode);
    pm.calls = args.calls;
    if (args.rounds.length()==m.bells())
      pm.startrow(args.rounds);
    
    // Set the space to fit to
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
			       - args.title_style.size * 0.2f, 1);
    
    // Now fit to the space
    if(args.leads_per_column || args.rows_per_column) {
      if(args.leads_per_column)
	pm.rows_per_column = args.leads_per_column * m.length();
      else if(args.rows_per_column)
	pm.rows_per_column = args.rows_per_column;
      if(args.columns_per_set) {
	pm.columns_per_set = args.columns_per_set;
	if(args.sets_per_page) {
	  pm.sets_per_page = args.sets_per_page;
	} else {
	  if(!args.pages) args.pages = 1;
	  pm.sets_per_page = divu(pm.total_rows, 
				  args.pages * pm.columns_per_set 
				  * pm.rows_per_column);
	}
      } else {
	if(!args.sets_per_page) args.sets_per_page = 1;
	pm.sets_per_page = args.sets_per_page;
	if(!args.pages) args.pages = 1;
	pm.columns_per_set = divu(pm.total_rows,
				  args.pages * pm.sets_per_page
				  * pm.rows_per_column);
      }
      pm.scale_to_space(args.fitwidth, args.fitheight, args.numbers ? 1.f : 2.f);
    } else {
      if(args.columns_per_set) {
	pm.columns_per_set = args.columns_per_set;
	if(!args.sets_per_page) args.sets_per_page = 1;
	pm.sets_per_page = args.sets_per_page;
	if(!args.pages) args.pages = 1;
	pm.rows_per_column = divu(pm.total_rows,
				  args.pages * pm.columns_per_set
				  * pm.sets_per_page * m.length())
	  * m.length();
	pm.scale_to_space(args.fitwidth, args.fitheight, 
			  args.numbers ? 1.f : 2.f);
      } else {
	if(args.sets_per_page) {
	  pm.rows_per_column = m.length();
	  pm.sets_per_page = args.sets_per_page;
	  if(!args.pages) args.pages = 1;
	  pm.columns_per_set = divu(pm.total_rows,
				    args.pages * pm.sets_per_page
				    * pm.rows_per_column);
	  pm.scale_to_space(args.fitwidth, args.fitheight, 
			    args.numbers ? 1.f : 2.f);
	} else {
	  if(!args.pages) args.pages = 1;
          int tr = pm.total_rows;
	  pm.total_rows = divu(tr, args.pages * m.length()) * m.length();
	  pm.fit_to_space(args.fitwidth, args.fitheight, 
			  args.vgap_mode, args.numbers ? 1.f : 2.f);
	  pm.total_rows = tr;
	}
      }
    }

    // Set up the things which override the fitting, and everything else
    if(!args.font.empty()) pm.opt.style.font = args.font;
    if(args.font_size) pm.opt.style.size = args.font_size;
    if(args.label_font_size) pm.opt.label_style.size = args.label_font_size;
    pm.opt.style.col = args.col;
    pm.opt.label_style.col = args.label_col;
    if(args.xspace != 0) pm.opt.xspace = args.xspace;
    if(args.yspace != 0) pm.opt.yspace = args.yspace;
    if(args.hgap != 0) pm.hgap = args.hgap;
    if(args.vgap != 0) pm.vgap = args.vgap;
    pm.number_mode = args.number_mode;
    if(args.grid > 0) { 
      pm.opt.grid_type = args.grid;
      pm.opt.grid_style = args.grid_style;
    }

    // Get the bounding box of the image
    float blx, bly, urx, ury;
    pm.get_bbox(blx, bly, urx, ury);

    // Position the output correctly
    if(args.format == eps) {
      pm.xoffset.set_float(-blx, 1); 
      pm.yoffset.set_float(-bly, 1);
    } else {
      // Centre the output on the page
      pm.xoffset.set_float((args.width.in_points() - (urx + blx)) / 2, 1);
      pm.yoffset.set_float((args.height.in_points() - (ury + bly) 
			    - (args.title.empty() ? 0 
			       : args.title_style.size * 0.2f)) / 2, 1);
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
		     + (urx + blx) / 2, 1);
    titley.set_float(pm.yoffset.in_points() + ury
		     + args.title_style.size * 0.1f, 1);
    int i = args.title.find('$');
    if(i != (int) args.title.npos) args.title.replace(i, 1, m.fullname());

    // Create a printpage object
    printpage* pp = NULL;
    switch(args.format) {
      case eps :
	pp = new printpage_ps(*os, 0, 0, int(urx-blx), 
			      int(ury-bly
				   + (args.title.empty() ? 0 
				      : args.title_style.size * 0.2)));
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
    if(!args.title.empty()) 
      pp->text(args.title, titlex, titley, 
	       text_style::centre, args.title_style);
    pm.print(*pp);
    delete pp;
  }
#if RINGING_USE_EXCEPTIONS
  catch(exception& e) {
    cerr << argv[0] << ": " << e.what() << endl;
    return 1;
  }
#endif
  return 0;
}
