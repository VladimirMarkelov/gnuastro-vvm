/*********************************************************************
NoiseChisel - Detect and segment signal in noise.
NoiseChisel is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef ARGS_H
#define ARGS_H

#include <argp.h>

#include "commonargs.h"
#include "fixedstringmacros.h"










/**************************************************************/
/**************        argp.h definitions       ***************/
/**************************************************************/




/* Definition parameters for the argp: */
const char *argp_program_version=SPACK_STRING"\n"COPYRIGHT
  "\n\nWritten by Mohammad Akhlaghi";
const char *argp_program_bug_address=PACKAGE_BUGREPORT;
static char args_doc[] = "ASTRdata";





const char doc[] =
  /* Before the list of options: */
  TOPHELPINFO
  SPACK_NAME" Detects and segments signal that is deeply burried in noise. "
  "It employs a noise-based detection and segmentation method enabling it "
  "to be very resilient to the rich diversity of shapes in astronomical "
  "targets.\n"
  MOREHELPINFO
  /* After the list of options: */
  "\v"
  PACKAGE_NAME" home page: "PACKAGE_URL;





/* Available letters for short options:

   f j w x y z
   A C J W X Y Z

   Number keys free: >=517

   Options with keys (second structure element) larger than 500 do not
   have a short version.
 */
static struct argp_option options[] =
  {
    {
      0, 0, 0, 0,
      "Input:",
      1
    },
    {
      "mask",
      'M',
      "STR",
      0,
      "Mask image file name.",
      1
    },
    {
      "mhdu",
      'H',
      "STR",
      0,
      "Mask image header name.",
      1
    },
    {
      "kernel",
      'k',
      "STR",
      0,
      "Kernel image file name.",
      1
    },
    {
      "khdu",
      'U',
      "STR",
      0,
      "Kernel image header name.",
      1
    },
    {
      "skysubtracted",
      'E',
      0,
      0,
      "Input is already sky subtracted.",
      1
    },
    {
      "minbfrac",
      'B',
      "FLT",
      0,
      "Minimum fraction of undetected area in a mesh.",
      1
    },
    {
      "minnumfalse",
      'F',
      "INT",
      0,
      "Min No. of false detection/segments for quantile.",
      1
    },



    {
      0, 0, 0, 0,
      "Output:",
      2
    },



    {
      0, 0, 0, 0,
      "Mesh grid:",
      3
    },
    {
      "smeshsize",
      's',
      "INT",
      0,
      "Size of each small mesh (tile) in the grid.",
      3
    },
    {
      "lmeshsize",
      'l',
      "INT",
      0,
      "Size of each large mesh (tile) in the grid.",
      3
    },
    {
      "nch1",
      'a',
      "INT",
      0,
      "Number of channels along first FITS axis.",
      3
    },
    {
      "nch2",
      'b',
      "INT",
      0,
      "Number of channels along second FITS axis.",
      3
    },
    {
      "lastmeshfrac",
      'L',
      "INT",
      0,
      "Fraction of last mesh area to add new.",
      3
    },
    {
      "mirrordist",
      'd',
      "FLT",
      0,
      "Distance beyond mirror point. Multiple of std.",
      3
    },
    {
      "minmodeq",
      'Q',
      "FLT",
      0,
      "Minimum acceptable quantile for the mode.",
      3
    },
    {
      "interponlyblank",
      511,
      0,
      0,
      "Only interpolate over the blank pixels.",
      3
    },
    {
      "numnearest",
      'n',
      "INT",
      0,
      "Number of nearest neighbors to interpolate.",
      3
    },
    {
      "smoothwidth",
      'T',
      "INT",
      0,
      "Width of smoothing kernel (odd number).",
      3
    },
    {
      "checkmesh",
      500,
      0,
      0,
      "Store mesh IDs in `_mesh.fits' file.",
      3
    },
    {
      "fullinterpolation",
      501,
      0,
      0,
      "Ignore channels in interpolation.",
      3
    },
    {
      "fullsmooth",
      502,
      0,
      0,
      "Ignore channels in smoothing.",
      3
    },
    {
      "fullconvolution",
      504,
      0,
      0,
      "Ignore channels in convolution.",
      3
    },
    {
      "meshbasedcheck",
      516,
      0,
      0,
      "Each mesh in one pixel in mesh check images.",
      3
    },



    {
      0, 0, 0, 0,
      "Detection:",
      4
    },
    {
      "qthresh",
      't',
      "FLT",
      0,
      "Quantile threshold on convolved image.",
      4
    },
    {
      "erode",
      'e',
      "INT",
      0,
      "Num. erosions to apply after thresholding.",
      4
    },
    {
      "erodengb",
      506,
      "4or8",
      0,
      "Use 4 or 8 connectivity in erosion.",
      4
    },
    {
      "opening",
      'p',
      "INT",
      0,
      "Depth of opening to apply after erosion.",
      4
    },
    {
      "openingngb",
      507,
      "4or8",
      0,
      "Use 4 or 8 connectivity in opening.",
      4
    },
    {
      "sigclipmultip",
      'u',
      "FLT",
      0,
      "Multiple of standard deviation in sigma-clipping.",
      4
    },
    {
      "sigcliptolerance",
      'r',
      "FLT",
      0,
      "Difference in STD tolerance to halt iteration.",
      4
    },
    {
      "dthresh",
      'R',
      "FLT",
      0,
      "Threshold (STD multiple) for false detections.",
      4
    },
    {
      "detsnminarea",
      'i',
      "INT",
      0,
      "Minimum area to calculate S/N in detection.",
      4
    },
    {
      "detsnhistnbins",
      510,
      "INT",
      0,
      "Detection S/N hist. N. bins `_XX_detsn.txt'.",
      4
    },
    {
      "detquant",
      'c',
      "FLT",
      0,
      "False detections S/N quantile to find the true.",
      4
    },
    {
      "dilate",
      'I',
      "INT",
      0,
      "Number of times to dilate true detections.",
      4
    },

    {
      "checkthreshold",
      505,
      0,
      0,
      "Threshold value on each mesh `_thresh.fits'.",
      4
    },
    {
      "checkdetection",
      508,
      0,
      0,
      "False detection steps in file `_det.fits'.",
      4
    },
    {
      "checkdetectionsky",
      509,
      0,
      0,
      "Sky for false detections in file `_detsky.fits'.",
      4
    },
    {
      "checkdetectionsn",
      503,
      0,
      0,
      "Mesh detection S/N limit `_detsn.fits'.",
      4
    },
    {
      "checksky",
      512,
      0,
      0,
      "Final sky and its STD per pixel `_sky.fits'.",
      4
    },


    {
      0, 0, 0, 0,
      "Segmentation:",
      5
    },
    {
      "segsnminarea",
      'm',
      "INT",
      0,
      "Minimum area to find clumps S/N.",
      5
    },
    {
      "segquant",
      'g',
      0,
      0,
      "Signal to noise ratio quantile for clumps.",
      5
    },
    {
      "segsnhistnbins",
      514,
      "INT",
      0,
      "Segmentation S/N hist. N. bins `_XX_segsn.txt'.",
      5
    },
    {
      "keepmaxnearriver",
      'v',
      0,
      0,
      "Keep clumps with max touching river.",
      5
    },
    {
      "gthresh",
      'G',
      0,
      0,
      "Threshold (STD multiple) to stop growing clumps.",
      5
    },
    {
      "objbordersn",
      'O',
      "FLT",
      0,
      "Minimum S/N for grown clumps to be one object.",
      5
    },

    {
      "checksegmentation",
      513,
      0,
      0,
      "Store segmentation steps in file `_seg.fits'.",
      5
    },
    {
      "checkclumpsn",
      515,
      0,
      0,
      "Mesh clump S/N limit `_clumpsn.fits'.",
      4
    },


    {
      0, 0, 0, 0,
      "Operating modes:",
      -1
    },



    {0}
  };





/* Parse a single option: */
static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
  /* Save the arguments structure: */
  struct noisechiselparams *p = state->input;

  /* Set the pointer to the common parameters for all programs
     here: */
  state->child_inputs[0]=&p->cp;

  /* In case the user incorrectly uses the equal sign (for example
     with a short format or with space in the long format, then `arg`
     start with (if the short version was called) or be (if the long
     version was called with a space) the equal sign. So, here we
     check if the first character of arg is the equal sign, then the
     user is warned and the program is stopped: */
  if(arg && arg[0]=='=')
    argp_error(state, "Incorrect use of the equal sign (`=`). For short "
	       "options, `=` should not be used and for long options, "
	       "there should be no space between the option, equal sign "
	       "and value.");

  switch(key)
    {


    /* Input: */
    case 'M':
      allocatecopyset(arg, &p->up.maskname, &p->up.masknameset);
      break;
    case 'H':
      allocatecopyset(arg, &p->up.mhdu, &p->up.mhduset);
      break;
    case 'K':
      allocatecopyset(arg, &p->up.kernelname, &p->up.kernelnameset);
      break;
    case 'U':
      allocatecopyset(arg, &p->up.khdu, &p->up.khduset);
      break;
    case 'E':
      p->skysubtracted=1;
      p->up.skysubtractedset=1;
      break;
    case 'B':
      floatl0s1(arg, &p->minbfrac, "minbfrac", key, SPACK, NULL, 0);
      p->up.minbfracset=1;
      break;
    case 'F':
      sizetlzero(arg, &p->minnumfalse, "minnumfalse", key, SPACK, NULL, 0);
      p->up.minnumfalseset=1;
      break;

    /* Output: */

    /* Mesh grid: */
    case 's':
      sizetlzero(arg, &p->smp.meshsize, "smeshsize", key, SPACK, NULL, 0);
      p->up.smeshsizeset=1;
      break;
    case 'l':
      sizetlzero(arg, &p->lmp.meshsize, "lmeshsize", key, SPACK, NULL, 0);
      p->up.lmeshsizeset=1;
      break;
    case 'a':
      sizetlzero(arg, &p->smp.nch1, "nch1", key, SPACK, NULL, 0);
      p->up.nch1set=1;
      break;
    case 'b':
      sizetlzero(arg, &p->smp.nch2, "nch2", key, SPACK, NULL, 0);
      p->up.nch2set=1;
      break;
    case 'L':
      floatl0s1(arg, &p->smp.lastmeshfrac, "lastmeshfrac", key, SPACK,
                NULL, 0);
      p->up.lastmeshfracset=1;
      break;
    case 'd':
      floatl0(arg, &p->smp.mirrordist, "mirrordist", key, SPACK, NULL, 0);
      p->up.mirrordistset=1;
      break;
    case 'Q':
      floatl0s1(arg, &p->smp.minmodeq, "minmodeq", key, SPACK, NULL, 0);
      p->up.minmodeqset=1;
      break;
    case 511:
      p->smp.interponlyblank=1;
      break;
    case 'n':
      sizetlzero(arg, &p->smp.numnearest, "numnearest", key, SPACK, NULL, 0);
      p->up.numnearestset=1;
      break;
    case 'T':
      sizetpodd(arg, &p->smp.smoothwidth, "smoothwidth", key, SPACK, NULL, 0);
      p->up.smoothwidthset=1;
      break;
    case 500:
      p->meshname="a";  /* Just a placeholder! It will be corrected later */
      break;
    case 501:
      p->smp.fullinterpolation=1;
      p->up.fullinterpolationset=1;
      break;
    case 502:
      p->smp.fullsmooth=1;
      p->up.fullsmoothset=1;
      break;
    case 504:
      p->smp.fullconvolution=1;
      p->up.fullconvolutionset=1;
      break;
    case 516:
      p->smp.meshbasedcheck=1;
      break;


    /* Detection */
    case 't':
      floatl0s1(arg, &p->qthresh, "qthresh", key, SPACK, NULL, 0);
      p->up.qthreshset=1;
      break;
    case 'e':
      sizetelzero(arg, &p->erode, "erode", key, SPACK, NULL, 0);
      p->up.erodeset=1;
      break;
    case 506:
      int4or8(arg, &p->erodengb, "erodengb", key, SPACK, NULL, 0);
      p->up.erodengbset=1;
      break;
    case 'p':
      sizetelzero(arg, &p->opening, "opening", key, SPACK, NULL, 0);
      p->up.openingset=1;
      break;
    case 507:
      int4or8(arg, &p->openingngb, "openingngb", key, SPACK, NULL, 0);
      p->up.openingngbset=1;
      break;
    case 'u':
      floatl0(arg, &p->sigclipmultip, "sigclipmultip", key, SPACK,
              NULL, 0);
      p->up.sigclipmultipset=1;
      break;
    case 'r':
      floatl0s1(arg, &p->sigcliptolerance, "sigcliptolerance", key, SPACK,
              NULL, 0);
      p->up.sigcliptoleranceset=1;
      break;
    case 'R':
      anyfloat(arg, &p->dthresh, "dthresh", key, SPACK, NULL, 0);
      p->up.dthreshset=1;
      break;
    case 'i':
      sizetlzero(arg, &p->detsnminarea, "detsnminarea", key, SPACK, NULL, 0);
      p->up.detsnminareaset=1;
      break;
    case 510:
      sizetelzero(arg, &p->detsnhistnbins, "detsnhistnbins", key,
                  SPACK, NULL, 0);
      p->up.detsnhistnbinsset=1;
      break;
    case 'c':
      floatl0s1(arg, &p->detquant, "detquant", key, SPACK, NULL, 0);
      p->up.detquantset=1;
      break;
    case 'I':
      sizetelzero(arg, &p->dilate, "dilate", key, SPACK, NULL, 0);
      p->up.dilateset=1;
      break;
    case 505:
      p->threshname="a";
      break;
    case 508:
      p->detectionname="a";
      break;
    case 509:
      p->detectionskyname="a";
      break;
    case 503:
      p->detectionsnname="a";
      break;
    case 512:
      p->skyname="a";
      break;


    /* Segmentation: */
    case 'm':
      sizetlzero(arg, &p->segsnminarea, "segsnminarea", key, SPACK, NULL, 0);
      p->up.segsnminareaset=1;
      break;
    case 'g':
      floatl0s1(arg, &p->segquant, "segquant", key, SPACK, NULL, 0);
      p->up.segquantset=1;
      break;
    case 'v':
      p->keepmaxnearriver=1;
      break;
    case 'G':
      anyfloat(arg, &p->gthresh, "gthresh", key, SPACK, NULL, 0);
      p->up.gthreshset=1;
      break;
    case 'O':
      floatl0(arg, &p->objbordersn, "objbordersn", key, SPACK,
              NULL, 0);
      p->up.objbordersnset=1;
      break;

    case 514:
      sizetelzero(arg, &p->segsnhistnbins, "segsnhistnbins", key,
                  SPACK, NULL, 0);
      p->up.segsnhistnbinsset=1;
      break;
    case 513:
      p->segmentationname="a";
      break;
    case 515:
      p->clumpsnname="a";
      break;


    /* Operating modes: */


    /* Read the non-option arguments: */
    case ARGP_KEY_ARG:

      /* See what type of input value it is and put it in. */
      if( nameisfits(arg) )
        {
          if(p->up.inputname)
            argp_error(state, "Only one input image should be given.");
          else
            p->up.inputname=arg;
	}
      else
        argp_error(state, "%s is not a valid file type.", arg);
      break;





    /* The command line options and arguments are finished. */
    case ARGP_KEY_END:
      if(p->cp.setdirconf==0 && p->cp.setusrconf==0
	 && p->cp.printparams==0)
	{
	  if(state->arg_num==0)
	    argp_error(state, "No argument given!");
	  if(p->up.inputname==NULL)
	    argp_error(state, "No input FITS image(s) provided!");
	}
      break;





    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}





/* Specify the children parsers: */
struct argp_child children[]=
  {
    {&commonargp, 0, NULL, 0},
    {0, 0, 0, 0}
  };





/* Basic structure defining the whole argument reading process. */
static struct argp thisargp = {options, parse_opt, args_doc,
			       doc, children, NULL, NULL};

#endif