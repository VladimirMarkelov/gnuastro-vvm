# Run the program to build a report the Gnuastro version in C++.
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
execname=./versioncpp





# Skip?
# =====
#
# If the actual executable wasn't built, then this is a hard error and must
# be FAIL.
if [ ! -f $execname ]; then
    echo "$execname library program not compiled.";
    exit 99;
fi;





# Actual test script
# ==================
$execname
