The Ringing Class Library
=========================

This is the Ringing Class Library, and this file contains some notes
about installing and using the library.

Introduction
------------

  The Ringing Class Library is a collection of C++ classes which represent
the objects studied in English change ringing.  These include rows, changes,
methods and method libraries.  The project's home page can be found at
  http://ringing-lib.sourceforge.net/
where there is also information about how you can contribute, report bugs,
and get at discussion forums.


Dependencies
------------

  The Ringing Class Library can optionally be built to use the Xerces XML
library to support XML method libraries.  This can be controlled by the
`--with-xerces` (or `--without-xerces`) options to `./configure`.  The gsiril 
program that is supplied with the library can be compiled with GNU readline
to support better command line editing.  The `--with-readline` option controls
this.  When no explicit option is given to `./configure`, it attempts to 
locate the libraries and will use them if they can be found.

  To use Xerces from Microsoft Visual Studio, edit the ringing/common-msvc.h
header to define `RINGING_USE_XERCES` to 1, and manually add xerces to the 
list of libraries and headers required.  (This is done via Settings on the 
Project menu.  In the C/C++ tab, choose "Preprocessor" in the Category 
selector and add the include directory to "Additional include directories".
Then in the Link tab, add Xerces's `.lib` file to "Object/library modules".  

  Use of readline from Microsoft Visual Studio is untested.

  If you are compiling on Debian Linux, we recommend having the `autoconf', 
`libgdome2-dev' and `libreadline-dev' packages installed.

Installation
------------

  If you have a Unix-like system, you should be able to type `./configure` to
configure the package.  If you do not yet have a `configure` script, you'll 
need to run `autoreconf -i`, which will run `aclocal`, `libtoolize`, 
`automake` and `autoconf` to create one.  The `configure` script will look at
whether your C++ compiler supports various language features such as
namespaces and exceptions.  The results of these tests are put into the file
`ringing/common-am.h` as macro definitions.  It is possible that you will want
to edit this file by hand to change the default behaviour of the class
library.  You can then type `make` to compile the package, and `make install`
to install it.  

  If you are using Windows with a compiler other than Microsoft Visual Studio,
or any other system on which `./configure` does not work, you should copy the 
file `ringing/common-am.h.in` to `ringing/common-am.h` and edit it to indicate
how your C++ compiler behaves and where your standard header files are to be 
found.  The comments in that file explain which parts need to be customised. 
After customisation, you should be able to compile all the files in the 
`ringing/` directory into a library.  You should then install all the header 
files in that directory to somewhere where your C++ compiler will find them 
(as `#include <ringing/row.h>` etc.)


Using the library
-----------------

  The documentation describing how to use the library is in the `doc/` sub-
directory.  If you don't have the software necessary to build the Texinfo
documentation, you can download it separately in various formats from the
project web page (see above).


Supported compilers
-------------------

  It is hoped that the class library will compile on any modern C++ compiler.
However, at present the code is generally only tested using the latest
version of GCC.

