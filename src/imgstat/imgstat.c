/*********************************************************************
ImageStatistics - Get general statistics about the image.
ImgeStatistics is part of GNU Astronomy Utilities (Gnuastro) package.

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
along with gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <math.h>
#include <errno.h>
#include <error.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mode.h"
#include "timing.h"
#include "statistics.h"

#include "main.h"
#include "imgstat.h"


/* This function will report the simple immediate statistics of the
   data. For the average and standard deviation, the unsorted data is
   used so we don't suddenly encounter rounding errors. */
void
reportsimplestats(struct imgstatparams *p)
{
  double sum;
  size_t modeindex;
  float ave, std, med, mode;

  sum=floatsum(p->img, p->size);
  favestd(p->img, p->size, &ave, &std, NULL);
  med=p->sorted[indexfromquantile(p->size, 0.5f)];

  printf(SNAMEVAL FNAMEVAL FNAMEVAL, "Number of points", p->size,
	 "Minimum", p->sorted[0], "Maximum", p->sorted[p->size-1]);
  printf(FNAMEVAL FNAMEVAL FNAMEVAL FNAMEVAL, "Sum", sum, "Mean", ave,
	 "Standard deviation", std, "Median", med);
  if(modeindexinsorted(p->sorted, p->size, &modeindex, 1.5f, -1))
    {
      mode=p->sorted[modeindex];
      printf(FNAMEVAL, "Mode", mode);
    }
  else
    {
      printf(STRVAL, "Mode", "Not accurate");
    }
}





void
printasciihist(struct imgstatparams *p)
{
  size_t j;
  float *bins;
  float quant=-1.0f; /* histmin and histmax were already set before. */
  int i, binonzero=0, normhist=0, maxhistone=1;

  /* Find the histogram for the ASCII plot: */
  setbins(p->sorted, p->size, ASCIIHISTNUMBINS, p->histmin,
          p->histmax, binonzero, quant, &bins);
  histogram(p->sorted, p->size, bins, ASCIIHISTNUMBINS,
            normhist, maxhistone);

  /* It's maximum value is set to one. Multiply that by the desired
     height in pixels. */
  for(j=0;j<ASCIIHISTNUMBINS;++j)
    bins[j*2+1]*=ASCIIHISTHEIGHT;

  /* Plot the ASCII histogram: */
  printf("   -- ASCII histogram in the range: %f - %f:\n",
	 p->histmin, p->histmax);
  for(i=ASCIIHISTHEIGHT;i>=0;--i)
    {
      printf("    |");
      for(j=0;j<ASCIIHISTNUMBINS;++j)
	{
	  if(bins[j*2+1]>=((float)i-0.5f)
	     && bins[j*2+1]>0.0f) printf("*");
	  else printf(" ");
	}
      printf("\n");
    }
  printf("    |");
  for(j=0;j<ASCIIHISTNUMBINS;++j) printf("-");
  printf("\n\n");

  /* Clean up.*/
  free(bins);
}




void
printhistcfp(struct imgstatparams *p, float *bins, size_t numbins,
             char *filename, char *outputtype)
{
  float d;
  size_t i;
  FILE *out;
  time_t rawtime;
  int int0float1=1;

  /* Open the file: */
  errno=0;
  out=fopen(filename, "w");
  if(out==NULL)
    error(EXIT_FAILURE, errno, "Couldn't open file %s", filename);

  /* Get the time to print on the report. */
  time(&rawtime);
  fprintf(out, "# %s \n# %s, created on %s", SPACK_STRING, outputtype,
	  ctime(&rawtime));
  fprintf(out, "# Input (hdu): %s (%s)\n", p->up.inputname, p->cp.hdu);
  if(p->up.masknameset)
    fprintf(out, "# Mask (hdu): %s (%s)\n", p->up.maskname, p->up.mhdu);

  if(p->lowerbin)
    fprintf(out, "# Column 1: Flux of lower value of each bin\n");
  else
    fprintf(out, "# Column 1: Flux in the middle of each bin\n");

  if(strcmp(outputtype, CFPSTRING))
    {
      fprintf(out, "# Column 2: Average of the sorted index of all points "
              "in this bin");
      if(p->normcfp)
          fprintf(out, " (normalized).\n");
      else if (p->maxcfpeqmaxhist)
        fprintf(out, " (Scaled to the histogram).\n");
      else
        {
          fprintf(out, ".\n");
          int0float1=0;
        }
    }
  else if (strcmp(outputtype, HISTSTRING))
    {
      if(p->normhist)
        fprintf(out, "# Column 2: Fraction of points in this bin. \n");
      else if(p->maxhistone)
        fprintf(out, "# Column 2: Histogram if the maximum bin is "
                "set to 1.\n");
      else
        {
          fprintf(out, "# Column 2: Number of points in this bin. \n");
          int0float1=0;
        }
    }

  /* Put the data in the file: */
  if(p->lowerbin==0) d=(bins[2]-bins[0])/2;
  else               d=0.0f;
  if(int0float1)
    for(i=0;i<numbins;++i)
      fprintf(out, "%-20.6f"PRINTFLT, bins[i*2]+d, bins[i*2+1]);
  else
    for(i=0;i<numbins;++i)
      fprintf(out, "%-20.6f"PRINTINT, bins[i*2]+d, bins[i*2+1]);
  fclose(out);
}





void
imgstat(struct imgstatparams *p)
{
  size_t i;
  float quant=-1.0f;                   /* The quantile was already   */
  float maxhist=-FLT_MAX, *bins=NULL;  /* taken into affect in ui.c. */


  /* Report the simple statistics: */
  if(p->cp.verb)
    {
      reportsimplestats(p);
      if(p->asciihist) printasciihist(p);
    }

  /* Make the histogram: */
  if(p->histname)
    {
      /* Make the actual data histogram and save it. */
      setbins(p->sorted, p->size, p->histnumbins, p->histmin,
              p->histmax, p->binonzero, quant, &bins);
      histogram(p->sorted, p->size, bins, p->histnumbins,
                p->normhist, p->maxhistone);
      printhistcfp(p, bins, p->histnumbins, p->histname, HISTSTRING);

      /* Get the hisogram maximum value if it is needed for the
         cumulative frequency plot. */
      if(p->maxcfpeqmaxhist)
	for(i=0;i<p->histnumbins;++i)
	  if(bins[i*2+1]>maxhist)
	    maxhist=bins[i*2+1];
    }

  /* Make the cumulative distribution function: */
  if(p->cfpname)
    {
      if(p->cfpsimhist)
	{
	  p->cfpnum=p->histnumbins;
	  for(i=0;i<p->cfpnum;++i)
	    bins[i*2+1]=0.0f;
	}
      else
	{
	  if(p->histname) free(bins);
          setbins(p->sorted, p->size, p->cfpnum, p->cfpmin,
                  p->cfpmax, p->binonzero, quant, &bins);
	}
      cumulativefp(p->sorted, p->size, bins, p->cfpnum, p->normcfp);

      if(p->maxcfpeqmaxhist)
	for(i=0;i<p->cfpnum;++i)
	  bins[i*2+1]*=(maxhist/p->size);

      printhistcfp(p, bins, p->cfpnum, p->cfpname, CFPSTRING);
    }

  /* Print out the Sigma clippings:
  if(p->verb && p->sigmaclip)
    {
      printf(" - %.2f sigma-clipping by convergence (med, mean, "
	     "std, number):\n", p->sigmultip);
      sigmaclip_converge(p);
      printf(" - %.2f sigma-clipping %lu times (med, mean, std, number):\n",
	     p->sigmultip, p->timesclip);
      sigmaclip_certainnum(p);
    }
  */
  /* Free the allocated arrays: */
  if(p->histname || p->cfpname) free(bins);
}