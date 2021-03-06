/*********************************************************************
match -- Functions to match catalogs and WCS.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2017, Free Software Foundation, Inc.

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
#include <errno.h>
#include <error.h>
#include <stdlib.h>

#include <gsl/gsl_sort.h>

#include <gnuastro/box.h>
#include <gnuastro/list.h>
#include <gnuastro/permutation.h>









/**********************************************************************/
/*****************   Coordinate match custom list   *******************/
/**********************************************************************/
struct match_coordinate_sfll
{
  float f;
  size_t v;
  struct match_coordinate_sfll *next;
};





static void
match_coordinate_add_to_sfll(struct match_coordinate_sfll **list,
                             size_t value, float fvalue)
{
  struct match_coordinate_sfll *newnode;

  errno=0;
  newnode=malloc(sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: new node couldn't be allocated",
          __func__);

  newnode->v=value;
  newnode->f=fvalue;
  newnode->next=*list;
  *list=newnode;
}





static void
match_coordinate_pop_from_sfll(struct match_coordinate_sfll **list,
                               size_t *value, float *fvalue)
{
  struct match_coordinate_sfll *tmp;
  tmp=*list;
  *value=tmp->v;
  *fvalue=tmp->f;
  *list=tmp->next;
  free(tmp);
}




















/********************************************************************/
/*************            Coordinate matching           *************/
/*************      Sanity checks and preparations      *************/
/********************************************************************/
/* Since these checks are repetative, its easier to have a separate
   function for both inputs. */
static void
match_coordinate_sanity_check_columns(gal_data_t *coord, char *info)
{
  gal_data_t *tmp;

  for(tmp=coord; tmp!=NULL; tmp=tmp->next)
    {
      if(tmp->type!=GAL_TYPE_FLOAT64)
        error(EXIT_FAILURE, 0, "%s: the input coordinates must have "
              "`float64' type. At least one node of the %s list has type "
              "of `%s'", __func__, info, gal_type_name(tmp->type, 1));

      if(tmp->ndim!=1)
        error(EXIT_FAILURE, 0, "%s: each input coordinate column must have "
              "a single dimension (be a single column). Atleast one node of "
              "the %s list has %zu dimensions", __func__, info, tmp->ndim);

      if(tmp->size!=coord->size)
        error(EXIT_FAILURE, 0, "%s: the nodes of each list of coordinates "
              "must have the same number of elements. At least one node of "
              "the %s list has %zu elements while the first has %zu "
              "elements", __func__, info, tmp->size, coord->size);
    }
}





/* To keep the main function clean, we'll do the sanity checks here. */
static void
match_coordinaes_sanity_check(gal_data_t *coord1, gal_data_t *coord2,
                              gal_data_t *aperture)
{
  double *aper=aperture->array;
  size_t ncoord1=gal_list_data_number(coord1);

  /* Make sure both lists have the same number of datasets. NOTE: they
     don't need to have the same number of elements. */
  if( ncoord1!=gal_list_data_number(coord2) )
    error(EXIT_FAILURE, 0, "%s: the two inputs have different numbers of "
          "datasets (%zu and %zu respectively)", __func__, ncoord1,
          gal_list_data_number(coord2));


  /* This function currently only works for 2 dimensions. */
  if(ncoord1!=2)
    error(EXIT_FAILURE, 0, "%s: inputs correspond to %zu dimensions, this "
          "function currently only works on 2 dimensional datasets",
          __func__, ncoord1);

  /* Check the column properties. */
  match_coordinate_sanity_check_columns(coord1, "first");
  match_coordinate_sanity_check_columns(coord2, "second");

  /* Check the aperture values. */
  if(aper[0]<=0)
    error(EXIT_FAILURE, 0, "%s: the first value in the aperture (%g) is the "
          "semi-major axis and thus not be zero or negative", __func__,
          aper[0]);
  if(aper[1]<=0 || aper[1]>1)
    error(EXIT_FAILURE, 0, "%s: the second value in the aperture (%g) is "
          "the axis ratio, so it must be larger than zero and less than 1",
          __func__, aper[1]);
}





/* To keep things clean, the sorting of each input array will be done in
   this function. */
static size_t *
match_coordinates_prepare_sort(gal_data_t *coords, size_t minmapsize)
{
  gal_data_t *tmp;
  size_t *permutation=gal_data_malloc_array(GAL_TYPE_SIZE_T, coords->size,
                                            __func__, "permutation");

  /* Get the permutation necessary to sort all the columns (based on the
     first column). */
  gsl_sort_index(permutation, coords->array, 1, coords->size);

  /* Sort all the coordinates. */
  for(tmp=coords; tmp!=NULL; tmp=tmp->next)
    gal_permutation_apply(tmp, permutation);

  /* Clean up. */
  return permutation;
}





/* Do the preparations for matching of coordinates. */
static void
match_coordinates_prepare(gal_data_t *coord1, gal_data_t *coord2,
                          int sorted_by_first, int inplace,
                          gal_data_t **A_out, gal_data_t **B_out,
                          size_t **A_perm, size_t **B_perm,
                          size_t minmapsize)
{
  gal_data_t *c, *tmp, *A=NULL, *B=NULL;

  /* Sort the datasets if they aren't sorted. If the dataset is already
     sorted, then `inplace' is irrelevant. */
  if(!sorted_by_first)
    {
      /* Allocating a new list is only necessary when  */
      if(!inplace)
        {
          /* Copy the first list. */
          for(tmp=coord1; tmp!=NULL; tmp=tmp->next)
            {
              c=gal_data_copy(tmp);
              c->next=NULL;
              gal_list_data_add(&A, c);
            }

          /* Copy the second list. */
          for(tmp=coord2; tmp!=NULL; tmp=tmp->next)
            {
              c=gal_data_copy(tmp);
              c->next=NULL;
              gal_list_data_add(&B, c);
            }

          /* Reverse both lists: the copying process reversed the order. */
          gal_list_data_reverse(&A);
          gal_list_data_reverse(&B);

          /* Set the output pointers. */
          *A_out=A;
          *B_out=B;
        }
      else
        {
          *A_out=coord1;
          *B_out=coord2;
        }

      /* Sort each dataset by the first coordinate. */
      *A_perm = match_coordinates_prepare_sort(*A_out, minmapsize);
      *B_perm = match_coordinates_prepare_sort(*B_out, minmapsize);
    }
  else
    {
      *A_out=coord1;
      *B_out=coord2;
    }
}




















/********************************************************************/
/*************            Coordinate matching           *************/
/********************************************************************/
static double
match_coordinates_elliptical_r(double d1, double d2, double *ellipse,
                               double c, double s)
{
  double Xr = d1 * ( c       )     +   d2 * ( s );
  double Yr = d1 * ( -1.0f*s )     +   d2 * ( c );
  return sqrt( Xr*Xr + Yr*Yr/ellipse[1]/ellipse[1] );
}






/* Go through both catalogs and find which records/rows in the second
   catalog (catalog b) are within the acceptable distance of each record in
   the first (a). */
static void
match_coordinates_second_in_first(gal_data_t *A, gal_data_t *B,
                                  gal_data_t *aperture,
                                  struct match_coordinate_sfll **bina)
{
  /* To keep things easy to read, all variables related to catalog 1 start
     with an `a' and things related to catalog 2 are marked with a `b'. The
     redundant variables (those that equal a previous value) are only
     defined to make it easy to read the code.*/
  double dist[2];
  size_t ar=A->size, br=B->size;
  size_t ai, bi, blow, prevblow=0;
  double r, c, s, *aper=aperture->array;
  double *a[2]={A->array, A->next->array};
  double *b[2]={B->array, B->next->array};
  int iscircle=((double *)(aperture->array))[1]==1 ? 1 : 0;

  /* Preparations for the shape of the aperture. */
  if(iscircle)
    dist[0]=dist[1]=aper[0];
  else
    {
      /* Using the box that encloses the aperture, calculate the distance
         along each axis. */
      gal_box_bound_ellipse_extent(aper[0], aper[0]*aper[1], aper[2], dist);

      /* Calculate the sin and cos of the given ellipse if necessary for
         ease of processing later. */
      c = cos( aper[3] * M_PI/180.0 );
      s = sin( aper[3] * M_PI/180.0 );
    }


  /* For each row/record of catalog `a', make a list of the nearest records
     in catalog b within the maximum distance. Note that both catalogs are
     sorted by their first axis coordinate.*/
  for(ai=0;ai<ar;++ai)
    {
      /* Initialize `bina'. */
      bina[ai]=NULL;

      /* Find the first (lowest first axis value) row/record in catalog `b'
	 that is within the search radius for this record of catalog
	 `a'. `blow' is the index of the first element to start searching
	 in the catalog `b' for a match to `a[][ai]' (the record in catalog
	 a that is currently being searched). `blow' is only based on the
	 first coordinate, not the second.

         Both catalogs are sorted by their first coordinate, so the `blow'
	 to search for the next record in catalog `a' will be larger or
	 equal to that of the previous catalog `a' record. To account for
	 possibly large distances between the records, we do a search here
	 to change `blow' if necessary before doing further searching.*/
      for( blow=prevblow; blow<ar && b[0][blow] < a[0][ai]-dist[0]; ++blow)
	{/* This can be blank, the `for' does all we need :-). */}


      /* `blow' is now found for this `ai' and will be used unchanged to
	 the end of the loop. So keep its value to help the search for the
	 next entry in catalog `a'. */
      prevblow=blow;


      /* Go through catalog `b' (starting at `blow') with a first axis
	 value smaller than the maximum acceptable range for `si'. */
      for( bi=blow; bi<br && b[0][bi] <= a[0][ai] + dist[0]; ++bi )
	{
	  /* Only consider records with a second axis value in the
	     correct range, note that unlike the first axis, the
	     second axis is no longer sorted. so we have to do both
	     lower and higher limit checks for each item.

	     Positions can have an accuracy to a much higher order of
	     magnitude than the search radius. Therefore, it is
	     meaning-less to sort the second axis (after having sorted
	     the first). In other words, very rarely can two first
	     axis coordinates have EXACTLY the same floating point
	     value as each other to easily define an independent
	     sorting in the second axis. */
	  if( b[1][bi] >= a[1][ai]-dist[1] && b[1][bi] <= a[1][ai]+dist[1] )
	    {
	      /* Now, `bi' is within the rectangular range of `ai'. But
		 this is not enough to consider the two objects matched for
		 the following reasons:

		 1) Until now we have avoided calculations other than
		    larger or smaller on double precision floating point
		    variables for efficiency. So the `bi' is within a
		    square of side `dist[0]*dist[1]' around `ai' (not
		    within a fixed radius).

		 2) Other objects in the `b' catalog may be closer to `ai'
		    than this `bi'.

		 3) The closest `bi' to `ai' might be closer to another
		    catalog `a' record.

		 To address these problems, we will use a linked list to
		 keep the indexes of the `b's near `ai', along with their
		 distance. We only add the `bi's to this list that are
		 within the acceptable distance.

                 Since we are dealing with much fewer objects at this
		 stage, it is justified to do complex mathematical
		 operations like square root and multiplication. This fixes
		 the first problem.

		 The next two problems will be solved with the list after
		 parsing of the whole catalog is complete.*/
              r = ( iscircle
                    ? sqrt( (b[0][bi]-a[0][ai])*(b[0][bi]-a[0][ai])
                            + (b[1][bi]-a[1][ai])*(b[1][bi]-a[1][ai]) )
                    : match_coordinates_elliptical_r(b[0][bi]-a[0][ai],
                                                     b[1][bi]-a[1][ai],
                                                     aper, c, s));
	      if(r<aper[0])
		match_coordinate_add_to_sfll(&bina[ai], bi, r);
	    }
	}


      /* If there was no objects within the acceptable distance, then the
	 linked list pointer will be NULL, so go on to the next `ai'. */
      if(bina[ai]==NULL)
	continue;

      /* For checking the status of affairs uncomment this block
      {
	struct match_coordinate_sfll *tmp;
	printf("\n\nai: %lu:\n", ai);
	printf("ax: %f (%f -- %f)\n", a[0][ai], a[0][ai]-dist[0],
               a[0][ai]+dist[0]);
	printf("ay: %f (%f -- %f)\n", a[1][ai], a[1][ai]-dist[1],
               a[1][ai]+dist[1]);
	for(tmp=bina[ai];tmp!=NULL;tmp=tmp->next)
	  printf("%lu: %f\n", tmp->v, tmp->f);
      }
      */
    }
}





/* In `match_coordinates_second_in_first', we made an array of lists, here
   we want to reverse that list to fix the second two issues that were
   discussed there. */
void
match_coordinates_rearrange(gal_data_t *A, gal_data_t *B,
                            struct match_coordinate_sfll **bina)
{
  size_t bi;
  float *fp, *fpf, r, *ainb;
  size_t ai, ar=A->size, br=B->size;

  /* Allocate the space for `ainb' and initialize it to NaN (since zero is
     meaningful) in this context (both for indexs and also for
     floats). This is a two column array that will keep the distance and
     index of the closest element in catalog `a' for each element in
     catalog b. */
  errno=0; ainb=calloc(2*br, sizeof *ainb);
  if(ainb==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for `ainb'", __func__,
          br*sizeof *ainb);
  fpf=(fp=ainb)+2*br; do *fp++=NAN; while(fp<fpf);

  /* Go over each object in catalog `a' and re-distribute the near objects,
     to find which ones in catalog `a' are within the search radius of
     catalog b in a sorted manner. Note that we only need the `ai' with the
     minimum distance to `bi', the rest are junk.*/
  for( ai=0; ai<ar; ++ai )
    while( bina[ai] )	/* As long as its not NULL.            */
      {
	/* Pop out a `bi' and its distance to this `ai' from `bina'. */
	match_coordinate_pop_from_sfll(&bina[ai], &bi, &r);

	/* If nothing has been put here (the isnan condition below is
	   true), or something exists (the isnan is false, and so it
	   will check the second OR test) with a distance that is
	   larger than this distance then just put this value in. */
	if( isnan(ainb[bi*2]) || r<ainb[bi*2+1] )
	  {
	    ainb[bi*2  ] = ai;
	    ainb[bi*2+1] = r;
	  }
      }

  /* For checking the status of affairs uncomment this block
  {
    printf("\n\nFilled ainb:\n");
    for(bi=0;bi<br;++bi)
      if( !isnan(ainb[bi*2]) )
	printf("bi: %lu: %.0f, %f\n", bi, ainb[bi*2], ainb[bi*2+1]);
  }
  */

  /* Re-fill the bina array, but this time only with the `bi' that is
     closest to it. Note that bina was fully set to NULL after popping all
     the elements in the loop above.*/
  for( bi=0; bi<br; ++bi )
    if( !isnan(ainb[bi*2]) )
      {
	/* Just to keep the same terminology as before and easier
	   reading.*/
	r=ainb[bi*2+1];
	ai=(size_t)(ainb[bi*2]);

	/* Check if this is the first time we are associating a `bi' to
	   this `ai'. If so, then just allocate a single element
	   list. Otherwise, see if the distance is closer or not. If so,
	   replace the values in the single node. */
	if( bina[ai] )
	  {
	    /* If the distance of this record is smaller than the existing
	       entry, then replace the values. */
	    if( r < bina[ai]->f )
	      {
		bina[ai]->f=r;
		bina[ai]->v=bi;
	      }
	  }
	else
          match_coordinate_add_to_sfll(&bina[ai], bi, r);
      }

  /* For checking the status of affairs uncomment this block
  {
    size_t bi, counter=0;
    double *a[2]={A->array, A->next->array};
    double *b[2]={B->array, B->next->array};
    printf("Rearranged bina:\n");
    for(ai=0;ai<ar;++ai)
      if(bina[ai])
        {
          ++counter;
          bi=bina[ai]->v;
          printf("A_%lu (%.8f, %.8f) <--> B_%lu (%.8f, %.8f):\n\t%f\n",
                 ai, a[0][ai], a[1][ai], bi, b[0][bi], b[1][bi],
                 bina[ai]->f);
        }
    printf("\n-----------\nMatched: %zu\n", counter);
  }
  exit(0);
  */

  /* Clean up */
  free(ainb);
}




















/********************************************************************/
/*************            Coordinate matching           *************/
/********************************************************************/
/* Match two positions: the inputs (`coord1' and `coord2') should be lists
   of coordinates. To speed up the search, this function needs the input
   arrays to be sorted by their first column. If it is already sorted, you
   can set `sorted_by_first' to non-zero. When sorting is necessary and
   `inplace' is non-zero, the actual inputs will be sorted. Otherwise, an
   internal copy of the inputs will be made which will be used (sorted) and
   later freed. Therefore when `inplace==0', the input's won't be
   changed.

   The output is a list of `gal_data_t' with the following columns:

       Node 1: First catalog index (counting from zero).
       Node 2: Second catalog index (counting from zero).
       Node 3: Distance between the match.                    */
gal_data_t *
gal_match_coordinates(gal_data_t *coord1, gal_data_t *coord2,
                      gal_data_t *aperture, int sorted_by_first,
                      int inplace, size_t minmapsize)
{
  float r;
  double *rmatch;
  size_t *aind, *bind;
  gal_data_t *A, *B, *out;
  struct match_coordinate_sfll **bina;
  size_t ai, bi, counter=0, *A_perm=NULL, *B_perm=NULL;

  /* Do a small sanity check and make the preparations. After this point,
     we'll call the two arrays `a' and `b'.*/
  match_coordinaes_sanity_check(coord1, coord2, aperture);
  match_coordinates_prepare(coord1, coord2, sorted_by_first, inplace,
                            &A, &B, &A_perm, &B_perm, minmapsize);

  /* Allocate the `bina' array (an array of lists). Let's call the first
     catalog `a' and the second `b'. This array has `a->size' elements
     (pointers) and for each, it keeps a list of `b' elements that are
     nearest to it. */
  errno=0;
  bina=calloc(A->size, sizeof *bina);
  if(bina==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for `bina'", __func__,
          A->size*sizeof *bina);

  /* All records in `b' that match each `a' (possibly duplicate). */
  match_coordinates_second_in_first(A, B, aperture, bina);

  /* Two re-arrangings will fix the issue. */
  match_coordinates_rearrange(A, B, bina);

  /* Find how many matches there were in total */
  for(ai=0;ai<A->size;++ai) if(bina[ai]) ++counter;

  /* Allocate the output list. */
  out=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &counter, NULL, 0,
                     minmapsize, "CAT1_ROW", "counter",
                     "Row index in first catalog (counting from 0).");
  out->next=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &counter, NULL, 0,
                           minmapsize, "CAT2_ROW", "counter",
                           "Row index in second catalog (counting "
                           "from 0).");
  out->next->next=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &counter, NULL,
                                 0, minmapsize, "MATCH_DIST", NULL,
                                 "Distance between the match.");

  /* Fill in the output arrays. */
  counter=0;
  aind=out->array;
  bind=out->next->array;
  rmatch=out->next->next->array;
  for(ai=0;ai<A->size;++ai)
    if(bina[ai])
      {
        /* Note that the permutation keeps the original indexs. */
        match_coordinate_pop_from_sfll(&bina[ai], &bi, &r);
        aind[counter]=A_perm[ai];
        bind[counter]=B_perm[bi];
        rmatch[counter++]=r;
      }

  /* Clean up and return. */
  free(bina);
  free(A_perm);
  free(B_perm);
  if(A!=coord1)
    {
      gal_list_data_free(A);
      gal_list_data_free(B);
    }
  return out;
}
