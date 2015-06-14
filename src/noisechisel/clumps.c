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
#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>

#include "timing.h"
#include "checkset.h"
#include "forqsort.h"
#include "neighbors.h"
#include "arraymanip.h"
#include "linkedlist.h"
#include "statistics.h"
#include "fitsarrayvv.h"

#include "main.h"

#include "clumps.h"
#include "binary.h"
#include "detection.h"









/****************************************************************
 *****************   Over segmentation       ********************
 ****************************************************************/
/* This is very similar to the immersion method of Vincent &
   Soille(1991), but I will not separate the image into layers, and
   work based on the ordered flux values. If a certain pixel (at a
   certain level) has no neighbors it is a local maximum and will be
   assigned a new label. If it has a labeled neighbor, it will take
   that label and if there is more than one neighboring labeled region
   that pixel will be a `river` pixel.

   x0, y0, x1 and y1:

     - For the noise (mesh boxs), they specify the region in the image
       where this over-segmentation is taking place.

     - For the detections, they should be set to 0, 0, is0 and is1,
       where is0 and is1 are the height and width of the input image.
*/
void
oversegment(struct clumpsthreadparams *ctp)
{
  struct noisechiselparams *p=ctp->p;

  /* pix is not actually used its self, however, the pointer to it
     will be extensively used (ind) for the macro FILL_NGB_8_REGION.
     This macro works on the pointer to the index, not the index its
     self. `pix` is filled with different index values, so the pointer
     to it doesn't change. */
  size_t pix;

  float *arr=p->conv;
  struct sll *Q=NULL, *cleanup=NULL;
  long n1, rlab, nlab, curlab=1, *clab=p->clab;
  size_t x0=ctp->x0, y0=ctp->y0, x1=ctp->x1, y1=ctp->y1;
  size_t *n, *nf, *indf, *pind, *ind=&pix, is1=p->lmp.s1;
  size_t ng, *rn, *rnf, numngb, ngb[8], *relngb=p->relngb;

  /* Sort the indexs based on the flux within them. */
  qsort(ctp->inds, ctp->area, sizeof(size_t), indexfloatdecreasing);

  /* Initialize the region you want to over-segment. */
  indf=(pind=ctp->inds)+ctp->area;
  do clab[*pind]=SEGMENTINIT; while(++pind<indf);

  /* In the case where a connected region with the same flux or masked
     regions exists, some later indices might already be labeled. Note
     that in the convolved image that is being used here, the masked
     pixels have the smallest possible float value.  */
  indf=(pind=ctp->inds)+ctp->area;
  do
    /* When regions of a constant flux or masked regions exist, some
       later indexs (although they have same flux) will be filled
       before hand. If they are done, there is no need to do them
       again. */
    if(clab[*pind]==SEGMENTINIT)
      {
        /* View the over-segmentation pixel by pixel. Make sure that
           `--viewsegtf` is off, or else several copies will be made.

        if(p->data0_noise1==0 && p->mid==81 && pind!=p->indexs)
          {
            long *tmp;
            float *ftmp;
            static size_t counter=0;
            size_t tmpx0=495, tmpy0=499, tmpx1=510, tmpy1=508;

            if(counter==0)
              {
                floatshrinkarraytonew(arr, np->lmesh.s0, np->lmesh.s1,
                                      tmpx0, tmpy0, tmpx1, tmpy1, &ftmp);
                array_to_fits("overseg.fits", NULL, "1", FLOAT_IMG,
                              ftmp, tmpx1-tmpx0, tmpy1-tmpy0, NULL);
                free(ftmp);
                counter++;
              }

            if( *(pind-1)/is1>=tmpx0 && *(pind-1)/is1<tmpx1
                && *(pind-1)%is1>=tmpy0 && *(pind-1)%is1<tmpy1
                && counter<=(tmpx1-tmpx0)*(tmpy1-tmpy0) )
              {
                longshrinkarraytonew(clab, np->lmesh.s0, np->lmesh.s1,
                                     tmpx0, tmpy0, tmpx1, tmpy1, &tmp);
                array_to_fits("overseg.fits", NULL, "1", LONG_IMG,
                              tmp, tmpx1-tmpx0, tmpy1-tmpy0, NULL);
                free(tmp);
                counter++;
              }
          }
        */
        /* Some cases might happen where one or multiple regions of
           the pixels under study have the same flux. In particular
           note that masked pixels were all given a value of
           FLT_MAX. We have sorted the pixels by flux. So two equal
           valued pixels of two separate (but equal flux) regions
           might fall immediately after each other (For example two
           nearby stars whose centers are masked and are initially
           detected as one object because their wings touch above the
           noise).

           Therefore, if we see that the next pixel in the index list
           has the same flux as this one, it does not guarantee that
           it should be given the same label. Similar to the breadth
           first search algorithm for finding connected components, we
           will search all the neighbours and the neighbours of those
           neighbours that have the same flux of this pixel to see if
           they touch any label or not and to finally give them all
           the same label. */
        if( pind+1<indf && arr[*pind]==arr[*(pind+1)] )
          {
            n1=0;
            if(Q!=NULL || cleanup!=NULL)
              error(EXIT_FAILURE, 0, "A bug! Please contact us at %s so we "
                    "can fix this problem. In oversegment (clumps.c) Q and "
                    "cleanup should be NULL but while checking the equal "
                    "flux regions they aren't.", PACKAGE_BUGREPORT);
            add_to_sll(&Q, *pind);
            add_to_sll(&cleanup, *pind);
            clab[*pind]=SEGMENTTMPCHECK;

            /* Find all the pixels that have the same flux and are
               connected. */
            while(Q!=NULL)
              {
                /* Pop an element from the queue. */
                pop_from_sll(&Q, ind);

                /* Check the vicinity of this pixel that was just
                   popped to see if it can find any already labeled
                   neighbour or not. */
                FILL_NGB_8_REGION;

                /* If the pixel is on the side of the region, set it
                   as a river, no more need to look around it. */
                if(numngb<8)
                  clab[*ind]=SEGMENTRIVER;
                else
                  {
                    /* Begin looking into the neighbours of this pixel. */
                    nf=(n=ngb)+numngb;
                    do
                      {
                        /* If this neighbour has not been labeled yet
                           and has an equal flux, add it to the queue
                           to expand the studied region.*/
                        nlab=clab[*n];
                        if( nlab == SEGMENTINIT && arr[*n] == arr[*pind] )
                          {
                            clab[*n]=SEGMENTTMPCHECK;
                            add_to_sll(&cleanup, *n);
                            add_to_sll(&Q, *n);
                          }
                        /* If this neighbour has a positive nlab, it
                           means that it belongs to another object, so
                           if n1 has not been set for the whole region
                           put this label equal to n1. If n1 has been
                           set and is different from `nlab' then this
                           whole equal flux region should be a wide
                           river because it is connecting two
                           connected regions.*/
                        else if (nlab>0)
                          {
                            if(n1==0)
                              n1=nlab;
                            else if(nlab!=n1)
                              n1=SEGMENTRIVER;
                          }
                        /* If this neigbour has a label of zero, then
                           we are on the edge of the region. When
                           over-segmenting the noise and the
                           detections, clab is zero for the parts of
                           the image that we are not interested in
                           (detections and noise respectively). */
                        else if(nlab==0)
                          clab[*ind]=SEGMENTRIVER;
                      }
                    while(++n<nf);
                  }
              }

            /* Set the label that is to be given to this equal flux
               region. If n1 was set to any value, then that label
               should be used for the whole region. Otherwise, this is
               a new label.

               About the topinds checks:
               =========================

               The knowing the top index of a profile is very
               important in order to be able to successfully remove
               false detections. However, our convention of setting
               the masked pixels as the maximum floating point value
               is going to make some troubles in this regard.

               Masked pixels will always be checked first for each
               clump. The moment a new label is definied (curlab++),
               we are on the top index of a profile. If it is a NaN
               pixel (note that the NaN pixels do not change after
               convolution), then we can't assign it to the top flux
               of that profile. So we mark this by setting the value
               to the topinds for this profile to NOTOPIND which is an
               impossible index. later on, for every other pixel, we
               check to see if topinds has been set for this clump or
               not. If it hasn't, then since the pixels are ordered by
               flux, this index is the top index.
            */
            if(n1)
              {
                rlab = n1;
                if(ctp->topinds && ctp->topinds[rlab]==NOTOPIND
                   && n1>0 && !isnan(p->img[*pind]))
                  ctp->topinds[rlab]=*pind;
              }
            else
              {
                rlab = curlab++;
                if( ctp->topinds)
                  {
                    if( isnan(p->img[*pind]) )
                      ctp->topinds[rlab]=NOTOPIND;
                    else
                      ctp->topinds[rlab]=*pind;
                  }
              }

            /* Give the same label to the whole connected equal flux
               region, except those that might have been on the side
               of the image and were a river pixel. */
            while(cleanup!=NULL)
              {
                pop_from_sll(&cleanup, &pix);
                /* If it was on the sides of the image, it has been
                   changed to a river pixel. */
                if(clab[pix]==SEGMENTTMPCHECK)
                  clab[pix]=rlab;
              }
          }
        /* The flux of this pixel is not the same as the next sorted
           flux, so simply find the label for this object. */
        else
          {
            /* Check if the pixel is on the side of the image (for
               detections) or mesh box (for noise).*/
            if(*pind/is1==x0 || *pind%is1==y0
               || *pind/is1==x1-1 || *pind%is1==y1-1)
              n1=SEGMENTRIVER;
            else
              {
                /* Set the first neighbour's label to zero. */
                n1=0;
                if(Q!=NULL || cleanup!=NULL)
                  error(EXIT_FAILURE, 0, "A bug! Please contact us at %s "
                        "so we can fix this problem. In oversegment "
                        "(clumps.c) Q and cleanup should be NULL but while "
                        "checking the equal flux regions they aren't.",
                        PACKAGE_BUGREPORT);

                /* Go over all the 8 neighbors of this pixel and see
                   if all the neighbors that have a non-negative value
                   belong to one label or not. If the pixel is
                   neighboured by more than one label, set it as a
                   river pixel. Also if it is touching a zero valued
                   pixel (which does not belong to this object), set
                   it as a river pixel.

                   relngb was defined in ui.c: it keeps the relative
                   indexs of the neighbors of a pixel.*/
                rnf=(rn=relngb)+8;
                do
                  {
                    ng=*pind+*rn;
                    nlab=clab[ng];
                    if(nlab>0)
                      {
                        if(n1==0) n1=nlab;
                        else if(nlab!=n1)
                          {
                            n1=SEGMENTRIVER;
                            break;
                          }
                      }
                    else if(nlab==0)
                      {
                        n1=SEGMENTRIVER;
                        break;
                      }
                  }
                while(++rn<rnf);
              }

            /* Either assign a new label to this pixel, or give it the
               one of its neighbors. If n1 equals zero, then it is a
               new peak, and a new label should be created.  But if
               n1!=0, it is either a river pixel (has more than one
               labeled neighbor) or all its neighbors have the same
               label. */
            if(n1)
              {
                rlab = n1;
                if(ctp->topinds && ctp->topinds[rlab]==NOTOPIND
                   && n1>0 && !isnan(p->img[*pind]))
                  ctp->topinds[rlab]=*pind;
              }
            else
              {
                rlab = curlab++;
                if( ctp->topinds )
                  {
                    if( isnan(p->img[*pind]) )
                      ctp->topinds[rlab]=NOTOPIND;
                    else
                      ctp->topinds[rlab]=*pind;
                  }

              }

            /* Put the found label in the pixel. */
            clab[ *pind ] = rlab;
          }
      }
  while(++pind<indf);

  ctp->numclumps=curlab;
}




















/******************************************************************/
/*************            Grow clumps            ******************/
/******************************************************************/
/* Grow the true clumps in a detection. Note that unlike before, were
   river pixels would get a separate label for them selves, here, they
   don't, they just get set back to SEGMENTINIT. This is because the
   some of the pixels that lie immediately between two labeled regions
   might not be in the blankinds array (they were below the
   threshold). So we have to find river pixels later on after the
   growth is done independently. */
void
growclumps(struct clumpsthreadparams *ctp, int withrivers)
{
  long n1, *olab=ctp->p->olab;
  size_t *ind, *indf, thisround, numngb;
  size_t *n, *nf, numblanks=ctp->numblanks;
  size_t ngb[8], is0=ctp->p->lmp.s0, is1=ctp->p->lmp.s1;

  /* The basic idea is this: after growing, not all the blank pixels
     are necessarily filled, for example the pixels might belong to
     two regions above the growth threshold. So the pixels in between
     them (which are below the threshold will not ever be able to get
     a label). Therefore, the safest way we can terminate the loop of
     growing the objects is to stop it when the number of pixels left
     to fill in this round (thisround) equals the number of blanks.

     To start the loop, we set thisround=numblanks+1. Note that
     immediately after the loop has started, thisround is set to
     numblanks, so we will not be reading an un-initialized element.
  */
  thisround=numblanks+1;
  while(thisround>numblanks)
    {
      /* "thisround" will keep the number of pixels to be inspected in
	 this round. "numblanks" will count the number of pixels left
	 without an index by the end of this round. Since numblack
	 comes from the previous loop (or outside for the first loop)
	 it has to be saved in "thisround" to begin counting a
	 fresh. */
      thisround=numblanks;
      numblanks=0;

      /* Go over all the available indexs to fill: */
      indf = ( ind=ctp->blankinds ) + thisround;
      do
	{
	  /* We begin by assuming the neighbor label is zero (meaning
	     that no neighbor actually exists!) */
	  n1=0;

	  /* Check the 4 connected neighbors of the pixel: */
          FILL_NGB_4_ALLIMG;
	  nf=(n=ngb)+numngb;
	  do
            if( olab[*n]>0 )	             /* This neighbor is labeled. */
	    {
              if(n1)           /* A previous neighboring label was found. */
                {
                  if(n1!=olab[*n])      /* This neighbor has a new label. */
                    {              /* So it should be set to SEGMENTINIT. */
                      n1=SEGMENTINIT;     /* IMPORTANT: not SEGMENTRIVER. */
                      break;                   /* See explanations above. */
                    }
                }
              else              /* This is the first labeled     */
                {               /* neighbor that is found.       */
                  n1=olab[*n];
                  if(!withrivers) break;
                }
	    }
	  while(++n<nf);

	  /* The loop above finishes with three possibilities:
  	       n1==0            --> No labeled neighbor was found.
	       n1==SEGMENTINIT  --> It is connecting two labeled regions.
	       n1>0             --> It only has one neighbouring label.*/
	  if(n1==0)		/* First condition above. */
	    ctp->blankinds[numblanks++]=*ind;
	  else			/* Last two conditions above. */
            olab[*ind]=n1;
	}
      while(++ind<indf);
    }

  ctp->numblanks=numblanks;
}


















/******************************************************************/
/*************             Clump S/N             ******************/
/******************************************************************/
/* In this function we want to find the general information for each
   clump in an over-segmented labeled array. The signal in each clump
   is the average signal inside it subtracted by the average signal in
   the river pixels around it. So this function will go over all the
   pixels in the object (already found in deblendclumps()) and add
   them appropriately.

   The output is an array of size numseg*INFOTABCOLS. INFOTABCOLS=4. The
   columns are:
   0: Average signal in clump.
   1: Number of pixels in clump.
   2: Average signal around clump.
   3: Number of pixels around clump.
   4: Standard deviation on flux weighted center of clump.
*/
void
getclumpinfo(struct clumpsthreadparams *ctp, double **outclumpinfo)
{
  struct noisechiselparams *p=ctp->p;
  struct meshparams *smp=&p->smp;

  double *xys, *clumpinfo;
  float *img=p->img, *smpstd=smp->garray2;
  size_t lab, is0=p->lmp.s0, is1=p->lmp.s1;
  long *clab=p->clab, wngb[WNGBSIZE], ngblab;
  size_t x0=ctp->x0, y0=ctp->y0, x1=ctp->x1, y1=ctp->y1;
  size_t i=0, ii=0, row, *n, *nf, ngb[8], *ind, *indf, numngb;

  /* Just make sure that the box size is not only around one pixel! */
  if(x1-x0<=1 || y1-y0<=1)
    error(EXIT_FAILURE, 0, "A bug! Please contact us at %s so we can find "
          "and fix the problem in clumpinfo (clumps.c). For some reason, "
          "the specified input region is %lu by %lu wide.",
          PACKAGE_BUGREPORT, y1-y0, x1-x0);

  /* Allocate the clump information array. */
  errno=0;
  clumpinfo=calloc(ctp->numclumps*INFOTABCOLS, sizeof *clumpinfo);
  if(clumpinfo==NULL)
    error(EXIT_FAILURE, errno, "%lu bytes for clumpinfo (clumps.c)",
          ctp->numclumps*INFOTABCOLS*sizeof *clumpinfo);
  *outclumpinfo=clumpinfo;

  /* If the image is sky subtracted or if we are working on the
     detections (not noise), we will need the light weighted center of
     each clump. In the former, for finding the error in measuring the
     sky (through the smp->garray2) which is done in this same
     function. In the latter case, in order to find the signal to
     noise threshold on lmp->garray1. */
  if(p->skysubtracted || p->b0f1)
    {
      errno=0; xys=ctp->xys=calloc(2*ctp->numclumps, sizeof *xys);
      if(xys==NULL)
        error(EXIT_FAILURE, errno, "%lu bytes for xys (clumps.c)",
              2*ctp->numclumps*sizeof *xys);
    }
  else xys=NULL; /* To check if it is needed below. */

  /* Go over all the pixels in this set of pixels and fill in the
     proper information for each clump. */
  indf = (ind=ctp->inds) + ctp->area;
  do
    if(!isnan(img[*ind]))
      {
        if(clab[*ind]==SEGMENTRIVER) /* We are on a river. */
          {
            /* Fill in the neighbours arrary for this pixel. If we are
               working on the mesh grid (the noise), then we only want
               the neighbors within a region. Otherwise (when working
               on the detections) we want the neighbors on the full
               image.*/
            if(p->b0f1) {FILL_NGB_8_ALLIMG}
            else        {FILL_NGB_8_REGION}


            /* We are on a river pixel. So its value has to be added
               to the borders of any object it touches. But since it
               might touch a labeled region more than once, we use
               wngb to keep track of which label we have already added
               its value to. `ii` is the number of different labels
               this river pixel has already been added to. wngb will
               keep the labels. */
            ii=0;
            memset(wngb, 0, sizeof(wngb));

            /* Look into the 8-connected neighbors: */
            nf=(n=ngb)+numngb;
            do
              if( (ngblab=clab[ *n ]) >0)
                {
                  /* Go over wngb to see if this river pixel's value
                     has been added to a segment or not. */
                  for(i=0;i<ii;++i)
                    if(wngb[i]==ngblab)
                      /* It is already present. break out. */
                      break;

                  if(i==ii) /* This label was not added yet. */
                    {
                      clumpinfo[ ngblab*INFOTABCOLS+2 ]+=img[*ind];
                      ++clumpinfo[ ngblab*INFOTABCOLS+3 ];
                      wngb[ii]=ngblab;
                      ++ii;
                    }
                }
            while(++n<nf);
          }
        else                    /* We are on a clump. */
          {
            lab=clab[*ind];
            ++clumpinfo[ lab * INFOTABCOLS + 1 ];
            clumpinfo[ lab * INFOTABCOLS ] += img[*ind];
            if(xys)
              {
                xys[ 2 * lab     ] += (*ind/is1) * img[*ind];
                xys[ 2 * lab + 1 ] += (*ind%is1) * img[*ind];
              }

          }
      }
  while(++ind<indf);

  /* Do the final preparations. All the calculations are only
     necessary for the clumps that satisfy the minimum area. So there
     is no need to waste time on the smaller ones. */
  for(lab=1;lab<ctp->numclumps;++lab)
    {
      row=lab*INFOTABCOLS;
      if(clumpinfo[row+1]>p->segsnminarea)
        {
          /* Find the index of the flux weighted center and use it to
             find the standard deviation for this clump. Note that
             this is only needed if the input image was already sky
             subtracted. If it wasn't, then we are not subtracting the
             sky to worry about its error! The error in a pixel flux
             measurement is simply its square root. */
          if(xys)
            {
              /* Especially for noise, it might happen that the total
                 flux or any of the positions multiplied by flux
                 becomes negative. If left unchecked, it will result
                 in checking the wrong pointers, so we need to check
                 them at this point. */
              if(clumpinfo[row]<0.0f || xys[2*lab]<0.0f
                 || xys[2*lab+1]<0.0f)
                {
                  clumpinfo[row+1]=0;
                  xys[2*lab]=xys[2*lab+1]=NAN;
                  continue;
                }
              else
                {
                  /* It is important that xys are corrected
                     (divided by the total flux here). Because they
                     are used later when we don't have access to
                     clumpinfo any more. */
                  xys[2*lab  ] /= clumpinfo[row];
                  xys[2*lab+1] /= clumpinfo[row];
                  if(p->skysubtracted)
                    clumpinfo[row+4]=
                      smpstd[imgxytomeshid(smp, xys[2*lab],
                                           xys[2*lab+1])];

                  /* For a check:
                  printf("%lu: (%lu, %lu) --> %f\n", lab,
                         (size_t)(xys[2*lab+1] / clumpinfo[row]) +1,
                         (size_t)(xys[2*lab] / clumpinfo[row]) +1,
                         clumpinfo[row+4]);
                  */
                }
            }

          /* Convert sum to average. IMPORTANT NOTE: This has to be
             done after the step above, because there, we need the
             total flux to find the flux weighted center positions. */
          clumpinfo[row  ] /= clumpinfo[row+1];
          clumpinfo[row+2] /= clumpinfo[row+3];
        }
      else
        if(xys) xys[2*lab]=xys[2*lab+1]=NAN;
    }

  /* To check a specific detection (p->b0f1==1) or a noise mesh grid
     (p->b0f1==0). set ctp->thislabel to the desired label.
  if(p->b0f1==1 && ctp->thislabel==1)
    for(i=1;i<ctp->numclumps;++i)
      printf("%lu: %-10.3f %-5.0f %-10.3f %-5.0f %-10.3f\n", i,
             clumpinfo[i*INFOTABCOLS], clumpinfo[i*INFOTABCOLS+1],
             clumpinfo[i*INFOTABCOLS+2], clumpinfo[i*INFOTABCOLS+3],
             clumpinfo[i*INFOTABCOLS+4]);
  */

  /* If we are on the noise clumps, then xys is not needed any more.
     So if it was allocated, fre the space. */
  if(p->b0f1==0 && xys) free(xys);
}





void
clumpsntable(struct clumpsthreadparams *ctp, float **sntable)
{
  struct noisechiselparams *p=ctp->p;

  float *sntab;
  double *clumpinfo, err;
  size_t i, ind, row, counter=0;
  double I, O, Ni, cpscorr=p->cpscorr;


  /* Get the information for all the segments. */
  getclumpinfo(ctp, &clumpinfo);


  /* Allocate the signal to noise table. */
  errno=0;
  sntab = *sntable = malloc( ctp->numclumps * sizeof *sntab );
  if(sntab==NULL)
    error(EXIT_FAILURE, errno, "%lu bytes for sntab (clumps.c)",
          ctp->numclumps*sizeof *sntab);


  /* Start calculating the Signal to noise ratios. */
  sntab[0]=0;
  for(i=1;i<ctp->numclumps;++i)
    {
      /* These variables used for easy readability. */
      row=i*INFOTABCOLS;
      I  = clumpinfo[ row     ];
      Ni = clumpinfo[ row + 1 ];
      O  = clumpinfo[ row + 2 ];

      /* If the inner flux is smaller than the outer flux (happens
	 only in noise cases) or the area is smaller than the minimum
	 area to calculate signal-to-noise, then set the S/N of this
	 segment to zero. */
      if( Ni>p->segsnminarea && I>O )   /* This is O, not 0 (zero). */
	{
          /* If the sky was subtracted then put in the second power of
             the standard deviation multiplied by two (because we are
             measuring two fluxs). */
          err  = ( p->skysubtracted
                   ? ( 2.0f * clumpinfo[row+4] * clumpinfo[row+4] )
                   : 0.0f );

          /* Calculate the Signal to noise ratio, if we are on the
             noise regions, we don't care about the IDs of the clumps
             anymore, so store the Signal to noise ratios contiguously
             (for easy sorting and etc). Note that counter will always
             be smaller and equal to i. */
          ind = p->b0f1 ? i : counter++;
	  sntab[ind]=( sqrt((float)(Ni)/cpscorr)*(I-O)
		       / sqrt( (I>0?I:-1*I) + (O>0?O:-1*O) + err ) );
	}
      else
	sntab[i]=0;
    }


  /* If we are dealing with noise, replace the number of clumps with
     the number of those with a sufficient area and inner flux. */
  if(p->b0f1==0)
    ctp->numclumps=counter;


  /* To check a specific detection (p->b0f1==1) or a noise mesh grid
     (p->b0f1==0):
  if(p->b0f1==1 && ctp->thislabel==1)
    for(i=1;i<ctp->numclumps;++i)
      printf("%lu: %-10.3f %-5.0f %-10.3f --> %-10.3f (%-6.0f, %-6.0f)\n", i,
             clumpinfo[i*INFOTABCOLS], clumpinfo[i*INFOTABCOLS+1],
             clumpinfo[i*INFOTABCOLS+2], sntab[i],
             ctp->xys[i*2], ctp->xys[i*2+1]);
  */
  /* Clean up */
  free(clumpinfo);
}


















/******************************************************************/
/*************           S/N threshold           ******************/
/******************************************************************/
void *
clumpsnthreshonmesh(void *inparams)
{
  struct clumpsthreadparams ctp;
  struct meshthreadparams *mtp=(struct meshthreadparams *)inparams;
  struct meshparams *mp=mtp->mp;
  struct noisechiselparams *p=(struct noisechiselparams *)mp->params;

  size_t *mponeforall=mp->oneforall;

  float *sntable;
  char cline[1000];
  long *clab=p->clab;
  size_t s0, s1, nf, ind, startind, is1=mp->s1;
  size_t i, *indexs=&mp->indexs[mtp->id*mp->thrdcols];
  char suffix[50], *histname, *segmentationname=p->segmentationname;

  /* Set the necessary pointers for the clumpsthreadparams
     structure. */
  ctp.p=p;
  ctp.topinds=NULL;             /* For noise, we don't want topinds. */
  ctp.inds=&mponeforall[mtp->id*mp->maxs0*mp->maxs1];

  /* Go over all the meshs that are assigned to this thread. */
  for(i=0;indexs[i]!=NONTHRDINDEX;++i)
    {
      /* Set index of this mesh: */
      ind=ctp.thislabel=indexs[i];

      /* Find the necessary parameters */
      startind=mp->start[ind];
      s0=mp->ts0[mp->types[ind]];
      s1=mp->ts1[mp->types[ind]];
      ctp.x1 = (ctp.x0=startind/is1) + s0;
      ctp.y1 = (ctp.y0=startind%is1) + s1;


      /* Check to see if we have enough blank area for getting the
	 background noise statistics. */
      count_f_b_onregion(p->byt, startind, s0, s1, is1, &nf, &ctp.area);
      if( (float)ctp.area < (float)(s0*s1)*p->minbfrac )
        {
          if(segmentationname)
            longinitonregion(clab, 0, startind, s0, s1, is1);
          continue;
        }


      /* We want to find the clumps on the noise, not the signal, the
         number of noise pixels in this mesh were calculated above
         (area). Here we want to pull out the indexs of those pixels
         which is necessary for the over-segmentation. */
      index_f_b_onregion(p->byt, startind, s0, s1, is1, ctp.inds, 0);


      /* Do the over-segmentation and put the number of clumps in
         ctp.numclumps */
      oversegment(&ctp);
      if(ctp.numclumps<p->minnumfalse)
        {
          if(segmentationname)
            longinitonregion(clab, 0, startind, s0, s1, is1);
          continue;
        }


      /* Find the signal to noise of all the clumps. */
      clumpsntable(&ctp, &sntable);


      /* Continue if the number of clumps is enough: */
      if(ctp.numclumps<p->minnumfalse)
        {
          free(sntable);
          if(segmentationname)
            longinitonregion(clab, 0, startind, s0, s1, is1);
          continue;
        }


      /* Sort the Signal to noise ratio values and remove outliers. */
      qsort(sntable, ctp.numclumps, sizeof *sntable, floatincreasing);
      removeoutliers_flatcdf(sntable, &ctp.numclumps);
      if(ctp.numclumps<p->minnumfalse)
        {
          free(sntable);
          if(segmentationname)
            longinitonregion(clab, 0, startind, s0, s1, is1);
          continue;
        }


      /* Put the signal to noise quantile into the mesh grid. */
      mp->garray1[ind]=sntable[indexfromquantile(ctp.numclumps, p->segquant)];


      /* If the user has asked for it, make the histogram of the
         S/N distribution. */
      if(p->segsnhistnbins)
        {
          /* histname has to be set to NULL so automaticoutput can
             safey free it. */
          histname=NULL;
          sprintf(suffix, "_%lu_detsn.txt", ind);
          sprintf(cline, "# %s\n# %s started on %s"
                  "# Input: %s (hdu: %s)\n"
                  "# Histogram for S/N distribution of false "
                  "clumps.\n"
                  "# On large mesh id %lu.\n"
                  "# The %.3f quantile has a value of %.4f on "
                  "this bin.", SPACK_STRING, SPACK_NAME,
                  ctime(&p->rawtime), p->up.inputname, p->cp.hdu,
                  ind, p->segquant, mp->garray1[ind]);
          automaticoutput(p->up.inputname, suffix, p->cp.removedirinfo,
                          p->cp.dontdelete, &histname);
          savehist(sntable, ctp.numclumps, p->segsnhistnbins,
                   histname, cline);
          free(histname);
        }

      /* Cleanup: */
      free(sntable);
    }

  /* Free any allocated space and if multiple threads were used, wait
     until all other threads finish. */
  if(mp->numthreads>1)
    pthread_barrier_wait(&mp->b);
  return NULL;
}





void
clumpsngrid(struct noisechiselparams *p)
{
  float snave;
  char report[VERBMSGLENGTHS2_V];
  struct meshparams *lmp=&p->lmp;

  /* Set the convolved image as the basis for sorting the indexs and
     finding clumps. */
  forqsortindexarr=p->conv;

  /* Find the clump Signal to noise ratio on successful meshs then on
     all the meshs. findsnthreshongrid is in detection.c. */
  operateonmesh(lmp, clumpsnthreshonmesh, sizeof(size_t), 0, 1);
  findsnthreshongrid(&p->lmp, p->clumpsnname, "Interpolating the "
                     "CLUMP Signal to noise ratio threshold", p->wcs);
  if(p->cp.verb)
    {
      snave=floataverage(lmp->garray1, lmp->nmeshi);
      sprintf(report, "Clump S/N limit found (Average: %.3f).",
              snave);
      reporttiming(NULL, report, 2);
    }

}




















/******************************************************************/
/*************        Remove false clumps        ******************/
/******************************************************************/
/* Given the signal to noise of each segment (in sntable), and a
   threshold for an acceptable S/N (in `p`), remove those segments
   that don't satisfy the criteria and correct the number of clumps. */
void
removefalseclumps(struct clumpsthreadparams *ctp, float *sntable)
{
  struct meshparams *lmp=&ctp->p->lmp;

  double *xys=ctp->xys;
  float *snonmesh=lmp->garray1;
  long *newlabs, *clab=ctp->p->clab;
  size_t *n, *nf, is0=lmp->s0, is1=lmp->s1;
  size_t i, curlab=1, *ind, *indf, ngb[8], numngb;

  /* Allocate space for the new labels array. */
  errno=0; newlabs=malloc(ctp->numclumps*sizeof *newlabs);
  if(newlabs==NULL)
    error(EXIT_FAILURE, errno, "%lu bytes for newlabs in "
          "removefalsedetections (clumps.c)", ctp->numclumps*sizeof *newlabs);

  /* We want the removed regions to become SEGMENTINIT. */
  longinit(newlabs, ctp->numclumps, SEGMENTINIT);

  /* Set the new labels: */
  if(ctp->p->keepmaxnearriver)
    {       /* `{' because We don't want to confuse the `else' below. */
      for(i=1;i<ctp->numclumps;++i)
        if( !isnan(xys[2*i]) && sntable[i] >
            snonmesh[ imgxytomeshid(lmp, xys[2*i], xys[2*i+1]) ] )
          newlabs[i]=curlab++;
    }
  else
    for(i=1;i<ctp->numclumps;++i)
      if(ctp->topinds[i]!=NOTOPIND)
        {
          /* Check to see if the brightest pixel in this clump is
             touching a river or not. */
          ind=&ctp->topinds[i];
          FILL_NGB_8_ALLIMG;
          nf=(n=ngb)+numngb;
          do if(clab[*n++]==SEGMENTRIVER) break; while(n<nf);

          /* If the brightest pixel of this clump was not touching a
             river and its Signal to noise ratio is larger than the
             threshold, then give it a new label.*/
          if(n==nf && !isnan(xys[2*i]) && sntable[i] >
             snonmesh[ imgxytomeshid(lmp, xys[2*i], xys[2*i+1]) ] )
            newlabs[i]=curlab++;
        }
  ctp->numclumps=curlab;

  /* Change the values of the false clumps. Note that the labels are
     either SEGMENTRIVER or a label */
  indf = (ind=ctp->inds) + ctp->area;
  do
    {
      if(clab[*ind]>0)
	clab[*ind] = newlabs[ clab[*ind] ];
      else
	clab[*ind] = SEGMENTINIT;
    }
  while(++ind<indf);

  /* Cleanup. */
  free(newlabs);
}
