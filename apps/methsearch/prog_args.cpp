// -*- C++ -*- prog_args.cpp - handle program arguments
// Copyright (C) 2002, 2003, 2004, 2005 Richard Smith <richard@ex-parrot.com>

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
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#else
#include <cassert>
#endif

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
      args.true_course = args.true_extent = false;
    }
  else if ( arg == "h" || arg == "half-lead" )
    {
      args.true_trivial = args.true_half_lead = true;
      args.true_lead = args.true_course = args.true_extent = false;
    }
  else if ( arg == "n" || arg == "none" ) 
    {
      args.true_trivial = true;
      args.true_half_lead = args.true_lead 
	= args.true_course = args.true_extent = false;
    }
  else if ( arg == "x" || arg == "really-none" )
    {
      args.true_trivial = args.true_half_lead
	= args.true_lead = args.true_course = args.true_extent  = false;
    }
  else
    {
      cerr << "Unknown -F option: \"" << arg << "\"\n";
      return false;
    }

  return true;
}


arguments::arguments()
  : mask("*"),
    require_expr_idx( static_cast<size_t>(-1) )
{
}

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

  p.add( new string_opt
	 ( '\0', "require", 
	   "Require EXPR to be true", "EXPR",
	   require_str ) );

  p.add( new boolean_opt
	 ( 'e', "restricted-le",
	   "Only allow 12 and 1N (or 1 and 12N lead ends)",
	   require_limited_le ) );

  p.add( new boolean_opt
	 ( 'E', "prefer-restricted-le",
	   "Prefer 12 and 1N (or 1 and 12N lead ends)",
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
	 ( 's', "symmetric", 
	   "Look for (normally) symmetric methods (default)",
	   sym ) );

  p.add( new boolean_opt
	 ( 'k', "rotational", 
	   "Look for rotationally symmteric methods 'Brave New Worlds' "
	   "symmetry",
	   skewsym ) );

  p.add( new boolean_opt
	 ( 'd', "double", 
	   "Look for double methods - 'Double Eastern' symmetry",
	   doubsym ) );

  p.add( new boolean_opt
	 ( 'S', "surprise", 
	   "Require an internal place between dodging positions",
	   surprise ) );

  p.add( new boolean_opt
	 ( 'T', "treble-bob", 
	   "Forbid an internal place between dodging positions",
	   treble_bob ) );

  p.add( new integer_opt
	 ( 'G', "treble-dodges", 
	   "Look for methods where the treble dodges NUM times in each "
	   "position", "NUM",
	   treble_dodges ) );

  p.add( new integer_opt
	 ( 'U', "hunts", 
	   "Look for methods with NUM hunt bells", "NUM",
	   hunt_bells ) );

  p.add( new boolean_opt
	 ( 'u', "status",
	   "Display the current status",
	   status ) );

  p.add( new integer_opt
	 ( '\0', "limit",
	   "Limit the search to the first NUM methods", "NUM",
	   search_limit ) );

  p.add( new boolean_opt
	 ( '\0', "count",
	   "Count the number of methods found",
	   count ) );

  p.add( new boolean_opt
	 ( '\0', "raw-count",
	   "Count the number of methods found and print it without "
	   "surrounding text",
	   raw_count ) );

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

  p.add( new delegate_opt
	 ( 'M', "music",
	   "Score the music in a plain course", "PATTERN",
	   &musical_analysis::add_pattern ) );

  p.add( new string_opt
	 ( '\0', "start-at",
	   "Start the search with the method PN", "PN",
	   startmeth ) );

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

  if ( bells < 3 || bells >= int(bell::MAX_BELLS) ) 
    {
      ap.error( make_string() << "The number of bells must be between 3 and " 
		<< bell::MAX_BELLS-1 << " (inclusive)" );
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

  if ( !hunt_bells && !lead_len )
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


  if ( ! lead_len )
    lead_len = (1 + treble_dodges) * bells * 2;

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

  if ( (surprise || treble_bob) && ! hunt_bells )
    {
      ap.error( "Surprise and/or treble bob methods require at least one "
		"hunt bell"  );
      return false;
    }

  if ( treble_dodges && bells % 2 == 1 )
    {
      ap.error( "Treble dodging methods can only be be found on an"
		" even number of bells" );
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

  if ( outfmt.size() && R_fmt_str.size() )
    {
      ap.error( "The -O option cannot be used with the -R option" );
      return false;
    }

  if (!quiet) {
    if ( outfile == "-" ) outfile.erase();

    try {
      if ( outfmt.empty() || outfmt == "fmt" ) {
	outfmt.erase();
	if ( R_fmt_str.empty() ) R_fmt_str = "$p\t$l";
	outputs.add( new fmtout( R_fmt_str, outfile ) );
      } 
      else if ( outfmt == "xml" ) 
	outputs.add( new xmlout( outfile ) );

      else {
	ap.error( "Unknown -O format: must be either `xml' or `fmt'" );
	return false;
      }
    }
    catch ( const argument_error &error ) {
      ap.error( make_string() << "Error parsing -R format: " << error.what() );
      return false;
    }
  }

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


  if ( require_str.size() ) 
    {
      try
	{
	  require_expr_idx = format_string::parse_requirement( require_str );
	}
      catch ( const argument_error &error )
	{
	  ap.error( make_string() << "Error parsing --require argument: "
		    << error.what() );
	  return false;
	}
    }

  if ( skewsym + sym + doubsym >= 2 )
    skewsym = sym = doubsym = true;

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

  if ( formats_have_names() && ! method_libraries::has_libraries() )
    {
      ap.error( "The -L option must be used if either $n or $N is used" );
      return false;
    }

  if ( formats_have_old_lhcodes() )
    {
      if ( bells != 6 || hunt_bells != 1 || show_all_meths )
	{
	  ap.error( "Old-style lead end codes are only supported for "
		    "non-differential single-hunt minor methods" );
	  return false;
	}
      else if ( !require_limited_le && !require_pbles ) 
	{
	  ap.error( "Old-style lead end codes only apply to 12 or 16 lead "
		    "heads: -e or -r must be used" );
	  return false;
	}
    }

  assert( !mask.empty() );
      
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
  
  if ( int(allowed_changes.size()) != lead_len )
    {
      ap.error( "The specified mask was an incorrect length" );
      return false;
    }

  return true;
}
