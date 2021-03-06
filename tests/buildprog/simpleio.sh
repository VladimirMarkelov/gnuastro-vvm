# Make labeled regions on an image with blank pixels.
#
# See the Tests subsection of the manual for a complete explanation
# (in the Installing gnuastro section).
#
# Original author:
#     Mohammad Akhlaghi <akhlaghi@gnu.org>
# Contributing author(s):
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.





# Preliminaries
# =============
#
# Set the variables (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
img=psf.fits
prog=buildprog
execname=../bin/$prog/ast$prog
source=$topsrc/tests/$prog/simpleio.c





# Skip?
# =====
#
# If the dependencies of the test don't exist, then skip it. There are two
# types of dependencies:
#
#   - The executable was not made (for example due to a configure option).
#
#   - The input data was not made (for example the test that created the
#     data file failed).
#
#   - Gnuastro ships with its own version of Libtool for the building of
#     the libraries and programs. But here, we want to test the user's
#     libtool (and how it works with BuildProgram). So if libtool wasn't
#     found at configure time, we need to skip this test.
if [ ! -f $execname ]; then echo "$execname not created.";  exit 77; fi
if [ ! -f $img      ]; then echo "$img does not exist.";    exit 77; fi
if [ ! -f $source   ]; then echo "$source does not exist."; exit 77; fi
if [ "x$hasgnulibtool" != "xyes" ];then
    echo "GNU libtool not present.";
    exit 77;
fi




# Actual test script
# ==================
#
# We want to use the `libgnuastro.la' corresponding to this install, not
# the one (that is possibly) installed (hence the use of `--la').
#
# Except for `gnuastro/config.h', all headers are installed in
# `$topsrc/lib' and `gnuastro/config.h' is in "../lib/"
$execname $source $img 1 --la=../lib/libgnuastro.la -I$topsrc/lib -I../lib/
