/*********************************************************************
Image Crop - Crop a given size from one or multiple images.
Image Crop is part of GNU Astronomy Utilities (AstrUtils) package.

Copyright (C) 2013-2015 Mohammad Akhlaghi
Tohoku University Astronomical Institute, Sendai, Japan.
http://astr.tohoku.ac.jp/~akhlaghi/

AstrUtils is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

AstrUtils is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with AstrUtils. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#ifndef MAIN_H
#define MAIN_H


#include "config.h"
#include "linkedlist.h"
#include "fitsarrayvv.h"
#include "commonparams.h"



/* Progarm name macros: */
#define SPACK_VERSION   "0.1"
#define SPACK           "astrimgcrop" /* Subpackage executable name. */
#define SPACK_NAME      "ImageCrop"  /* Subpackage full name.       */
#define SPACK_STRING    SPACK_NAME" ("PACKAGE_STRING") "SPACK_VERSION
#define LOGFILENAME     SPACK".log"




/* Structure for the log file. Since we are operating in parallel
   mode, writing to a file will significantly decrease the speed. So
   we will make an array to keep the status of each output.*/
struct imgcroplog
{
  char         *name; /* The name of this output.                  */
  size_t      numimg; /* The number of images used in this output. */
  int   centerfilled; /* Is the center filled? (0 or 1)            */
};





/* The sides of the image keep the celestial coordinates of the four
   sides of this image. With respect to the pixels they are.
*/
struct inputimgs
{
  char             *name;  /* File name of input image.                   */
  long          naxes[2];  /* Size of the image.                          */
  int               nwcs;  /* Number of WCS in each input image.          */
  struct wcsprm     *wcs;  /* WCS structure of each input image.          */
  char           *wcstxt;  /* Text output of each WCS.                    */
  int           nwcskeys;  /* Number of keywords in the header WCS.       */
  double      corners[8];  /* RA and Dec of this image corners (within).  */
  double        sized[2];  /* Width and height of image in degrees.       */
  double  equatorcorr[2];  /* If image crosses the equator, see wcsmode.c.*/
};





/* User interface parameters: */
struct uiparams
{
  char      *catname;  /* Catalog file name.                            */
  struct stll  *stll;  /* Linked list keeping input file names.         */

  /* Check if all parameters are read (use .def file for
     comparison). The non optional parameters (like the catalog and
     input FITS images that come in from arguments, not options) are
     checked in the args.h files. */
  int         catset;
  int     imgmodeset;
  int     wcsmodeset;
  int       racolset;
  int      deccolset;
  int          raset;
  int         decset;
  int        xcolset;
  int        ycolset;
  int          xcset;
  int          ycset;
  int      iwidthset;
  int      wwidthset;
  int     sectionset;
  int      suffixset;
  int checkcenterset;
};






/* Main program parameters: */
struct imgcropparams
{
  /* Before actual program: */
  struct commonparams cp;  /* Common parameters.                       */
  struct uiparams     up;  /* User interface parameters.               */

  /* Operating modes: */
  int            imgmode;  /* ==1: will use X and Y coordiates.        */
  int            wcsmode;  /* ==1: will use Ra and Dec coordiates.     */

  /* Input */
  size_t          numimg;  /* Number of given image names.             */
  size_t            xcol;  /* Catalog X column                         */
  size_t            ycol;  /* Catalog Y column                         */
  int            noblank;  /* ==1: no blank (out of image) pixels.     */
  char          *section;  /* Section string.            */
  double              xc;  /* The center point, one crop (FITS stnrd). */
  double              yc;  /* The center point, one crop (FITS stnrd). */
  long         iwidth[2];  /* Image mode width (in pixels).            */
  size_t           racol;  /* Catalog RA column                        */
  size_t          deccol;  /* Catalog Dec column                       */
  double              ra;  /* RA of one crop box center.               */
  double             dec;  /* Dec of one crop box center.              */
  double             res;  /* Resolution in arcseconds                 */
  double          wwidth;  /* WCS mode width (in arcseconds).          */
  size_t     checkcenter;  /* width of a box to check for zeros        */
  int    keepblankcenter;  /* ==1: If center is not filled, remove.    */
  int     zeroisnotblank;  /* ==1: In float or double, keep 0.0 pixels.*/

  /* Output: */
  char           *suffix;  /* Ending of output file name.              */

  /* INTERNAL PARAMETERS: */
  struct inputimgs *imgs;  /* Basic WCS and size information for input.*/
  struct imgcroplog *log;  /* To keep the log of the outputs.          */
  time_t         rawtime;  /* Starting time of the program.            */
  int      outnameisfile;  /* Output filename is a directory.          */
  double            *cat;  /* Data of catalog.                         */
  size_t             cs0;  /* Number of rows in the catalog.           */
  size_t             cs1;  /* Number of columns in the catalog.        */
  int             bitpix;  /* BITPIX value for this image.             */
  void           *bitnul;  /* Null value for this data-type.           */
  int           datatype;  /* CFITSIO datatype value for this image.   */
};




/* Function declarations: */
void
imgcrop(struct imgcropparams *p);

#endif