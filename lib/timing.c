/*********************************************************************
Functions to report timing in verbose mode.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2015, Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <stdio.h>
#include <stdint.h>

#include <gnuastro-internal/timing.h>



/* Micro-second based timer, which can be used to generate random numbers.
   The type of `tv_sec' and `tv_usec' is `long int' (from the GNU C Library
   manual), so this function will also return long. */
long
gal_timing_time_based_rng_seed()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  return tv.tv_sec + tv.tv_usec;
}





/* Used to report the time it takes for an action to be done. */
void
gal_timing_report(struct timeval *t1, char *jobname, size_t level)
{
  double dt=1e30;
  struct timeval t2;

  if(t1)
    {
      gettimeofday(&t2, NULL);

      dt= ( ((double)t2.tv_sec+(double)t2.tv_usec/1e6) -
            ((double)t1->tv_sec+(double)t1->tv_usec/1e6) );
    }

  if(level==0)
    printf("%s %-f seconds\n", jobname, dt);
  else if(level==1)
    {
      if(t1)
        printf("  - %-*s %f seconds\n",
               GAL_TIMING_VERB_MSG_LENGTH_V, jobname, dt);
      else printf("  - %s\n", jobname);
    }
  else if(level==2)
    {
      if(t1)
        printf("  ---- %-*s %f seconds\n",
               GAL_TIMING_VERB_MSG_LENGTH_V-3, jobname, dt);
      else printf("  ---- %s\n", jobname);
    }
}
