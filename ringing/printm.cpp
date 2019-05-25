// printm.cpp - Printing whole methods
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/printm.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

void printmethod::defaults()
{
  opt.defaults();
  hgap = opt.xspace * 3;
  vgap = opt.yspace;

  if(!m) return;

  row r = m->lh();
  int l = r.order();
  total_rows = m->length() * l;
  rows_per_column = m->length();
  columns_per_set = l;
  sets_per_page = 0;
  xoffset = opt.xspace / 2;
  yoffset = (opt.yspace * (m->length() * 2 + 3)) / 2;
  number_mode = miss_lead;
  pn_mode = pn_first;
  reverse_placebells = placebells_at_rules = false;

  // Set up the lines
  change c = (*m)[m->length() - 1];
  printrow::options::line_style huntstyle, workstyle;
  workstyle.width.n = 1; workstyle.width.d = 2; 
  workstyle.width.u = dimension::points;
  workstyle.col.grey = false; workstyle.col.red = 0; 
  workstyle.col.green = 0; workstyle.col.blue = 1.0;
  workstyle.crossing = false;
  huntstyle.width.n = 1; huntstyle.width.d = 4;
  huntstyle.width.u = dimension::points;
  huntstyle.col.grey = false; huntstyle.col.red = 1.0;
  huntstyle.col.green = 0; huntstyle.col.blue = 0;
  huntstyle.crossing = false;

  opt.lines.clear();
  bool found_working_bell = false;
  for(bell b(0); b < m->bells(); ++b) {
    if(r[b] == b)
      opt.lines[b] = huntstyle;
    else if(!found_working_bell && c.findplace(b)) {
      opt.lines[b] = workstyle;
      placebells = b;
      found_working_bell = true;
    }
  }
  if(!found_working_bell)
    for(bell b(0); b < m->bells(); ++b)
      if(r[b] != b) {
	opt.lines[b] = workstyle;
	placebells = b;
	break;
      }
}

bool printmethod::needrule(int i)
{
  list<pair<int,int> >::const_iterator j;
  for(j = rules.begin(); j != rules.end(); j++)
    if((*j).second) {
      if(((i+1) % (*j).second) == ((*j).first % (*j).second)) return true;
    } else {
      if((i+1) == (*j).first) return true;
    }
  return false;
}

void printmethod::print(printpage& pp)
{
  if(!m) return;

  if (rounds.bells()!=m->bells())
    rounds=row::rounds(m->bells());

  row_block b(*m,rounds);
  int i = 0;
  int total_row_count = 0;
  int ic = 0; // calling position count
  int column, columnset, row_count;
  bool sym = m->issym();
  bool pn;
  int pnextra = 0;

  if(number_mode == miss_always) 
    opt.flags |= printrow::options::miss_numbers;
  else
    opt.flags &= ~printrow::options::miss_numbers;
  pn = ((pn_mode & pn_mask) != pn_none);
  if(pn) pnextra = find_pnextra();

  for(columnset = 0; total_row_count < total_rows; columnset++) {
    if(sets_per_page && (columnset == sets_per_page)) {
      pp.new_page();
      columnset = 0;
    }
    {
      printrow pr(pp, opt);
      // Move to the beginning of this set of columns
      pr.set_position(xoffset, yoffset);
      pr.move_position(0, -opt.yspace * columnset * (rows_per_column + 1));
      pr.move_position(0, -vgap * columnset);
      for(column = 0; column < columns_per_set 
	    && total_row_count < total_rows; column++) {
	// Print the first row, which is the same as the last row of the
	// previous column.
	pr << b[i]; if(needrule(i % (b.size()-1))) {
          pr.rule();
        }
	// Turn on number-missing if necessary
	if(number_mode == miss_column) {
	  opt.flags |= printrow::options::miss_numbers;
	  pr.set_options(opt);
	}
	for(row_count = 0; row_count < rows_per_column 
	      && total_row_count < total_rows; row_count++) {
	  ++i;
	  if(i == (int) b.size()) { // We've got to the end of a lead
	    b[0] = b[b.size() - 1]; b.recalculate();
	    // Miss the first row of the lead - we've already printed it
	    // at the end of the previous lead.
	    i = 1;
	    if((pn_mode & pn_mask) == pn_first 
               || (pn_mode & pn_mask) == pn_first_asym) pn = false;
	  }
	  if(i == 1) {
	    // Turn number-missing back on
	    if(number_mode == miss_lead) {
	      opt.flags |= printrow::options::miss_numbers;
	      pr.set_options(opt);
	    }
	    if(!placebells_at_rules && placebells >= 0) {// Print place bell
	      pr.placebell(placebells, reverse_placebells ? +1 : 0);
	      if(!(opt.flags & printrow::options::numbers))
	        pr.dot(-1);
	    }
	  } else if(i == (int) (b.size() - 2)) {
	    // print calling positions
	    if (char pos=call(ic++)) {
	      pr.text(string("-")+pos,opt.xspace/2,text_style::left,false,true);
//	      pr.text(string("-")+pos,0,text_style::left,false,true);
	    }
	  }
	  else if(i == (int) (b.size() - 1)) {
	    if(number_mode == miss_lead) {
	      // Lead head - print the numbers
	      opt.flags &= ~printrow::options::miss_numbers;
	      pr.set_options(opt);
	    }
            if (!placebells_at_rules && reverse_placebells && 
	        row_count != total_rows - 1) {
              if(placebells >= 0)
 	        pr.placebell(placebells, -1);
	      if(!(opt.flags & printrow::options::numbers))
	        pr.dot(-1); 
            }
	  }
	  if(number_mode == miss_column && row_count == rows_per_column - 1) {
	    // Last row of a column - print the numbers
	    opt.flags &= ~printrow::options::miss_numbers;
	    pr.set_options(opt);
	  }
	  if(pn && ((pn_mode & pn_mask) == pn_all 
		    || !sym || (pn_mode & pn_mask) == pn_first_asym 
                    || i <= (m->length()+1)/2 || i == m->length())
	     && !((pn_mode & pn_nox) && (*m)[i-1].count_places() == 0)) 
	    pr.text((*m)[i-1].print(), opt.xspace,text_style::right, 
		    true, false);
	  pr << b[i];
	  if(row_count < (rows_per_column - 1)
	     && total_row_count < (total_rows - 1)
	     && needrule(i % (b.size()-1))) {
	    pr.rule();
	    if(placebells_at_rules && reverse_placebells && placebells >= 0)
	      pr.placebell(placebells, -1);
	  }
	  if(placebells_at_rules && placebells >= 0 &&
	     row_count < (rows_per_column - 1)
	     && total_row_count < (total_rows - 1)
	     && needrule((i-1) % (b.size()-1))) 
	      pr.placebell(placebells, reverse_placebells ? +1 : 0);
	  total_row_count++;
	}
	if(total_row_count < total_rows) { // Next column
	  pr.move_position(opt.xspace * m->bells(), 0);
	  pr.move_position(hgap, 0);
	}
      }
    }
  }
}

int printmethod::find_pnextra()
{
  int result = 0;
  method::const_iterator i; int k;
  for(i = m->begin(); i != m->end(); i++) {
    k = (*i).count_places();
    if(k > result) result = k;
  }
  return (result+3)/2;
}

void printmethod::scale_to_space(const dimension& width,
                                 const dimension& height,
                                 float aspect,
                                 int pnextra)
{
  float new_xspace, vlimit;

  if((pn_mode & pn_mask) != pn_none && pnextra == -1) 
    pnextra = find_pnextra();
  new_xspace = width.in_points() 
    / (columns_per_set * (m->bells() + ((placebells >= 0) ? 3 : 1)
			  + (((pn_mode & pn_mask) == pn_all) ? pnextra : 0)) 
       + (((pn_mode & pn_mask) == pn_first 
            || (pn_mode & pn_mask) == pn_first_asym ) ? pnextra : 0) - 1);
  vlimit = height.in_points() * aspect / 
    ((rows_per_column + 3) * sets_per_page - 2);
  if(vlimit < new_xspace) new_xspace = vlimit;
  opt.xspace.set_float(new_xspace, 100);
  opt.yspace.set_float(new_xspace / aspect, 100);
  hgap = opt.xspace * (((placebells >= 0) ? 3 : 1) + 
		       (((pn_mode & pn_mask) == pn_all) ? pnextra : 0));
  vgap = opt.yspace * 2;
  opt.style.size = static_cast<int>(opt.yspace.in_points() * 9);
}  

void printmethod::fit_to_space(const dimension& width, 
				const dimension& height, bool vgap_mode, 
				float aspect)
{
  if(!m) return;

  float curr_xspace = -1, new_xspace = 0;
  int leads_per_column;
 
  // Work out how many bells' worth of space to leave for place notation
  int pnextra = -1;
  if((pn_mode & pn_mask) != pn_none) pnextra = find_pnextra();

  int lead = m->length();
  // We try each possible number of leads per column until
  // the required size starts going down rather than up.
  for(leads_per_column = 1; new_xspace > curr_xspace; leads_per_column++) {
    curr_xspace = new_xspace;
    if(vgap_mode) {
      rows_per_column = lead;
      sets_per_page = leads_per_column;
    } else {
      rows_per_column = leads_per_column * lead;
      sets_per_page = 1;
    }
    columns_per_set = divu(total_rows, rows_per_column * sets_per_page);
    scale_to_space(width, height, aspect, pnextra);
    new_xspace = opt.xspace.in_points();
  }

  leads_per_column -= 2;
  if(vgap_mode) {
    rows_per_column = lead;
    sets_per_page = leads_per_column;
  } else {
    rows_per_column = leads_per_column * lead;
    sets_per_page = 1;
  }
  columns_per_set = divu(total_rows, rows_per_column * sets_per_page);
  scale_to_space(width, height, aspect, pnextra);
}

float printmethod::total_width() {
  int columns = divu(total_rows, rows_per_column);
  if(columns_per_set < columns) columns = columns_per_set;
  return (opt.xspace.in_points() * (m->bells() * columns + 
				    (((pn_mode & pn_mask) != pn_none) 
				     ? find_pnextra() : 0)))
	  + (hgap.in_points() * (columns - 1))
	  + ((placebells >= 0) ? opt.style.size * 0.035f : 0);
}
 
float printmethod::total_height() {
  int columns = divu(total_rows, rows_per_column);
  if(columns_per_set < columns) columns = columns_per_set;
  int columnsets = divu(total_rows, rows_per_column * columns);
  if(sets_per_page && (sets_per_page < columnsets)) 
    columnsets = sets_per_page;
  return (opt.yspace.in_points() * (rows_per_column + 1) * columnsets)
    + (vgap.in_points() * (columnsets - 1))
    + ((placebells >= 0) ? opt.style.size * 0.035f : 0);
}

void printmethod::get_bbox(float& blx, float& bly, float& urx, float& ury)
{
  int columns = divu(total_rows, rows_per_column);
  if(columns_per_set < columns) columns = columns_per_set;
  blx = -opt.xspace.in_points() 
    * (0.5f + (((pn_mode & pn_mask) != pn_none) ? find_pnextra() : 0));
  urx = opt.xspace.in_points() * (m->bells() * columns - 0.5f
				  + ((placebells >= 0) ? 1.9f : 0))
    + (hgap.in_points() * (columns - 1));
 
  int columnsets = divu(total_rows, rows_per_column * columns);
  if(sets_per_page && (sets_per_page < columnsets)) 
    columnsets = sets_per_page;
  bly = -(opt.yspace.in_points() * ((rows_per_column + 1) * columnsets - 0.5f))
    - (vgap.in_points() * (columnsets - 1));
  ury = ((placebells >= 0) ? opt.style.size * 0.07f : 0);
  if(ury < opt.yspace.in_points() * 0.5f)
    ury = opt.yspace.in_points() * 0.5f;
}

char printmethod::call(size_t l) const {
	if (l<calls.length()&&calls[l]!=' ')
		return calls[l];
	else
		return 0;
}

RINGING_END_NAMESPACE

