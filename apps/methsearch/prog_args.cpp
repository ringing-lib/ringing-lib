// -*- C++ -*- prog_args.cpp - handle program arguments
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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


#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/row.h>
#include <ringing/streamutils.h>
#include "args.h"
#include "prog_args.h"
#include "libraries.h"
#include "mask.h"
#include "format.h"
#include "music.h"

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
  else if ( arg == "U" )
    {
      args.true_trivial = args.true_half_lead 
	= args.true_lead = args.true_course = args.no_U_falseness = true;
    }
  else if ( arg == "e" || arg == "extent" )
    {
      args.true_trivial = args.true_half_lead 
	= args.true_lead = args.true_extent = true;
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
  : H_fmt( "",       format_string::stat_type   ),
    R_fmt( "%p\t%l", format_string::normal_type )
{
}

void arguments::bind( arg_parser &p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
	 ( 'b', "bells",  
	   "The number of bells.  This option is required", "BELLS",
	   bells.get() ) );

  p.add( new boolean_opt
	 ( 'c', "cyclic", 
	   "Require cyclic lead ends",
	   require_cyclic_les.get() ) );

  p.add( new boolean_opt
	 ( 'r', "regular",
	   "Require regular (P.B.) lead ends",
	   require_pbles.get() ) );

  p.add( new boolean_opt
	 ( 'A', "all-methods",    
	   "Include differentials or differential hunters",
	   show_all_meths.get() ) );

  p.add( new integer_opt
	 ( 'p', "blows-per-place", 
	   "At most NUM consecutive blows in one place (0 = unlimited)", "NUM",
	   max_consec_blows.get() ) );

  p.add( new integer_opt
	 ( 'l', "places-per-change", 
	   "At most NUM places in any change (0 = unlimited)", "NUM",
	   max_places_per_change.get() ) );

  p.add( new integer_opt
	 ( 'n', "changes-per-lead", 
	   "Require NUM changes per lead (0 = automatic)", "NUM",
	   lead_len.get() ) );

  p.add( new boolean_opt
	 ( 'q', "quiet",  
	   "Do not display methods when found",
	   quiet.get() ) );

  p.add( new string_opt
	 ( 'H', "frequencies", 
	   "Count frequencies of different method properties", "FMT", 
	   H_fmt.fmt ) );

  p.add( new string_opt
	 ( 'R', "format", 
	   "Use FMT to format methods as found", "FMT",
	   R_fmt.fmt ) );

  p.add( new boolean_opt
	 ( 'h', "any-hl-le",
	   "Allow any change at the half-lead and lead-end",
	   require_single_place_lh_le.get(), false ) );

  p.add( new boolean_opt
	 ( 'e', "restricted-le",
	   "Only allow 12 and 1N (or 1 and 12N lead ends)",
	   require_limited_le.get() ) );

  p.add( new boolean_opt
	 ( 'w', "right-place", 
	   "Require right place methods",
	   right_place.get() ) );

  p.add( new boolean_opt
	 ( 'f', "no-78s", 
	   "Prohibit 78s (on 8 bells) in the place notation",
	   no_78_pns.get() ) );

  p.add( new integer_opt
	 ( 'j', "max-adj-places",
	   "Forbid methods with more than NUM adjacent places", "NUM",
	   max_consec_places.get(), /* default = */ 1 ) );

  p.add( new boolean_opt
	 ( 's', "symmetric", 
	   "Look for (normally) symmetric methods (default)",
	   sym.get() ) );

  p.add( new boolean_opt
	 ( 'k', "rotational", 
	   "Look for rotationally symmteric methods 'Brave New Worlds' "
	   "symmetry",
	   skewsym.get() ) );

  p.add( new boolean_opt
	 ( 'd', "double", 
	   "Look for double methods - 'Double Eastern' symmetry",
	   doubsym.get() ) );

  p.add( new boolean_opt
	 ( 'S', "surprise", 
	   "Require an internal place between dodging positions",
	   surprise.get() ) );

  p.add( new boolean_opt
	 ( 'T', "treble-bob", 
	   "Forbid an internal place between dodging positions",
	   treble_bob.get() ) );

  p.add( new integer_opt
	 ( 'G', "treble-dodges", 
	   "Look for methods where the treble dodges NUM times in each "
	   "position", "NUM",
	   treble_dodges.get() ) );

  p.add( new integer_opt
	 ( 'U', "hunts", 
	   "Look for methods with NUM hunt bells", "NUM",
	   hunt_bells.get() ) );

  p.add( new boolean_opt
	 ( 'u', "status",
	   "Display the current status",
	   status.get() ) );

  p.add( new integer_opt
	 ( '\0', "limit",
	   "Limit the search to the first NUM methods", "NUM",
	   search_limit.get() ) );

  p.add( new boolean_opt
	 ( '\0', "count",
	   "Count the number of methods found",
	   count.get() ) );

  p.add( new boolean_opt
	 ( 'P', "parity-hack",
	   "Require an equal number of rows of each parity for each place "
	   "in the treble's path",
	   same_place_parity.get() ) );

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
	 ( '\0', "bob",
	   "Use CHANGE as the bob", "CHANGE",
	   bob ) );

  p.add( new string_opt
	 ( '\0', "start-at",
	   "Start the search with the method PN", "PN",
	   startmeth ) );

  p.add( new string_opt
	 ( '\0', "le-change",
	   "Require CHANGE as the lead-end change", "CHANGE",
	   required_le_change ) );

  p.add( new string_opt
	 ( '\0', "hl-change",
	   "Require CHANGE as the half-lead change", "CHANGE",
	   required_hl_change ) );

  p.add( new boolean_opt
	 ( '\0', "cyclic-hle", 
	   "Require cyclic half-lead ends",
	   require_cyclic_hle.get() ) );

  p.add( new boolean_opt
	 ( '\0', "cyclic-hlh", 
	   "Require cyclic half-lead heads",
	   require_cyclic_hlh.get() ) );

  p.add( new boolean_opt
	 ( '\0', "rev-cyclic-hle", 
	   "Require reverse cyclic half-lead ends",
	   require_rev_cyclic_hle.get() ) );

  p.add( new boolean_opt
	 ( '\0', "rev-cyclic-hlh", 
	   "Require reverse cyclic half-lead heads",
	   require_rev_cyclic_hlh.get() ) );

  p.add( new boolean_opt
	 ( '\0', "offset-cyclic", 
	   "Require offset cyclicity",
	   require_offset_cyclic.get() ) );
}

bool arguments::validate( arg_parser &ap )
{
  if ( bells == 0 ) 
    {
      ap.error( "Must specify the number of bells" );
      return false;
    }

  if ( bells < 3 || bells >= bell::MAX_BELLS ) 
    {
      ap.error( make_string() << "The number of bells must be between 3 and " 
		<< bell::MAX_BELLS-1 << " (inclusive)" );
      return false;
    }

  if ( ! hunt_bells && ! lead_len )
    {
      ap.error( "Must specify the lead length when searching for principles" );
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

  if ( ! hunt_bells && bells % 2 && right_place )
    {
      ap.error( "Odd bell methods need at least one hunt bell to be "
		"right place" );
      return false;
    }

  if ( require_cyclic_hlh && require_cyclic_hle )
    {
      ap.error( "You cannot require cyclic half-lead ends and "
		"half-lead heads" );
      return false;
    }

  if ( (require_cyclic_hlh || require_cyclic_hle) 
       && hunt_bells != 1 )
    {
      ap.error( "Cyclic half-leads are only implemented for "
		"single hunt methods" );
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

  if ( require_offset_cyclic && !treble_dodges )
    {
      ap.error( "The `offset-cyclic' option requires a treble-dodging"
		" path (e.g. -G1)" );
      return false;
    }

  try
    {
      R_fmt = format_string( R_fmt.fmt, format_string::normal_type );
    }
  catch ( const argument_error &error )
    {
      ap.error( make_string() << "Error parsing -R format: " << error.what() );
      return false;
    }

  try
    {
      H_fmt = format_string( H_fmt.fmt, format_string::stat_type );
      histogram = !H_fmt.fmt.empty();
    }
  catch ( const argument_error &error )
    {
      ap.error( make_string() << "Error parsing -H format: " << error.what() );
      return false;
    }

  {
    for ( vector<size_t>::const_iterator i( R_fmt.has_rows.begin() ), 
	    e( R_fmt.has_rows.end() ); i != e; ++i )
      {
	if ( *i > bells * (1 + treble_dodges) * 2 )
	  {
	    ap.error( "Format specifies row after end of method" );
	    return false;
	  }
      }
  }

  {
    for ( vector<size_t>::const_iterator i( H_fmt.has_rows.begin() ), 
	    e( H_fmt.has_rows.end() ); i != e; ++i )
      {
	if ( *i > bells * (1 + treble_dodges) * 2 )
	  {
	    ap.error( "Format specifies row after end of method" );
	    return false;
	  }
      }
  }

  if ( no_U_falseness )
    {
      if ( bob.empty() )
	{
	  ap.error( "Must specify a bob to use -FU" );
	  return false;
	}
    }

  if ( !bob.empty() )
    {
      try 
	{
	  // Check it can be created
	  change( bells, bob );
	} 
      catch ( const invalid_argument &e ) 
	{
	  ap.error( make_string() << "Unable to parse bob: " 
		    << e.what() );
	  return false;
	}
    }

  if ( skewsym + sym + doubsym >= 2 )
    skewsym = sym = doubsym = true;

  if ( R_fmt.has_falseness_group || H_fmt.has_falseness_group )
    {
      if ( bells != 8 )
	{
	  ap.error( "Falseness groups are only supported on 8 bells" );
	  return false;
	}

      if ( require_pbles && sym )
	;
      
      else if ( require_cyclic_les && skewsym )
	;

      else
	{
	  ap.error( "Falseness groups are only supported in "
		    "-kc or -sr searches" );
	  return false;
	}
    }

  if ( !required_hl_change.empty() )
    {
      if ( required_changes.empty() )
	required_changes.resize( 2*bells * (1+treble_dodges) );

      try 
	{
	  required_changes[ bells * (1 + treble_dodges) - 1 ]
	    = change( bells, required_hl_change );
	} 
      catch ( const invalid_argument &e ) 
	{
	  ap.error( make_string() 
		    << "Unable to parse required half-lead change: "
		    << e.what() );
	  return false;
	}
    }

  if ( !required_le_change.empty() )
    {
      if ( required_changes.empty() )
	required_changes.resize( 2*bells * (1+treble_dodges) );

      try 
	{
	  required_changes[ 2*bells * (1 + treble_dodges) - 1 ]
	    = change( bells, required_le_change );
	} 
      catch ( const invalid_argument &e ) 
	{
	  ap.error( make_string() 
		    << "Unable to parse required lead-end change: "
		    << e.what() );
	  return false;
	}
    }

  if ( R_fmt.has_name || R_fmt.has_full_name )
    {
      if ( ! method_libraries::has_libraries() )
	{
	  ap.error( "The -L option must be used if either %n or %N is used" );
	  return false;
	}
    }

  if ( !mask.empty() )
    {
      if ( !required_changes.empty() )
	{
	  ap.error( "Cannot specify a mask and a required half-lead"
		    " or lead-end" );
	  return false;
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

      if ( required_changes.size() != lead_len )
	{
	  ap.error( "The specified mask was an incorrect length" );
	  return false;
	}
    }


  if ( required_changes.size() )
    {
      if ( ! is_mask_consistent( *this, ap ) )
	{
	  ap.error( "Some of the required changes specified are inconsistent "
		    "with the specified symmetries" );
	  return false;
	}
    }

  return true;
}
