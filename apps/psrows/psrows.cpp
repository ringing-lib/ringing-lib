// -*- C++ -*- psrows.cpp - print out rows
// Copyright (C) 2022 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
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

RINGING_USING_NAMESPACE
RINGING_USING_STD

bool parse_int(const string& arg, int& i)
{
  try {
    i = lexical_cast<int>(arg);
  } catch (bad_lexical_cast const&) {
    cerr << "Invalid integer argument: \"" << arg << "\"\n";
    return false;
  }
  return true;
}

bool parse_float(const string& arg, float& f)
{
  try {
    f = lexical_cast<float>(arg);
  } catch (bad_lexical_cast const&) {
    cerr << "Invalid integer argument: \"" << arg << "\"\n";
    return false;
  }
  return true;
}

class colour_exception : public runtime_error { 
 public:
  colour_exception( string const& str ) : runtime_error(str) {}
};

float parse_colour_component(const string& arg, string::size_type start, 
                             string::size_type end) 
{
  string::size_type len( end == string::npos ? string::npos : end-start );
  string comp( arg.substr(start, len) );

  int col;
  try {
    col = lexical_cast<int>(comp);
  }
  catch (bad_lexical_cast const&) {
    throw colour_exception
      ( make_string() << "Invalid colour compoent: \"" << comp 
                      << "\" in \"" << arg << "\"" );
  }

  if (col < 0 || col > 100)
    throw colour_exception
      ( make_string() << "Colour component out of range: \"" << col
                      << "\" in \"" << arg << "\"" );
  return col/100.0f;
}

bool parse_colour(const string& arg, colour& col)
{
  try {
    string::size_type i = arg.find('-');

    float c1 = parse_colour_component(arg, 0, i);
    if (i==string::npos) { col = colour(c1); return true; }

    ++i;
    string::size_type j = arg.find('-', i);
    if (j==string::npos)
      throw colour_exception
        ( make_string() << "Invalid colour: \"" << arg << "\"" );

    float c2 = parse_colour_component(arg, i, j);

    i = j+1;
    j = arg.find('-', i);

    float c3 = parse_colour_component(arg, i, j);
    if (j==string::npos) { col = colour(c1, c2, c3); return true; }

    i = j+1;

    float c4 = parse_colour_component(arg, i, string::npos);
    col = colour(c1, c2, c3, c4); return true;
  } 
  catch (colour_exception const& e) {
    cerr << e.what() << endl;
    return false;
  }
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

string next_bit(const string& s, string::const_iterator& i)
{
  string::const_iterator j = i;
  while(j != s.end() && *j != ',') j++;
  string result(i, j);
  i = j; if(i != s.end()) ++i;
  return result;
}

class dimension_opt : public option {
public:
  dimension_opt( char c, const string& l, const string& d, const string& a,
	         dimension& opt )
    : option(c, l, d, a), opt(opt)
  {}

private:
  // Sets opt to the given argument.
  virtual bool process( const string& arg, const arg_parser& ) const {
    return parse_dimension(arg, opt);
  }
  dimension &opt;
};



enum format_t { ps, eps, pdf };

struct arguments {
  arguments() 
    : col(0), // black 
      width(210, 1, dimension::mm), height(297, 1, dimension::mm) // A4
  {}

  string               output_file;
  format_t             format;

  string               title; 
  text_style           title_style;
  string               font, font_size_str, col_str; 
  init_val<int, 0>     font_size, label_font_size; 
  colour               col, label_col;

  init_val<bool,false> landscape;

  string               paper_size;
  dimension            width, height;
  
  dimension            xspace, yspace;

  init_val<int, 0u>    length;

  vector<string>       custom_lines;
  map<bell, printrow::options::line_style> lines;

  vector<string>           rule_strs;
  list<printmethod::rule>  rules;

  vector<string>           label_strs;
  list<printmethod::label> labels;

  void bind( arg_parser& p );
  bool validate( arg_parser& p );

private:
  bool parse_title( string const& arg );
  bool parse_paper_size();
  bool parse_lines();
  bool parse_font_size();
  bool parse_colours();
  bool parse_rules();
  bool parse_labels();
};

void arguments::bind( arg_parser& p ) {
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new string_opt('o', "output-file", 
    "Output to FILE instead of standard output", "FILE",
    output_file));

  p.add(new boolean_opt('P', "pdf", 
    "Generate a Portable Document Format (PDF) file",
    format, pdf));
 
  p.add( new string_opt('t', "title",
    "Print TITLE above the method, using the font, size and colour"
    " specified.  In the string TITLE, the character `$' stands for the"
    " full name of the method.  If no arguments are specifed, print the"
    " name of the method as the title.", "TITLE[,FONT[,SIZE[,COLOUR]]]",
    title));

  p.add(new strings_opt('r', "rule", "Print rule-offs"
    " (thin horizontal lines) after the Ath change in each lead, and every B"
    " changes after that.  For example, use \"-r2,6\" for Stedman.  For a"
    " rule that is not repeated every lead, use `once' as B, for example, use"
    " \"-r112,once\" for a rule at the half course in surprise major.  If no"
    " argument is given, don't draw any rule-offs."
    "  This option may be used multiple times.", 
    "A[,B[,COLOUR[,DIMENSION]]]", rule_strs, ""));
  p.add(new strings_opt('T', "text", 
    "Additional text to print next to row number NUM.  This option may be"
    " used multiple times.", "NUM,TEXT", label_strs));

  p.add( new boolean_opt('L', "landscape",
    "Print in landscape orientation instead of portrait (not for EPS files)", 
    landscape));
  p.add( new string_opt('S', "paper-size",  "Set the paper size to the"
    " width and height given (not for EPS files).  The default is A4.",
    "DIMENSION,DIMENSION",
    paper_size));

  p.add( new integer_opt('\0', "length", "The number of rows expected.",
    "NUM", length));

  p.add( new string_opt('f', "font", "Use PostScript font FONT.  If this "
    " option is not specified, the font defaults to Helvetica.", "FONT",
    font));
  p.add( new string_opt('s', "font-size", "Use font size SIZE, in points.  If"
    " specified, LABELSIZE is used for place bells and calling positions.", 
    "SIZE[,LABELSIZE]", font_size_str));
  p.add( new string_opt('c', "colour", "Print everything except lines in"
    " COLOUR.  If COLOUR2 is specified, it is used for place bells, calling "
    "positions and place notations.", "COLOUR[,COLOUR2]", col_str));

  p.add( new strings_opt('l', "line",
    "Draw a line for BELL"
    " with colour COLOUR and thickness DIMENSION.  If `x' is included after"
    " BELL, draw the line only when that bell is passing another bell which"
    " has a line drawn.  If no arguments are given, don't draw any lines. "
    " This option may be used multiple times.",
    "[BELL[x]][,COLOUR[,DIMENSION]]", custom_lines, ""));

  p.add(new dimension_opt('x', "xspace", "Set the horizontal distance between"
    " consecutive bells in a row to DIMENSION", "DIMENSION", xspace));
  p.add(new dimension_opt('y', "yspace", "Set the vertical distance between"
    " consecutive rows to DIMENSION", "DIMENSION", yspace));
}
  
bool arguments::parse_title( string const& arg ) {
  string::const_iterator s = arg.begin();
  title = next_bit(arg, s);
  if (s != arg.end()) {
    title_style.font = next_bit(arg, s);
    if (s != arg.end()) {
      float f;
      if (!parse_float(next_bit(arg, s), f))
        return false;
      title_style.size = static_cast<int>(f * 10);
      if (s != arg.end()) {
        if (!parse_colour(next_bit(arg, s), title_style.col))
          return false;
        if (s != arg.end()) {
          cerr << "Too many arguments: \"" << arg << "\"\n";
          return false;
        }
      }
    }
  }
  return true;
}

bool arguments::parse_paper_size() {
  string::const_iterator s = paper_size.begin();

  if (!parse_dimension(next_bit(paper_size, s), width)) return false;
  if (s == paper_size.end()) {
    cerr << "Not enough arguments: \"" << paper_size << "\"\n";
    return false;
  }

  if (!parse_dimension(next_bit(paper_size, s), height)) return false;
  if (s != paper_size.end()) {
    cerr << "Too many arguments: \"" << paper_size << "\"\n";
    return false;
  }

  return true;
}
  
bool arguments::parse_lines() {
  for ( string const& arg : custom_lines ) {
    if (arg.empty()) 
      lines.clear();
    else {
      list<int> bl;
      string::const_iterator s = arg.begin();
      if (*s != ',') {
        bell b;
        do {
          try {
            b.from_char(*s);
          }
          catch (bell::invalid e) {
            cerr << "Invalid bell: '" << *s << "'\n";
            return false;
          }
          bl.push_back(b);
          ++s;
        } while(s != arg.end() && *s != 'x' && *s != 'X' && *s != ',');
      }
      printrow::options::line_style st;
      st.width.n = 1; st.width.d = 2; st.width.u = dimension::points;
      st.col = colour(0, 0, 1.0);
      st.crossing = false; 
      // 
      st.no_dots = (bl.size() == 1 && bl.back() == -2);
      if (s != arg.end()) {
        if (*s == 'x' || *s == 'X') { st.crossing = true; ++s; }
        if (s != arg.end()) {
          if (*s++ != ',') {
            cerr << "Invalid bell: \"" << arg << "\"\n";
            return false;
          }
          if (!parse_colour(next_bit(arg, s), st.col)) return false;
          if (s != arg.end() && !parse_dimension(next_bit(arg, s), st.width))
            return false;
          if (s != arg.end()) {
            cerr << "Too many arguments: \"" << arg << "\"\n";
            return false;
          }
        }
      }
      for (list<int>::iterator j = bl.begin(); j != bl.end(); ++j)
        lines[*j] = st;
    }
  }
  return true;
}

bool arguments::parse_font_size() {
  string::const_iterator s = font_size_str.begin();

  float f;
  if (!parse_float(next_bit(font_size_str, s), f)) return false;
  font_size = static_cast<int>(f * 10);

  if (s != font_size_str.end()) {
    if (!parse_float(next_bit(font_size_str, s), f)) return false;
    label_font_size = static_cast<int>(f * 10);
  }

  if (s != font_size_str.end()) {
    cerr << "Too many arguments: \"" << font_size_str << "\"\n";
    return false;
  }

  return true;
}

bool arguments::parse_colours() {
  string::const_iterator s = col_str.begin();
  if (!parse_colour(next_bit(col_str, s), col)) return false;
  if (s != col_str.end()) {
    if (!parse_colour(next_bit(col_str, s), label_col)) return false;
  }
  if (s != col_str.end()) {
    cerr << "Too many arguments: \"" << col_str << "\"\n";
    return false;
  }

  return true;
}

bool arguments::parse_rules() {
  for ( string const& arg : rule_strs ) {
    if (arg.empty())
      rules.clear();
    else {
      string::const_iterator s = arg.begin();
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
      rules.push_back(r);
    }
  }
  return true;
}

bool arguments::parse_labels() {
  for ( string const& arg : label_strs ) {
    string::const_iterator s = arg.begin();
    int row_number = -1;
    if (!parse_int(next_bit(arg, s), row_number)) return false;
    printmethod::label l(row_number, string(s, arg.end()));
    labels.push_back(l);
  }
  return true;
}

bool arguments::validate( arg_parser& p ) {
  if (!title.empty() && !parse_title(title))
    return false;
  if (!paper_size.empty() && !parse_paper_size())
    return false;
  if (!custom_lines.empty() && !parse_lines())
    return false;
  if (!font_size_str.empty() && !parse_font_size())
    return false;
  if (!col_str.empty() && !parse_colours())
    return false;
  if (!rule_strs.empty() && !parse_rules())
    return false;
  if (!label_strs.empty() && !parse_labels())
    return false;
  return true;
}

shared_pointer<ostream> make_output(arguments const& args) {
  // Find a stream to write to
  if (!args.output_file.empty()) { // Open the output file
    shared_pointer<ostream> os( new ofstream(args.output_file, ios::binary) );
    if (!os->good())
      throw runtime_error( make_string() << "Can't open output file " 
                                         << args.output_file );
    return os;
  }
  else return shared_pointer<ostream>( &cout, no_delete_helper<ostream>::fn );
}

shared_pointer<printpage> make_printpage(ostream& os, arguments const& args) {
  shared_pointer<printpage> pp;

  switch(args.format) {
    case ps:
      if (args.landscape)
        pp.reset(new printpage_ps(os, args.height));
      else
        pp.reset(new printpage_ps(os));
      break;
    case pdf:
      if (args.landscape)
        pp.reset(new printpage_pdf(os, args.height, args.width, true));
      else
        pp.reset(new printpage_pdf(os, args.width, args.height));
      break;
  }

  return pp;
}

void maybe_rule(printrow &pr, list<printmethod::rule> const& rules, 
                size_t count) {
  for (printmethod::rule const& r : rules) {
    if (r.repeat == -1 && count == r.offset || 
        r.repeat && count % r.repeat == r.offset % r.repeat) {
      pr.rule(r.style, r.flags);
      return;
    }
  }
}

void maybe_label(printrow &pr, list<printmethod::label> const& labels, 
                 printrow::options const& opt, size_t count) {
  for (printmethod::label const& l : labels) {
    if (l.row_number == count)
      pr.text(l.text, opt.xspace*3/2, l.align, false,
              l.align == text_style::left);
  }
}

int main(int argc, char *argv[])
{
  bell::set_symbols_from_env();

  arguments args;
  {
    arg_parser ap(argv[0], "psrows -- print out rows in PostScript or PDF.\v",
                  "OPTIONS");
    args.bind(ap);

    if ( !ap.parse(argc, argv) ) {
      ap.usage();
      return 1;
    }

    if ( !args.validate(ap) ) 
      return 1;
  }


  try {
    shared_pointer<ostream> os( make_output(args) );
    shared_pointer<printpage> pp( make_printpage(*os, args) );

    printrow::options opt;  opt.defaults();
    if (!args.custom_lines.empty())
      opt.lines = args.lines;
    if (!args.font.empty()) opt.style.font = args.font;
    if (args.font_size) opt.style.size = args.font_size;
    if (args.label_font_size) opt.label_style.size = args.label_font_size;
    opt.style.col = args.col;
    opt.label_style.col = args.label_col;
    if (args.xspace) opt.xspace = args.xspace;
    if (args.yspace) opt.yspace = args.yspace;

    printrow pr(*pp, opt);

    size_t columnset = 0, rows_per_column = args.length;
    dimension vgap = opt.yspace;
    dimension xoffset = opt.xspace / 2;
    dimension yoffset = (opt.yspace * (args.length * 2 + 3)) / 2;
    pr.set_position(xoffset, yoffset);
    pr.move_position(0, -opt.yspace * columnset * (rows_per_column + 1));
    pr.move_position(0, -vgap * columnset);

    size_t count = 0;
    while (cin) {
      row r;
      cin >> r;
      pr << r;

      maybe_rule(pr, args.rules, count);
      maybe_label(pr, args.labels, opt, count);

      ++count;
    }
  }
  catch(exception& e) {
    cerr << argv[0] << ": " << e.what() << endl;
    return 1;
  }
  return 0;
}
