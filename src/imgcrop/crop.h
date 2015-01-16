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
#ifndef CROP_H
#define CROP_H

#include "main.h"


struct cropparams
{
  void *array;

  /* Pointer to basic structure: */
  struct imgcropparams *p;

  /* About input image. */
  size_t        imgindex;  /* Index of this image in the input names.  */
  fitsfile       *infits;  /* Pointer to the input FITS image.         */
  long         fpixel[2];  /* Position of first pixel in input image.  */
  long         lpixel[2];  /* Position of last pixel in input image.   */

  /* Output (cropped) image. */
  double        world[2];  /* World coordinates of crop center.        */
  double        sized[2];  /* Width and height of image in degrees.    */
  double      corners[8];  /* RA and Dec of this crop's four sides.    */
  double  equatorcorr[2];  /* Crop crosses the equator, see wcsmode.c. */
  size_t          outlen;  /* Length of output name.                   */
  size_t        outindex;  /* Index of this crop in the output list.   */
  fitsfile      *outfits;  /* Pointer to the output FITS image.        */

  /* Thread parameters. */
  size_t         *indexs;  /* Indexs to be used in this thread.        */
  pthread_barrier_t   *b;  /* pthread barrier to keep threads waiting. */
};

void
sectionparser(char *section, long *naxes, long *fpixel, long *lpixel);

void
cropname(struct cropparams *crp);

void
cropflpixel(struct cropparams *crp);

void
onecrop(struct cropparams *crp);

int
iscenterfilled(struct cropparams *crp);

void
printlog(struct imgcropparams *p);

#endif