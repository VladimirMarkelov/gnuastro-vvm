# Add noise to a large image then subtract the sky.
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





# Preliminaries:
################
# Set the variabels (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
prog=subtractsky
execname=../src/$prog/ast$prog





# If the executable was not made (the user chose to not install this
# package), skip this test:
if [ ! -f $execname ]; then
    exit 77
fi




# This test relies on MakeNoise, so we have to check if it was built:
if [ ! -f ../src/mknoise/astmknoise ]; then
    exit 77
fi





# Actual test script:
#####################
base=convolve_spatial
export GSL_RNG_SEED=1
export GSL_RNG_TYPE=ranlxs2
../src/mknoise/astmknoise --envseed $base".fits"
$execname $base"_noised.fits" --checksky