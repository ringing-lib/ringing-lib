# escape.awk - Escape a file into a long C string
# Copyright (C) 2011, 2019 Richard Smith <richard@ex-parrot.com>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


# The main reason for putting this in a separate file is to avoid
# having to escape it from the shell and from make.

! /^[ \t]*\/\// && ! /^[ \t]*$/ {
 
	gsub( "\\\\", "\\\\" ); 
	gsub( "\"", "\\\"" ); 
	print "\"" $0 "\\n\"";
}
