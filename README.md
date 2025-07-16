The Ringing Class Library
=========================

This is the Ringing Class Library, and this file contains some notes
about installing and using the library.

Introduction
------------

  The Ringing Class Library is a collection of C++ classes which represent
the objects studied in English change ringing.  These include rows, changes,
methods and method libraries.  The project's home page can be found at
  https://github.com/ringing-lib/ringing-lib
where there is also information about how you can contribute, report bugs,
and get at discussion forums.

Installation
------------

  You will need GNU make installed on your system.  Then type `make` in
this directory to compile the package, and `make install` to install it.
By default this will install the library in `/usr/local/lib` and the
associated programs in `/usr/local/bin`.  Installing in this location
will require root access, which on some systems may require running
`sudo make install`.  To change the prefix, for example to install it in
your home directory, run `make install PREFIX=$HOME`.  The `PREFIX`
variable can also be set in a `local.mk` file, as described below.

Dependencies
------------

  The Ringing Class Library can built with no dependencies, aside from a
working C++ compiler and standard library, GNU make, and a POSIX
compliant shell.  However, certain optional functionality requires
additional libraries.  To enable this functionality, you will need set
some make variables.  This is most easily done by creating a file called
`local.mk` in the same directory as this file to contain these
variables, but they can also be put in the environment on added to the
make command line. 

  In order to read XML method libraries, you need either the Xerces XML
library or libgdome2.  This is controlled by settings the `HAVE_XERCES`
or `HAVE_GDOME` variable to `1`.  If the your system requires additional
-I, -L or -l options to use these libraries, these can be specified in
the `GDOME_INCLUDE`, `GDOME_LDFLAGS` or `GDOME_LIBS` variables, or
equivalently for Xerces.  For instance, putting the following in a
`local.mk` file will build using libgdome for XML support on Debian 11
('bullseye'):

```
HAVE_GDOME = 1
GDOME_INCLUDES += -I/usr/include/glib-2.0
GDOME_INCLUDES += -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
```

  The gsiril program that is supplied with the library can be compiled
with GNU readline to support better command line editing.  Setting the
`HAVE_READLINE` variable to `1` does this.

  Several programs can make use of the termcap library to give coloured
output.  Enable this with `HAVE_TERMAP`.

  If you are compiling on Debian Linux, we recommend having the 
`libgdome2-dev', `libxml2-dev`, `libglib2.0-dev`, `libreadline-dev` and
`libncurses-dev` packages installed.

Using the library
-----------------

  The documentation describing how to use the library is in the `doc/` sub-
directory.  If you don't have the software necessary to build the Texinfo
documentation, you can download it separately in various formats from the
project web page (see above).

Supported compilers
-------------------

  It is hoped that the class library will compile on any modern C++ compiler.
However, at present the code is generally only tested using recent 
versions of GCC.

