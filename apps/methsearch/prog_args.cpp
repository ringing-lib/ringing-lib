// -*- C++ -*- prog_args.cpp - handle program arguments
// Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2013
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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/row.h>
#include <ringing/streamutils.h>
#include <ringing/xmlout.h>
#include "args.h"
#include "prog_args.h"
#include "libraries.h"
#include "mask.h"
#include "format.h"
#include "music.h"
#include "methodutils.h"
#include "row_calc.h"
#if RINGING_OLD_INCLUDES
#include <algorithm.h>
#include <iterator.h>
#else
#include <algorithm>
#include <iterator>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#else
#include <cassert>
#endif
#include <iostream>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class falseness_opt : public option
{
public:
  falseness_opt( char c, const string &l, const string &d, const string &a,
		 arguments &args )
    : option( c, l, d, a ), args(args)
  {}

private:
  arguments &args;
  virtual bool process( const string &arg, const arg_parser & ) const;
};

bool falseness_opt::process( const string &arg, const arg_parser & ) const
{
  // Single letter options used:  c, e, h, l, n, r, s, x, P

  if ( arg == "CPS" )
    {
      args.true_trivial = args.true_half_lead 
	= args.true_lead = args.true_course = args.require_CPS = true;
    }
  else if ( arg == "e" || arg == "extent" )
    {
      args.true_trivial = args.true_half_lead 
	= args.true_lead = args.true_extent = true;
    }
  else if ( arg == "e+" || arg == "positive-extent" )
    {
      args.true_trivial = args.true_half_lead 
	= args.true_lead = args.true_positive_extent = true;
    }
  else if ( arg == "c" || arg == "course" )
    {
      args.true_trivial = args.true_half_lead 
	= args.true_lead = args.true_course = true;
    }
  else if ( arg == "l" || arg == "lead" )
    {
      args.true_trivial = args.true_half_lead = args.true_lead = true;
      args.true_course = false;
    }
  else if ( arg == "h" || arg == "half-lead" )
    {
      args.true_trivial = args.true_half_lead = true;
      args.true_lead = args.true_course = false;
    }
  else if ( arg == "n" || arg == "none" ) 
    {
      args.true_trivial = true;
      args.true_half_lead = args.true_lead = args.true_course = false;
    }
  else if ( arg == "x" || arg == "really-none" )
    {
      args.true_trivial = args.true_half_lead
	= args.true_lead = args.true_course = false;
    }
  else if ( arg.size() && arg[0] == ':' )
    {
      string fgs = arg.substr(1, string::npos);
      while (true) { 
        size_t i = fgs.find('-');  
        if ( i == string::npos ) 
          break;
        else if ( i == 0 || i >= fgs.size() - 1 ||
                  fgs[i-1] >= fgs[i+1] ||
                  // Need both lower case, or both upper case
                  ! ( islower( fgs[i-1] ) && islower( fgs[i+1] ) ||
                      isupper( fgs[i-1] ) && isupper( fgs[i+1] ) ) ) { 
          cerr << "Unable to parse falseness group range in -F option: \""
               << fgs << "\"\n";
          return false;
        } 
        string range; for ( char c=fgs[i-1]; c != fgs[i+1]; ++c ) range += c;
        fgs.replace( i, 1, range );
      }
      args.allowed_falseness = fgs;
    }
  else if ( arg.size() > 2 && arg[0] == 's' && arg[1] == '=' )
    {
      if ( args.start_row.bells() ) {
        cerr << "Multiple -Fs options are not currently supported\n";
        return false;
      }
      try {
        args.start_row = row( arg.substr(2) );
      }
      catch ( exception const& e ) { 
        cerr << "Error parsing row in -Fs option: " << e.what() << "\n";
        return false;
      }
    }
  else if ( arg.size() > 2 && arg[0] == 'r' && arg[1] == '=' )
    {
      row_calc::flags rcf = static_cast<row_calc::flags>
        ( row_calc::allow_implicit_treble | row_calc::allow_row_promotion );
      scoped_pointer<row_calc> rc;
      try {
        rc.reset( new row_calc( 0, arg.substr(2), rcf ) );
      } 
      catch ( exception const& e ) { 
        cerr << "Error parsing row in -Fr option: " << e.what() << "\n";
        return false;
      }
      try {
        for ( row_calc::const_iterator i=rc->begin(), e=rc->end(); i!=e; ++i ) 
          args.avoid_rows.insert( *i );
      }
      catch ( row::invalid const& e ) { 
        cerr << "Invalid row produced in -Fr option: " << e.what() << "\n";
        return false;
      }
    }
  else if ( arg.size() > 2 && arg[0] == 'P' && arg[1] == '=' )
    {
      row_calc::flags rcf = static_cast<row_calc::flags>
        ( row_calc::allow_implicit_treble | row_calc::allow_row_promotion );
      scoped_pointer<row_calc> rc;
      try {
        rc.reset( new row_calc( 0, arg.substr(2), rcf ) );
      } 
      catch ( exception const& e ) { 
        cerr << "Error parsing row in -FP option: " << e.what() << "\n";
        return false;
      }
      try {
        for ( row_calc::const_iterator i=rc->begin(), e=rc->end(); i!=e; ++i ) 
          args.pends_generators.push_back( *i );
      }
      catch ( row::invalid const& e ) { 
        cerr << "Invalid row produced in -FP option: " << e.what() << "\n";
        return false;
      }
    }
  else
    {
      cerr << "Unknown -F option: \"" << arg << "\"\n";
      return false;
    }

  return true;
}


arguments::arguments( int argc, char* argv[] )
{
  arg_parser ap(argv[0],
    "methsearch -- find methods with particular properties.", 
		  "OPTIONS" );
    
  bind( ap );
    
  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(1);
  }

  if ( !validate( ap ) ) 
    exit(1);
}

// Unused letters: -aghitvxzBDJKNWXY.  Plunder with caution

void arguments::bind( arg_parser &p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
	 ( 'b', "bells",  
	   "The number of bells.  This option is required", "BELLS",
	   bells ) );

  p.add( new boolean_opt
	 ( 'c', "cyclic", 
	   "Require cyclic lead ends",
	   require_cyclic_les ) );

  p.add( new boolean_opt
	 ( 'r', "regular",
	   "Require regular (P.B.) lead ends",
	   require_pbles ) );

  p.add( new boolean_opt
	 ( 'A', "all-methods",    
	   "Include differentials or differential hunters",
	   show_all_meths ) );

  p.add( new integer_opt
	 ( 'p', "blows-per-place", 
	   "At most NUM consecutive blows in one place (0 = unlimited)", "NUM",
	   max_consec_blows, /* default = */ 2 ) );

  p.add( new integer_opt
	 ( 'l', "places-per-change", 
	   "At most NUM places in any change (0 = unlimited)", "NUM",
	   max_places_per_change, /* default = */ 2 ) );

  p.add( new boolean_opt
	 ( '\0', "long-le-place", 
	   "Allow a long place across the lead-end", 
	   long_le_place ) );

  p.add( new integer_opt
	 ( 'n', "changes-per-lead", 
	   "Require NUM changes per lead (0 = automatic)", "NUM",
	   lead_len ) );

  p.add( new boolean_opt
	 ( 'q', "quiet",  
	   "Do not display methods when found",
	   quiet ) );

  p.add( new string_opt
	 ( 'H', "frequencies", 
	   "Count frequencies of different method properties", "FMT", 
	   H_fmt_str ) );

  p.add( new string_opt
	 ( 'R', "format", 
	   "Use FMT to format methods as found", "FMT",
	   R_fmt_str ) );

  p.add( new strings_opt
	 ( 'Q', "require", 
	   "Require EXPR to be true", "EXPR",
	   require_strs ) );

  p.add( new boolean_opt
	 ( 'e', "restricted-le",
	   "Only allow 12 and 1N (or 1 and 12N) lead ends",
	   require_limited_le ) );

  p.add( new boolean_opt
	 ( 'E', "prefer-restricted-le",
	   "Prefer 12 and 1N (or 1 and 12N) lead ends",
	   prefer_limited_le ) );

  p.add( new boolean_opt
	 ( 'w', "right-place", 
	   "Require right place methods",
	   right_place ) );

  p.add( new boolean_opt
	 ( 'f', "no-78s", 
	   "Prohibit 78s (on 8 bells) in the place notation",
	   no_78_pns ) );

  p.add( new integer_opt
	 ( 'j', "max-adj-places",
	   "Forbid methods with more than NUM adjacent places", "NUM",
	   max_consec_places, /* default = */ 1 ) );

  p.add( new boolean_opt
	 ( 'y', "symmetric-sections",
	   "Require each section to be symmetric", 
	   sym_sects, /* default = */ 1 ) );

  p.add( new boolean_opt
	 ( 's', "symmetric", 
	   "Look for palindromically symmetric methods (default)",
	   sym ) );

  p.add( new boolean_opt
	 ( 'k', "rotational", 
	   "Look for rotationally symmteric methods 'Brave New Worlds' "
	   "symmetry",
	   skewsym ) );

  p.add( new boolean_opt
	 ( 'd', "double", 
	   "Look for double methods - i.e. 'glide' symmetry",
	   doubsym ) );

  p.add( new boolean_opt
         ( '\0', "mirror",
           "Look for methods with mirror symmetry", 
           mirrorsym ) );

  p.add( new boolean_opt
         ( '\0', "floating-sym",
           "Allow the symmetry points in arbitrary places",
           floating_sym ) );

  p.add( new boolean_opt
	 ( 'S', "surprise", 
	   "Require an internal place between dodging positions",
	   surprise ) );

  p.add( new boolean_opt
	 ( 'T', "treble-bob", 
	   "Forbid an internal place between dodging positions",
	   treble_bob ) );

  p.add( new boolean_opt
         ( '\0', "delight",
           "Require internal places between some but not all dodging positions",
           delight ) );

  p.add( new boolean_opt
         ( '\0', "3rds-place-delight",
           "For minor only, require thirds as the treble moves between 4-5",
           delight3 ) );

  p.add( new boolean_opt
         ( '\0', "4ths-place-delight",
           "For minor only, require fourths as the treble moves between 2-3",
           delight4 ) );

  p.add( new boolean_opt
         ( '\0', "strict-delight",
           "Require internal places between all but one dodging positions",
           strict_delight ) );

  p.add( new boolean_opt
         ( '\0', "exercise",
           "Require internal places between all but two or fewer dodging "
           "positions", exercise ) );

  p.add( new boolean_opt
         ( '\0', "strict-exercise",
           "Require internal places between exactly all but two dodging "
           "positions", strict_exercise ) );

  p.add( new boolean_opt
         ( '\0', "pas-alla-tria",
           "Require internal places between exactly all but three dodging "
           "positions", pas_alla_tria ) );

  p.add( new boolean_opt
         ( '\0', "pas-alla-tessera",
           "Require internal places between exactly all but four dodging "
           "positions", pas_alla_tessera ) );

  p.add( new integer_opt
	 ( 'G', "treble-dodges", 
	   "Look for methods where the treble dodges NUM times in each "
	   "position", "NUM",
	   treble_dodges ) );

  p.add( new integer_opt
	 ( 'U', "hunts", 
	   "Look for methods with NUM hunt bells", "NUM",
	   hunt_bells, 0 ) );

  p.add( new string_opt
         ( 'Z', "treble-path",
           "Look for methods with the specified path", "PATH",
            treble_path ) );

  p.add( new boolean_opt
	 ( 'u', "status",
	   "Display the current status",
	   status ) );

  p.add( new integer_opt
	 ( '\0', "status-freq",
	   "Display the current status every NUM nodes", "NUM",
	   status_freq ) );

  p.add( new integer_opt
	 ( '\0', "limit",
	   "Limit the search to the first NUM methods", "NUM",
	   search_limit ) );

  p.add( new boolean_opt
	 ( '\0', "random",
	   "Randomise the order in which methods are listed",
	   random_order ) );

  p.add( new integer_opt
	 ( '\0', "loop",
	   "Repeatedly search for one random method NUM times", "NUM",
	   random_count, -1 ) );

  p.add( new integer_opt
	 ( '\0', "seed",
	   "Seed the random number generator with NUM", "NUM",
	   random_seed ) );

  p.add( new boolean_opt
	 ( 'C', "count",
	   "Count the number of methods found",
	   count ) );

  p.add( new boolean_opt
	 ( '\0', "raw-count",
	   "Count the number of methods found and print it without "
	   "surrounding text",
	   raw_count ) );

  p.add( new boolean_opt
         ( '\0', "node-count",
           "Count the number of search nodes visited",
           node_count ) );

  p.add( new boolean_opt
         ( 'I', "filter",
           "Act as a filter on standard input rather than searching",
           filter_mode ) );

  p.add( new boolean_opt
         ( '\0', "filter-lib",
           "Act as a filter on specified libraries rather than searching",
           filter_lib_mode ) );

  p.add( new boolean_opt
         ( '\0', "invert-filter",
           "Invert filter so only non-matching methods are listed",
           invert_filter ) );

  p.add( new integer_opt
         ( '\0', "timeout",
           "Time the search out after NUM seconds", "NUM",
           timeout ) );

  p.add( new boolean_opt
	 ( 'P', "parity-hack",
	   "Require an equal number of rows of each parity for each place "
	   "in the treble's path",
	   same_place_parity ) );

  p.add( new delegate_opt
	 ( 'L', "library",
	   "Look up method names in the library LIB", "LIB",
	   &method_libraries::add_new_library ) );

  p.add( new falseness_opt
	 ( 'F', "falseness",
	   "Check for falseness", "TYPE",
	   *this ) );

  p.add( new string_opt
	 ( 'm', "mask",
	   "Require that the method matches the given mask", "PATTERN",
	   mask ) );

  p.add( new string_opt
         ( '\0', "changes",
           "Use only changes from the given list", "CHANGES",
           changes_str ) );

  p.add( new delegate_opt
	 ( 'M', "music",
	   "Score the music in a plain course", "PATTERN",
	   &musical_analysis::add_pattern ) );

  p.add( new string_opt
	 ( '\0', "start-at",
	   "Start the search with the method PN", "PN",
	   startmethstr ) );

  p.add( new string_opt
         ( '\0', "prefix",
           "Require the method to start with PN", "PN",
           prefixstr ) );

  p.add( new boolean_opt
	 ( '\0', "cyclic-hle", 
	   "Require cyclic half-lead ends",
	   require_cyclic_hle ) );

  p.add( new boolean_opt
	 ( '\0', "cyclic-hlh", 
	   "Require cyclic half-lead heads",
	   require_cyclic_hlh ) );

  p.add( new boolean_opt
	 ( '\0', "rev-cyclic-hle", 
	   "Require reverse cyclic half-lead ends",
	   require_rev_cyclic_hle ) );

  p.add( new boolean_opt
	 ( '\0', "rev-cyclic-hlh", 
	   "Require reverse cyclic half-lead heads",
	   require_rev_cyclic_hlh ) );

  p.add( new boolean_opt
	 ( '\0', "regular-hls", 
	   "Require regular half-leads",
	   require_reg_hls ) );

  p.add( new boolean_opt
	 ( '\0', "offset-cyclic", 
	   "Require offset cyclicity",
	   require_offset_cyclic ) );

  p.add( new string_opt
	 ( 'o', "out-file",
	   "Output to file FILENAME", "FILENAME",
	   outfile ) );

  p.add( new string_opt
	 ( 'O', "out-format",
	   "Create as a FMTTYPE library ", "FMTTYPE",
	   outfmt ) );
}

bool arguments::validate( arg_parser &ap )
{
  if ( bells == 0 ) 
    {
      ap.error( "Must specify the number of bells" );
      return false;
    }

  if ( bells < 2 || bells > int(bell::MAX_BELLS) ) 
    {
      ap.error( make_string() << "The number of bells must be between 2 and " 
		<< bell::MAX_BELLS << " (inclusive)" );
      return false;
    }

  if ( prefer_limited_le && require_limited_le )
    {
      ap.error( "The -e and -E options are mutually exclusive" );
      return false;
    }

  if ( hunt_bells % 2 == 0 && (require_limited_le || prefer_limited_le) )
    {
      ap.error( "Lead ends can only be limited with an odd number of "
		"hunt bells");
      return false;
    }

  if ( !hunt_bells && !lead_len && !( filter_mode || filter_lib_mode ) )
    {
      ap.error( "Must specify the lead length when searching for principles" );
      return false;
    }

  if ( hunt_bells && lead_len )
    {
      ap.error( "Must not specify the lead length when searching for methods "
		"with a hunt bell" );
      return false;
    }

  if ( treble_path.size() && !hunt_bells ) 
    {
      ap.error( "Cannot specify a treble path without no hunts" );
      return false;
    }

  {
    treble_front = 1;
    treble_back  = bells;

    string::size_type i = treble_path.find('-');
    if ( i == string::npos ) 
      { 
        if ( treble_path.size() )
          {
            ap.error( "Treble path must be a range: FRONT-BACK" );
            return false;
          }
      }
    else
      {
        try {
          if ( i != 0u )
            treble_front = lexical_cast<bell>( treble_path.substr(0,i) ) + 1; 

          if ( i != treble_path.size()-1 )
            treble_back  = lexical_cast<bell>( treble_path.substr(i+1) ) + 1;
        } 
        catch ( bad_lexical_cast const& ) {
          ap.error( "Unable to parse arguments to -Z range as bell symbols" );
          return false;
        }

        if ( treble_back > bells || treble_front > bells ||
             treble_back < 0 || treble_front < 0 ) 
          { 
            ap.error( "Treble path out of range" );
            return false;
          }
        else if ( treble_back < treble_front )
          {
            ap.error( "Treble path range is of negative size" );
            return false;
          }
      }
  }

  if ( !lead_len && hunt_bells )
    lead_len = (1 + treble_dodges) * (treble_back - treble_front + 1) * 2;

  if ( hunt_bells >= bells ) 
    {
      ap.error( make_string() << "Method must have fewer than " 
		<< bells-1 << " hunt bells" );
      return false;
    }

  if ( treble_dodges && ! hunt_bells )
    {
      ap.error( "Treble dodging methods require at least one hunt bell"  );
      return false;
    }

  if ( surprise + treble_bob + delight + delight3 + delight4 + strict_delight +
       exercise + strict_exercise + pas_alla_tria + pas_alla_tessera >= 2 ) 
    {
      ap.error( "The surprise, treble bob and delight options are "
                "mututally exclusive");
      return false;
    }

  if ( ( delight3 || delight4 ) && treble_back != 6 && treble_front != 1 ) 
    {
      ap.error( "3rds and 4ths place delight methods are specific to minor" );
      return false;
    }

  if ( !sym && ( strict_delight || exercise || strict_exercise || 
                 pas_alla_tria || pas_alla_tessera ) )
    {
      ap.error( "Historical delight, exercise, pas-alla-tria and "
                "pas-alla-tessera classes are only well-defined for "
                "palindromic methods" );
      return false;
    }

  if ( surprise || treble_bob || delight || delight3 || delight4 ||   
       strict_delight || exercise || strict_exercise || pas_alla_tria || 
       pas_alla_tessera ) 
    {
      if ( ! hunt_bells )   
        {
          ap.error( "Surprise, treble bob or delight methods require at "
                    "least one hunt bell"  );
          return false;
        }
      if ( (treble_back-treble_front+1) % 2 == 1 ) 
        {
          ap.error( "Surprise, treble bob or delight methods can only be "
		    "found when the treble hunts an even number of places "
                    "(typically an even-bell method)"  );
          return false;
        }
    }

  if ( treble_dodges && (treble_back-treble_front+1) % 2 == 1 )
    {
      ap.error( "Treble dodging methods can only be found when "
		"the treble hunts an even number of places (typically an "
                "even-bell method)"  );
      return false;
    }

  if ( (treble_dodges || hunt_bells > 1) && true_extent )
    {
      ap.error( "-Fe is only implemented for single-hunt plain methods" );
      return false;
    }

  if ( !hunt_bells && bells % 2 && right_place )
    {
      ap.error( "Odd bell methods need at least one hunt bell to be "
		"right place" );
      return false;
    }
  
  if ( !hunt_bells && require_pbles )
    {
      ap.error( "Methods with no hunt bells cannot have plain bob "
		"lead heads" );
      return false;
    }

  if ( (require_cyclic_hlh || require_cyclic_hle || 
	require_rev_cyclic_hlh || require_rev_cyclic_hle)
       && hunt_bells != 1 )
    {
      ap.error( "Cyclic and reverse-cyclic half-leads are only "
		"implemented for single hunt methods" );
      return false;
    }
  if ( (require_cyclic_hlh || require_cyclic_hle ||
        require_rev_cyclic_hlh || require_rev_cyclic_hle ||
        require_reg_hls) && (treble_front != 1 || treble_back != bells) )
    {
      ap.error( "Half leads cannot be restricted for little methods" );
      return false;
    }

  if ( (require_cyclic_les || require_pbles) && treble_front != 1 )
    {
      ap.error( "Lead-ends cannot be restricted if the treble is not the "
                "hunt bell" );
      return false;
    }

  if ( same_place_parity && treble_dodges % 2 == 0 )
    {
      ap.error( "The `parity-hack' option is meaningful for"
		" methods with an odd number of dodges (e.g. -G1)" );
      return false;
    }

  if ( same_place_parity && !sym )
    {
      ap.error( "The `parity-hack' option is not currently"
		" implemented with asymmetrical methods" );
      return false;
    }

  if ( require_offset_cyclic && !treble_dodges && hunt_bells )
    {
      ap.error( "The `offset-cyclic' option requires a treble-dodging"
		" path (e.g. -G1) or a principle (e.g. -U0)" );
      return false;
    }

  if ( require_reg_hls && (!require_pbles || !hunt_bells == 1))
    {
      ap.error( "The `regular-hls' option requires a single-hunt regular "
		"lead head" );
      return false;
    }

  if ( outfmt == "xml" && R_fmt_str.size() )
    {
      ap.error( "The -Oxml option cannot be used with the -R option" );
      return false;
    }

  if ( random_count > 0 && !random_order ) {
    ap.error( "--loop is only supported with --random" );
    return false;
  }
  if ( random_order > 0 && ( filter_mode || filter_lib_mode ) ) {
    ap.error( "--random cannot be used when filtering" );
    return false;
  }
  if ( random_count > 0 && search_limit > 0 ) {
    ap.error( "--limit cannot be used when --loop is given an argument" );
    return false;
  }

  if ( outfmt == "utf8" ) 
    set_formats_in_unicode( true );

  if (!quiet) {
    try {
      if ( outfmt.empty() || outfmt == "fmt" || outfmt == "utf8" ) {
	outfmt.erase();
	if ( R_fmt_str.empty() )
          R_fmt_str = filter_mode ? "$p\t$a" : "$p\t$l";
	outputs.add( new fmtout( R_fmt_str, outfile ) );
      } 
      else if ( outfmt == "xml" ) {
        if ( outfile.empty() || outfile == "-" ) { 
          ap.error( make_string() << "XML output cannot be specified without "
                    "an output file name (with -o)" );
          return false;
        }
        try {
	  outputs.add( new xmlout( outfile ) );
        } 
        catch ( exception const& ex ) {
          ap.error( make_string() << "Unable to produce XML output: " 
                    << ex.what() ); 
          return false;
        }
      }
      else {
	ap.error( "Unknown -O format: "
                  "must be one of \"xml\", \"fmt\" or \"utf8\"" );
	return false;
      }
    }
    catch ( const argument_error &error ) {
      ap.error( make_string() << "Error parsing -R format: " << error.what() );
      return false;
    }
  }  // end if (quiet)

  try
    {
      if ( (histogram = !H_fmt_str.empty()) )
	outputs.add( new statsout( H_fmt_str ) );
    }
  catch ( const argument_error &error )
    {
      ap.error( make_string() << "Error parsing -H format: " << error.what() );
      return false;
    }


  for ( vector<string>::const_iterator 
          i = require_strs.begin(), e = require_strs.end(); i != e; ++i )
    {
      try
	{
	  require_expr_idxs.push_back( format_string::parse_requirement(*i) );
	}
      catch ( const argument_error &error )
	{
	  ap.error( make_string() << "Error parsing --require argument: "
		    << error.what() );
	  return false;
	}
    }

  if ( (skewsym || sym || doubsym) && lead_len % 2 ) {
    ap.error( "Symmetry is not supported for methods with an odd number of "
              "rows per lead" );
    return false;
  }

  if ( skewsym + sym + doubsym >= 2 )
    skewsym = sym = doubsym = true;

  if ( (skewsym || doubsym || mirrorsym) 
       && (bells - treble_back != treble_front-1) )
    {
      ap.error( "Double, rotational or mirror symmetry require the treble "
                "path to invariant under this symmetry" );
      return false;
    }

  if ( hunt_bells & floating_sym )
    {
      ap.error( "--floating-sym is not supported with hunt bells" );
      return false;
    }

  if ( lead_len && formats_max_lead_offset() > lead_len ) 
    {
      ap.error( "Format contains a $r or $h offset beyond the end of the "
                "lead" );
      return false;
    }

  if ( formats_have_falseness_groups() )
    {
      if ( bells % 2 || hunt_bells > 1 || !require_pbles
	   || !sym || show_all_meths )
	{
	  ap.error( "Falseness groups are only supported for regular "
		    "symmetric single-hunt even-bell methods" );
	  return false;
	}
    }

  if ( ( formats_have_names() || formats_have_cc_ids() ) 
       && ! method_libraries::has_libraries() )
    {
      ap.error( "The -L option must be used if $n, $N or $i is used" );
      return false;
    }

  if ( formats_have_payloads() && !filter_mode ) 
    {
      ap.error( "Filter payloads can only be accessed when filtering" );
      return false;
    }

  if ( filter_lib_mode && filter_mode )
    {
      ap.error( "--filter and --filter-lib are mutually exclusive" );
      return false;
    }

  if ( filter_lib_mode && ! method_libraries::has_libraries() )
    {
      ap.error( "--filter-lib specified, but no libaries specified" );
      return false;
    }

  if ( invert_filter && !filter_lib_mode && !filter_mode )
    {
      ap.error( "--invert-filter can only be used when filtering" );
      return false;
    }

  if ( formats_have_old_lhcodes() )
    {
      if ( bells != 6 & bells != 5 || hunt_bells != 1)
	{
	  ap.error( "Old-style lead end codes are only supported for "
		    "single-hunt doubles or minor methods" );
	  return false;
	}
    }

  if ( start_row.bells() == 0 )
    start_row = row(bells);
  else if ( start_row.bells() != bells )
    {
      ap.error( "Starting row is on the wrong numebr of bells" );
      return false;
    }

  if ( avoid_rows.size() )
  {
    set<row> x;
    for ( set<row>::const_iterator i=avoid_rows.begin(), e=avoid_rows.end();
            i != e; ++i )
      x.insert( *i * row(bells) );
    avoid_rows = x;
  }

  if ( pends_generators.size() ) 
  {
    pends = group( pends_generators );
    set<row> x;
    for ( set<row>::const_iterator i=avoid_rows.begin(), e=avoid_rows.end();
            i != e; ++i )
      x.insert( pends.rcoset_label(*i) );
    avoid_rows = x;
  }
  if ( avoid_rows.find( pends.rcoset_label(start_row) ) != avoid_rows.end() ) {
     ap.error( "The start row conflicts with avoided row" );
     return false;
  }
 
  if (mask.empty()) mask = "*";
  else if (!lead_len) {
      ap.error( "Cannot use a mask for principles without also supplying a "
                "lead length" );
      return false;
  }

  if (!changes_str.empty()) {
    size_t i = 0, n = changes_str.size();
    if ( changes_str[i] == '!' ) { include_changes = false; ++i; }
    while ( i < n ) {
      size_t j = changes_str.find(',', i); 
      if ( j == string::npos ) j = n;
      try { 
        changes.insert( change(bells, changes_str.substr(i,j-i)) );
      } catch ( const exception& e ) {
        ap.error(make_string() << "Unable to parse --changes: "  << e.what());
        return false;
      }
      i = j+1;
    }

    try {
      restrict_changes(*this);
    } catch (exception const& e) {
      ap.error(make_string() << e.what()); return false;
    }
  }
      
  try
    {
      if ( ! parse_mask( *this, ap ) )
	return false;
    }
  catch ( const exception &e )
    {
      ap.error( make_string() << "Unable to process mask: " << e.what() );
      return false;
    }
  
  if ( lead_len && int(allowed_changes.size()) != lead_len )
    {
      ap.error( "The specified mask was an incorrect length" );
      return false;
    }

  try
    {
      startmeth = method( startmethstr, bells );
    }
  catch ( const exception &e )
    {
      ap.error( make_string() 
                << "Unable to parse place-notation passed to --start-at: "
                << e.what() );
      return false;
    }

  try
    {
      // TODO:  This should be folded into the -m method mask 
      prefix = method( prefixstr, bells );
    }
  catch ( const exception &e )
    {
      ap.error( make_string()
                << "Unable to parse place-notation passed to --prefix: "
                << e.what() );
      return false;
    }

  if ( allowed_falseness.size() )
    {
      if ( hunt_bells != 1 ) 
        {
          ap.error( "Can only specify falseness for single-hunt methods" );
          return false;
        }
      if ( treble_dodges != 1 )
        {
          ap.error( "Can only specify falseness for 'normal' treble-dodging "
                    "methods (i.e. G1)" );
          return false;
        }
      if ( ! require_pbles )
        {
          ap.error( "Can only specify falseness for regular methods" );
          return false;
        }
    }

  if ( !status_freq ) status_freq = 10000;
 
  return true;
}
