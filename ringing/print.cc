// Printing stuff

#ifdef __GNUG__
#pragma implementation
#endif

#include "print.h"

void printrow::options::defaults()
{
  flags = numbers;
  style.size = 10;
  style.font = "Helvetica";
  xspace.n = 12; xspace.d = 1; xspace.u = dimension::points; 
  yspace.n = 12; yspace.d = 1; yspace.u = dimension::points;
  line_style s; s.width.n = 1; s.width.d = 2; s.width.u = dimension::points;
  lines[1] = s;
}

printrow::printrow(printpage& pp)
{ 
  options o; o.defaults();
  pr = pp.new_printrow(o); 
}

printrow::printrow(printpage& pp, const options& o)
{ 
  pr = pp.new_printrow(o); 
}
