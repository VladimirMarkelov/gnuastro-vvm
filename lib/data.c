/*********************************************************************
data -- Structure and functions to represent/work with data
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2016, Free Software Foundation, Inc.

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

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/mman.h>

#include <gnuastro/data.h>

#include <checkset.h>
#include <data-copy.h>
#include <data-arithmetic-other.h>
#include <data-arithmetic-binary.h>
#include <data-arithmetic-onlyint.h>







/*********************************************************************/
/*************          Size and allocation        *******************/
/*********************************************************************/
int
gal_data_dsize_is_different(gal_data_t *first, gal_data_t *second)
{
  size_t i;

  /* First make sure that the dimensionality is the same. */
  if(first->ndim!=second->ndim)
    return 1;

  /* Check if the sizes along all dimensions are the same: */
  for(i=0;i<first->ndim;++i)
    if( first->dsize[i] != second->dsize[i] )
      return 1;

  /* If it got to here, we know the dimensions have the same length. */
  return 0;
}





size_t
gal_data_sizeof(int type)
{
  /* Allocate space for the array to keep the image. */
  switch(type)
    {
    case GAL_DATA_TYPE_BIT:
      error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support bit "
            "types, please get in touch with us to implement it.");

      /* The parenthesis after sizeof is not a function, it is actually a
         type cast, so we have put a space between size of and the
         parenthesis to highlight this. In C, `sizeof' is an operator, not
         a function.*/
    case GAL_DATA_TYPE_UCHAR:
      return sizeof (unsigned char);

    case GAL_DATA_TYPE_LOGICAL: case GAL_DATA_TYPE_CHAR:
      return sizeof (char);

    case GAL_DATA_TYPE_STRING:
      return sizeof (char *);

    case GAL_DATA_TYPE_USHORT:
      return sizeof (unsigned short);

    case GAL_DATA_TYPE_SHORT:
      return sizeof (short);

    case GAL_DATA_TYPE_UINT:
      return sizeof (unsigned int);

    case GAL_DATA_TYPE_INT:
      return sizeof (int);

    case GAL_DATA_TYPE_ULONG:
      return sizeof (unsigned long);

    case GAL_DATA_TYPE_LONG:
      return sizeof (long);

    case GAL_DATA_TYPE_LONGLONG:
      return sizeof (LONGLONG);

    case GAL_DATA_TYPE_FLOAT:
      if( sizeof (float) != 4 )
        error(EXIT_FAILURE, 0, "`float` is not 32 bits on this machine");
      return sizeof (float);

    case GAL_DATA_TYPE_DOUBLE:
      if( sizeof (double) != 8 )
        error(EXIT_FAILURE, 0, "`double` is not 64 bits on this machine");
      return sizeof (double);

    case GAL_DATA_TYPE_COMPLEX:
      if( sizeof (float) != 4 )
        error(EXIT_FAILURE, 0, "`float` is not 32 bits on this machine");
      return sizeof (gsl_complex_float);

    case GAL_DATA_TYPE_DCOMPLEX:
      if( sizeof (double) != 8 )
        error(EXIT_FAILURE, 0, "`double` is not 64 bits on this machine");
      return sizeof (gsl_complex);

    default:
      error(EXIT_FAILURE, 0, "type value of %d not recognized in "
            "gal_data_sizeof", type);
    }

  error(EXIT_FAILURE, 0, "Control has reached the end of `gal_data_sizeof' "
        "This is a bug! Please contact us at %s so we can find the cause "
        "of the problem.", PACKAGE_BUGREPORT);
  return -1;
}





/* Copy the WCS structure from the input to the output structure. */
void
gal_data_copy_wcs(gal_data_t *in, gal_data_t *out)
{
  if(in->wcs)
    {
      errno=0;
      out->wcs=malloc(sizeof *out->wcs);
      if(out->wcs==NULL)
        error(EXIT_FAILURE, errno, "%zu bytes for out->wcs in "
              "gal_data_copy_wcs", sizeof *out->wcs);
      wcscopy(1, in->wcs, out->wcs);
    }
  else
    out->wcs=NULL;
}





/* Allocate an array based on the value of type. Note that the argument
   `size' is the number of elements, necessary in the array, the number of
   bytes each element needs will be determined internaly by this function
   using the datatype argument, so you don't have to worry about it. */
void *
gal_data_malloc_array(int type, size_t size)
{
  void *array;

  errno=0;
  array=malloc( size * gal_data_sizeof(type) );
  if(array==NULL)
    error(EXIT_FAILURE, errno, "array of %zu bytes in gal_data_malloc_array",
          size * gal_data_sizeof(type));

  return array;
}





void *
gal_data_calloc_array(int type, size_t size)
{
  void *array;

  errno=0;
  array=calloc( size,  gal_data_sizeof(type) );
  if(array==NULL)
    error(EXIT_FAILURE, errno, "array of %zu bytes in gal_data_calloc_array",
          size * gal_data_sizeof(type));

  return array;
}





/* Allocate space for one blank value of the given type and put the value
   in it. */
void *
gal_data_alloc_number(int type, void *number)
{
  void *allocated;

  /* Allocate the space for the blank value: */
  allocated=gal_data_malloc_array(type, 1);

  /* Put the blank value into it. */
  errno=0;
  switch(type)
    {
    case GAL_DATA_TYPE_BIT:
      error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support blank "
            "values for `GAL_DATA_TYPE_BIT', please get in touch with "
            "us to see how we can implement it.");

    case GAL_DATA_TYPE_UCHAR:
      *(unsigned char *)allocated=*(unsigned char *)number;
      break;

      /* CFITSIO says "int for keywords, char for table columns". Here we
         are only assuming table columns. So in practice this also applies
         to TSBYTE.*/
    case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
      *(char *)allocated=*(char *)number;
      break;

    case GAL_DATA_TYPE_STRING:
      *(unsigned char **)allocated=*(unsigned char **)number;
      break;

    case GAL_DATA_TYPE_USHORT:
      *(unsigned short *)allocated=*(unsigned short *)number;
      break;

    case GAL_DATA_TYPE_SHORT:
      *(short *)allocated=*(short *)number;
      break;

    case GAL_DATA_TYPE_UINT:
      *(unsigned int *)allocated=*(unsigned int *)number;
      break;

    case GAL_DATA_TYPE_INT:
      *(int *)allocated=*(int *)number;
      break;

    case GAL_DATA_TYPE_ULONG:
      *(unsigned long *)allocated=*(unsigned long *)number;
      break;

    case GAL_DATA_TYPE_LONG:
      *(long *)allocated=*(long *)number;
      break;

    case GAL_DATA_TYPE_LONGLONG:
      *(LONGLONG *)allocated=*(LONGLONG *)number;
      break;

    case GAL_DATA_TYPE_FLOAT:
      *(float *)allocated=*(float *)number;
      break;

    case GAL_DATA_TYPE_DOUBLE:
      *(double *)allocated=*(double *)number;
      break;

    case GAL_DATA_TYPE_COMPLEX:
      GSL_COMPLEX_P_REAL(((gsl_complex_float *)allocated)) =
        GSL_COMPLEX_P_REAL(((gsl_complex_float *)number));
      GSL_COMPLEX_P_IMAG(((gsl_complex_float *)allocated)) =
        GSL_COMPLEX_P_IMAG(((gsl_complex_float *)number));
      break;

    case GAL_DATA_TYPE_DCOMPLEX:
      GSL_COMPLEX_P_REAL(((gsl_complex *)allocated)) =
        GSL_COMPLEX_P_REAL(((gsl_complex *)number));
      GSL_COMPLEX_P_IMAG(((gsl_complex *)allocated)) =
        GSL_COMPLEX_P_IMAG(((gsl_complex *)number));
      break;

    default:
      error(EXIT_FAILURE, 0, "type value of %d not recognized in "
            "`gal_data_alloc_number'", type);
    }

  return allocated;
}





void
gal_data_mmap(gal_data_t *data)
{
  int filedes;
  char *filename;
  unsigned char uc=0;
  size_t bsize=data->size*gal_data_sizeof(data->type);

  /* Check if the .gnuastro folder exists, write the file there. If it
     doesn't exist, then make the .gnuastro directory.*/
  gal_checkset_mkdir(".gnuastro");

  /* Set the filename */
  gal_checkset_allocate_copy("./.gnuastro/mmap_XXXXXX", &filename);

  /* Create a zero-sized file and keep its descriptor.  */
  errno=0;
  /*filedes=open(filename, O_RDWR | O_CREAT | O_EXCL | O_TRUNC );*/
  filedes=mkstemp(filename);
  if(filedes==-1)
    error(EXIT_FAILURE, errno, "%s couldn't be created", filename);


  /* Make enough space to keep the array data. */
  errno=0;
  if( lseek(filedes, bsize, SEEK_SET) == -1 )
    error(EXIT_FAILURE, errno, "%s: unable to change file position by "
          "%zu bytes", filename, bsize);


  /* Write to the newly set file position so the space is allocated. */
  if( write(filedes, &uc, 1) == -1)
    error(EXIT_FAILURE, errno, "%s: unable to write one byte at the "
          "%zu-th position", filename, bsize);


  /* Map the memory. */
  data->array=mmap(NULL, bsize, PROT_READ | PROT_WRITE, MAP_SHARED,
                   filedes, 0);

  /* Close the file. */
  if( close(filedes) == -1 )
    error(EXIT_FAILURE, errno, "%s couldn't be closed", filename);

  /* Set the mmaped flag to 1 and keep the filename. */
  data->mmapname=filename;
}





/* Initialize the data structure.

   Some notes:

   - The `status' value is the only element that cannot be set by this
     function, it is initialized to zero.

   - If no `array' is given, a blank array of the given size will be
     allocated. If it is given the array pointer will be directly put here,
     so do not free it independently any more. If you want a separate copy
     of a dataset, you should use `gal_data_copy', not this function.

   - Space for the `name', `unit', and `comment' strings within the data
     structure are allocated here. So you can safely use literal strings,
     or statically allocated ones, or simply the strings from other data
     structures (and not have to worry about which one to free later).
*/
void
gal_data_initialize(gal_data_t *data, void *array, int type,
                    size_t ndim, long *dsize, struct wcsprm *wcs,
                    int clear, size_t minmapsize, char *name,
                    char *unit, char *comment)
{
  size_t i;
  gal_data_t in;

  /* Do the simple copying  cases. */
  data->status=0;
  data->next=NULL;
  data->ndim=ndim;
  data->type=type;
  data->minmapsize=minmapsize;
  gal_checkset_allocate_copy(unit, &data->unit);
  gal_checkset_allocate_copy(name, &data->name);
  gal_checkset_allocate_copy(comment, &data->comment);


  /* Copy the WCS structure. Note that the `in' data structure was just
     defined to keep this pointer to call `gal_data_copy_wcs'. */
  in.wcs=wcs;
  gal_data_copy_wcs(&in, data);


  /* Allocate space for the dsize array: */
  errno=0;
  data->dsize=malloc(ndim*sizeof *data->dsize);
  if(data->dsize==NULL)
    error(EXIT_FAILURE, errno, "%zu bytes for data->dsize in "
          "`gal_data_alloc'", ndim*sizeof *data->dsize);


  /* Fill in the `dsize' array and in the meantime set `size': */
  data->size=1;
  for(i=0;i<ndim;++i)
    {
      /* Do a small sanity check. */
      if(dsize[i]==0)
        error(EXIT_FAILURE, 0, "the size of a dimension cannot be zero. "
              "dsize[%zu] in `gal_data_alloc' has a value of 0", i);

      /* Write this dimension's size, also correct the total number of
         elements. */
      data->size *= ( data->dsize[i] = dsize[i] );
    }


  /* Allocate space for the array, clear it if necessary: */
  if(array)
    data->array=array;
  else
    {
      if( gal_data_sizeof(type)*data->size  > minmapsize )
        gal_data_mmap(data);
      else
        {
          /* Allocate the space for the array. */
          if(clear)
            data->array = gal_data_calloc_array(data->type, data->size);
          else
            data->array = gal_data_malloc_array(data->type, data->size);

          /* Set the values. */
          data->mmapname=NULL;
        }
    }
}





/* Allocate a data structure based on the given parameters. If you want to
   force the array into the hdd/ssd (mmap it), then set minmapsize=-1
   (largest possible size_t value), in this way, no file will be larger. */
gal_data_t *
gal_data_alloc(void *array, int type, size_t ndim, long *dsize,
               struct wcsprm *wcs, int clear, size_t minmapsize,
               char *title, char *unit, char *comment)
{
  gal_data_t *out;


  /* Allocate the space for the actual structure. */
  errno=0;
  out=malloc(sizeof *out);
  if(out==NULL)
    error(EXIT_FAILURE, errno, "%zu bytes for gal_data_t in gal_data_alloc",
          sizeof *out);


  /* Initialize the allocated array. */
  gal_data_initialize(out, array, type, ndim, dsize, wcs, clear, minmapsize,
                      title, unit, comment);


  /* Return the final structure. */
  return out;
}





/* Free the allocated contents of a data structure and possibly also the
   data structure its self. When `only_contents' is zero, the actual data
   structure will also be freed, see bellow.

   The reason for the `only_contents' argument is that the data structure
   might be allocated as an array (statically like `gal_data_t da[20]', or
   dynamically like `gal_data_t *da; da=malloc(20*sizeof *da);'). In both
   cases, a loop will be necessary to delete the allocated contents of each
   element of the data structure array, but not the structure its
   self. After that loop, if the array of data structures was statically
   allocated, you don't have to do anything. If it was dynamically
   allocated, we just have to run `free(da)'.*/
void
gal_data_free(gal_data_t *data, int only_contents)
{
  char **strarray=data->array;
  char *str=strarray[0], *strf=str+data->size;

  /* Free all the possible allocations. */
  free(data->dsize);
  if(data->name) free(data->name);
  if(data->unit) free(data->unit);
  if(data->wcs) wcsfree(data->wcs);
  if(data->comment) free(data->comment);

  /* If the data type is string, then each element in the array is actually
     a pointer to the array of characters, so free them before freeing the
     actual array. */
  if(data->type==GAL_DATA_TYPE_STRING)
    do if(str) free(str); while(++str<strf);

  /* Free the array. Note that if the array is mmap'd, then we need to
     delete the file that kept it. */
  if(data->mmapname)
    {
      /* Delete the file keeping the array. */
      remove(data->mmapname);

      /* Free the file name space. */
      free(data->mmapname);
    }
  else
    free(data->array);


  /* Finally, free the actual data structure. */
  if(only_contents==0)
    free(data);
}




















/*********************************************************************/
/*************    Data structure as a linked list   ******************/
/*********************************************************************/
/* Add a new data structure to the top of an existing linked list of data
   structures. Note that if the new node is its self a list, all its nodes
   will be added to the list. */
void
gal_data_add_to_ll(gal_data_t **list, gal_data_t *newnode)
{
  gal_data_t *tmp=newnode, *toadd;

  /* Check if newnode is itself a list or not. */
  if(newnode->next)
    {
      /* Go onto the last node in newnode's existing list. */
      while(tmp->next) tmp=tmp->next;

      /* Set the last node as the node to add to the list. */
      toadd=tmp;
    }
  else
    /* Its not a list, so just set it to `toadd'. */
    toadd=newnode;


  /* Set the next element of toadd and update what list points to.*/
  toadd->next=*list;
  *list=toadd;
}





gal_data_t *
gal_data_pop_from_ll(gal_data_t **list)
{
  struct gal_data_t *out;
  out=*list;
  *list=out->next;
  return out;
}





size_t
gal_data_num_in_ll(gal_data_t *list)
{
  size_t num=0;
  while(list!=NULL)
    {
      ++num;
      list=list->next;
    }
  return num;
}





gal_data_t **
gal_data_ll_to_array_of_ptrs(gal_data_t *list, size_t *num)
{
  size_t i=0;
  gal_data_t **out;

  /* Find the number of nodes in the list. */
  *num=gal_data_num_in_ll(list);

  /* Allocate space for the array. */
  errno=0;
  out=malloc(*num * sizeof *out);
  if(out==NULL)
    error(EXIT_FAILURE, 0, "%zu bytes for the output pointer in "
          "`gal_data_ll_to_array_of_ptrs'", *num * sizeof *out);

  /* Fill in the array with pointers to each data-structure. Note that we
     don't need the list pointer any more, so we can just increment it.*/
  while(list!=NULL) { out[i++]=list; list=list->next; }

  /* Return the allocated array. */
  return out;
}

















/*************************************************************
 **************          Blank data            ***************
 *************************************************************/
void *
gal_data_alloc_blank(int type)
{
  /* Define the pointers. */
  unsigned char     uc = GAL_DATA_BLANK_UCHAR;
  char               c = GAL_DATA_BLANK_CHAR;
  char            *str = GAL_DATA_BLANK_STRING;
  unsigned short    us = GAL_DATA_BLANK_USHORT;
  short              s = GAL_DATA_BLANK_SHORT;
  unsigned int      ui = GAL_DATA_BLANK_UINT;
  int                i = GAL_DATA_BLANK_INT;
  unsigned long     ul = GAL_DATA_BLANK_ULONG;
  long               l = GAL_DATA_BLANK_LONG;
  LONGLONG           L = GAL_DATA_BLANK_LONGLONG;
  float              f = GAL_DATA_BLANK_FLOAT;
  double             d = GAL_DATA_BLANK_DOUBLE;
  gsl_complex_float cx;
  gsl_complex      dcx;

  /* Put the blank value into it. */
  switch(type)
    {
    case GAL_DATA_TYPE_BIT:
      error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support blank "
            "values for `GAL_DATA_TYPE_BIT', please get in touch with "
            "us to see how we can implement it.");

    case GAL_DATA_TYPE_UCHAR:
      return gal_data_alloc_number(type, &uc);

      /* CFITSIO says "int for keywords, char for table columns". Here we
         are only assuming table columns. So in practice this also applies
         to TSBYTE.*/
    case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
      return gal_data_alloc_number(type, &c);

    case GAL_DATA_TYPE_STRING:
      return gal_data_alloc_number(type, &str);

    case GAL_DATA_TYPE_USHORT:
      return gal_data_alloc_number(type, &us);

    case GAL_DATA_TYPE_SHORT:
      return gal_data_alloc_number(type, &s);

    case GAL_DATA_TYPE_UINT:
      return gal_data_alloc_number(type, &ui);

    case GAL_DATA_TYPE_INT:
      return gal_data_alloc_number(type, &i);

    case GAL_DATA_TYPE_ULONG:
      return gal_data_alloc_number(type, &ul);

    case GAL_DATA_TYPE_LONG:
      return gal_data_alloc_number(type, &l);

    case GAL_DATA_TYPE_LONGLONG:
      return gal_data_alloc_number(type, &L);

    case GAL_DATA_TYPE_FLOAT:
      return gal_data_alloc_number(type, &f);

    case GAL_DATA_TYPE_DOUBLE:
      return gal_data_alloc_number(type, &d);

    case GAL_DATA_TYPE_COMPLEX:
      GSL_SET_COMPLEX(&cx, GAL_DATA_BLANK_FLOAT, GAL_DATA_BLANK_FLOAT);
      return gal_data_alloc_number(type, &cx);

    case GAL_DATA_TYPE_DCOMPLEX:
      GSL_SET_COMPLEX(&dcx, GAL_DATA_BLANK_DOUBLE, GAL_DATA_BLANK_DOUBLE);
      return gal_data_alloc_number(type, &dcx);

    default:
      error(EXIT_FAILURE, 0, "type value of %d not recognized in "
            "`gal_data_alloc_blank'", type);
    }

  return NULL;
}





/* Any non-zero pixel in the `mask' array is set as blank in the `in'
   array. */
void
gal_data_apply_mask(gal_data_t *in, gal_data_t *mask)
{
  int hasblank=0;
  float *mpt, *m, *mf;
  gal_data_t *converted;

  unsigned char     *uc   = in->array, *ucf   = uc  + in->size;
  char              *c    = in->array, *cf    = c   + in->size;
  char              **str = in->array, **strf = str + in->size;
  unsigned short    *us   = in->array, *usf   = us  + in->size;
  short             *s    = in->array, *sf    = s   + in->size;
  unsigned int      *ui   = in->array, *uif   = ui  + in->size;
  int               *ii   = in->array, *iif   = ii  + in->size;
  unsigned long     *ul   = in->array, *ulf   = ul  + in->size;
  long              *l    = in->array, *lf    = l   + in->size;
  LONGLONG          *L    = in->array, *Lf    = L   + in->size;
  float             *f    = in->array, *ff    = f   + in->size;
  double            *d    = in->array, *df    = d   + in->size;
  gsl_complex_float *cx   = in->array, *cxf   = cx  + in->size;
  gsl_complex       *dcx  = in->array, *dcxf  = dcx + in->size;


  /* Make sure the mask and input image have the same dimentionality: */
  if(in->ndim!=mask->ndim)
    error(EXIT_FAILURE, 0, "the `in' and `mask' data structures given "
          "to `gal_data_apply_mask' do not have the same dimensionality: "
          "%zu and %zu respectively", in->ndim, mask->ndim);


  /* Make sure the mask and input image have the same size along each
     dimension: */
  if(gal_data_dsize_is_different(in, mask))
    error(EXIT_FAILURE, 0, "the `in' and `mask' data structures given "
          "to `gal_data_apply_mask' do not have the same size along each "
          "dimension");


  /* First convert the mask to floating point. Note that although the
     standard definition of a mask is for it to be an integer, in some
     situations, the users might want to specify a floating point image as
     a mask. Such a mask might have values between 0 and 1 (for example
     they have made a mock profiles and want to mask all pixels covered by
     a profile). If we simply convert the mask image to an integer, all
     pixels with values between zero and one will be set to 0. So we need
     to internally convert the mask image to float to preserve this. */
  if(mask->type==GAL_DATA_TYPE_FLOAT)
    converted=mask;
  else
    converted=gal_data_copy_to_new_type(mask, GAL_DATA_TYPE_FLOAT);
  mpt=converted->array;


  /* Go over all the pixels and apply the mask. But first check if there
     actually are any blank values, it might be the case that there isn't
     any and in that case we can ignore the checks: */
  mf=(m=mpt)+in->size; do if(*m++ != 0.0f) hasblank=1; while(m<mf);
  if(hasblank)
    {
      switch(in->type)
        {
        case GAL_DATA_TYPE_BIT:
          error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support blank "
                "values for `GAL_DATA_TYPE_BIT', please get in touch with "
                "us to see how we can implement it.");

        case GAL_DATA_TYPE_UCHAR:
          do *uc = *mpt++==0.0f ? *uc : GAL_DATA_BLANK_UCHAR; while(++uc<ucf);
          break;

          /* CFITSIO says "int for keywords, char for table columns". Here we
             are only assuming table columns. So in practice this also applies
             to TSBYTE.*/
        case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
          do *c = *mpt++==0.0f ? *c : GAL_DATA_BLANK_CHAR; while(++c<cf);
          break;

        case GAL_DATA_TYPE_STRING:
          do *str = *mpt++==0.0f ? *str : GAL_DATA_BLANK_STRING;
          while(++str<strf);
          break;

        case GAL_DATA_TYPE_USHORT:
          do *us = *mpt++==0.0f ? *us : GAL_DATA_BLANK_USHORT;
          while(++us<usf);
          break;

        case GAL_DATA_TYPE_SHORT:
          do *s = *mpt++==0.0f ? *s : GAL_DATA_BLANK_SHORT; while(++s<sf);
          break;

        case GAL_DATA_TYPE_UINT:
          do *ui = *mpt++==0.0f ? *ui : GAL_DATA_BLANK_UINT; while(++ui<uif);
          break;

        case GAL_DATA_TYPE_INT:
          do *ii = *mpt++==0.0f ? *ii : GAL_DATA_BLANK_INT; while(++ii<iif);
          break;

        case GAL_DATA_TYPE_ULONG:
          do *ul = *mpt++==0.0f ? *ul : GAL_DATA_BLANK_ULONG; while(++ul<ulf);
          break;

        case GAL_DATA_TYPE_LONG:
          do *l = *mpt++==0.0f ? *l : GAL_DATA_BLANK_LONG; while(++l<lf);
          break;

        case GAL_DATA_TYPE_LONGLONG:
          do *L = *mpt++==0.0f ? *L : GAL_DATA_BLANK_LONGLONG; while(++L<Lf);
          break;

        case GAL_DATA_TYPE_FLOAT:
          do *f = *mpt++==0.0f ? *f : GAL_DATA_BLANK_FLOAT; while(++f<ff);
          break;

        case GAL_DATA_TYPE_DOUBLE:
          do *d = *mpt++==0.0f ? *d : GAL_DATA_BLANK_DOUBLE; while(++d<df);
          break;

        case GAL_DATA_TYPE_COMPLEX:
          do
            if(*mpt++ == 0.0f)
              GSL_SET_COMPLEX(cx, GAL_DATA_BLANK_FLOAT, GAL_DATA_BLANK_FLOAT);
          while(++cx<cxf);
          break;

        case GAL_DATA_TYPE_DCOMPLEX:
          do
            if(*mpt++==0.0f)
              GSL_SET_COMPLEX(dcx, GAL_DATA_BLANK_DOUBLE,
                              GAL_DATA_BLANK_DOUBLE);
          while(++dcx<dcxf);
          break;

        default:
          error(EXIT_FAILURE, 0, "type value of %d not recognized in "
                "`gal_data_alloc_blank'", in->type);
        }
    }

  /* Free the converted mask data if it was allocated: */
  if(converted!=mask)
    free(converted);
}





/* Change all blank values in `data' to the value pointed to by `value'. */
void
gal_data_blank_to_value(gal_data_t *data, void *value)
{
  /* 'value' will only be read from one of these based on the
     datatype. Which the caller assigned. If there is any problem, it is
     their responsability, not this function's.*/
  void *A=data->array;
  size_t S=data->size;
  unsigned char     *uc = A,   *ucf = A+S,   ucv = *(unsigned char *) value;
  char               *c = A,    *cf = A+S,    cv = *(char *) value;
  char            **str = A, **strf = A+S, *strv = *(char **) value;
  unsigned short    *us = A,   *usf = A+S,   usv = *(unsigned short *) value;
  short              *s = A,    *sf = A+S,    sv = *(short *) value;
  unsigned int      *ui = A,   *uif = A+S,   uiv = *(unsigned int *) value;
  int               *in = A,   *inf = A+S,   inv = *(int *) value;
  unsigned long     *ul = A,   *ulf = A+S,   ulv = *(unsigned long *) value;
  long               *l = A,    *lf = A+S,    lv = *(int32_t *) value;
  LONGLONG           *L = A,    *Lf = A+S,    Lv = *(int64_t *) value;
  float              *f = A,    *ff = A+S,    fv = *(float *) value;
  double             *d = A,    *df = A+S,    dv = *(double *) value;
  gsl_complex_float *cx = A,   *cxf = A+S,   cxv = *(gsl_complex_float *) value;
  gsl_complex      *dcx = A,  *dcxf = A+S,  dcxv = *(gsl_complex *) value;

  switch(data->type)
    {
    case GAL_DATA_TYPE_BIT:
      error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support bit "
            "datatype, please get in touch with us to implement it.");

    case GAL_DATA_TYPE_UCHAR:
      do if(*uc==GAL_DATA_BLANK_UCHAR) *uc++=ucv; while(uc<ucf);
      break;


    case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
      do if(*c==GAL_DATA_BLANK_CHAR) *c++=cv; while(c<cf);
      break;


    case GAL_DATA_TYPE_STRING:
      do if(*str==GAL_DATA_BLANK_STRING) *str++=strv; while(str<strf);
      break;


    case GAL_DATA_TYPE_USHORT:
      do if(*us==GAL_DATA_BLANK_USHORT) *us++=usv; while(us<usf);
      break;


    case GAL_DATA_TYPE_SHORT:
      do if(*s==GAL_DATA_BLANK_SHORT) *s++=sv; while(s<sf);
      break;


    case GAL_DATA_TYPE_UINT:
      do if(*ui==GAL_DATA_BLANK_UINT) *ui++=uiv; while(ui<uif);
      break;


    case GAL_DATA_TYPE_INT:
      do if(*in==GAL_DATA_BLANK_INT) *in++=inv; while(in<inf);
      break;


    case GAL_DATA_TYPE_ULONG:
      do if(*ul==GAL_DATA_BLANK_ULONG) *ul++=ulv; while(ul<ulf);
      break;


    case GAL_DATA_TYPE_LONG:
      do if(*l==GAL_DATA_BLANK_LONG) *l++=lv; while(l<lf);
      break;


    case GAL_DATA_TYPE_LONGLONG:
      do if(*L==GAL_DATA_BLANK_LONGLONG) *L++=Lv; while(L<Lf);
      break;


      /* Note that a NaN value is not equal to another NaN value, so we
         can't use the easy check for cases were the blank value is
         NaN. Also note that `isnan' is actually a macro, so it works for
         both float and double types.*/
    case GAL_DATA_TYPE_FLOAT:
      if(isnan(GAL_DATA_BLANK_FLOAT))
        do if(isnan(*f)) *f++=fv; while(f<ff);
      else
        do if(*f==GAL_DATA_BLANK_FLOAT) *f++=fv; while(f<ff);
      break;


    case GAL_DATA_TYPE_DOUBLE:
      if(isnan(GAL_DATA_BLANK_DOUBLE))
        do if(isnan(*d)) *d++=dv; while(d<df);
      else
        do if(*d==GAL_DATA_BLANK_FLOAT) *d++=dv; while(d<df);
      break;


    case GAL_DATA_TYPE_COMPLEX:
      if(isnan(GAL_DATA_BLANK_FLOAT))
          do
            if(isnan(GSL_COMPLEX_P_REAL(cx))
               && isnan(GSL_COMPLEX_P_IMAG(cx)) )
              GSL_SET_COMPLEX(cx, GSL_COMPLEX_P_REAL(&cxv),
                              GSL_COMPLEX_P_IMAG(&cxv));
          while(++cx<cxf);
      else
        do
          if( GSL_COMPLEX_P_REAL(cx) == GAL_DATA_BLANK_FLOAT
              && GSL_COMPLEX_P_IMAG(cx) == GAL_DATA_BLANK_FLOAT)
            GSL_SET_COMPLEX(cx, GSL_COMPLEX_P_REAL(&cxv),
                            GSL_COMPLEX_P_IMAG(&cxv));
        while(++cx<cxf);
      break;


    case GAL_DATA_TYPE_DCOMPLEX:
      if(isnan(GAL_DATA_BLANK_DOUBLE))
          do
            if(isnan(GSL_COMPLEX_P_REAL(dcx))
               && isnan(GSL_COMPLEX_P_IMAG(dcx)) )
              GSL_SET_COMPLEX(dcx, GSL_COMPLEX_P_REAL(&dcxv),
                              GSL_COMPLEX_P_IMAG(&dcxv));
          while(++dcx<dcxf);
      else
        do
          if( GSL_COMPLEX_P_REAL(dcx) == GAL_DATA_BLANK_FLOAT
              && GSL_COMPLEX_P_IMAG(dcx) == GAL_DATA_BLANK_FLOAT)
            GSL_SET_COMPLEX(dcx, GSL_COMPLEX_P_REAL(&dcxv),
                            GSL_COMPLEX_P_IMAG(&dcxv));
        while(++dcx<dcxf);
      break;


    default:
      error(EXIT_FAILURE, 0, "a bug! type value (%d) not recognized "
            "in `gal_data_blank_to_value'", data->type);
    }
}




/* Return 1 if the dataset has a blank value and zero if it doesn't. */
int
gal_data_has_blank(gal_data_t *data)
{
  /* 'value' will only be read from one of these based on the
     datatype. Which the caller assigned. If there is any problem, it is
     their responsability, not this function's.*/
  unsigned char     *uc = data->array,   *ucf = data->array + data->size;
  char               *c = data->array,    *cf = data->array + data->size;
  char            **str = data->array, **strf = data->array + data->size;
  unsigned short    *us = data->array,   *usf = data->array + data->size;
  short              *s = data->array,    *sf = data->array + data->size;
  unsigned int      *ui = data->array,   *uif = data->array + data->size;
  int               *in = data->array,   *inf = data->array + data->size;
  unsigned long     *ul = data->array,   *ulf = data->array + data->size;
  long               *l = data->array,    *lf = data->array + data->size;
  LONGLONG           *L = data->array,    *Lf = data->array + data->size;
  float              *f = data->array,    *ff = data->array + data->size;
  double             *d = data->array,    *df = data->array + data->size;
  gsl_complex_float *cx = data->array,   *cxf = data->array + data->size;
  gsl_complex      *dcx = data->array,  *dcxf = data->array + data->size;


  /* Go over the pixels and check: */
  switch(data->type)
    {
    case GAL_DATA_TYPE_BIT:
      error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support bit "
            "datatype, please get in touch with us to implement it.");

    case GAL_DATA_TYPE_UCHAR:
      do if(*uc++==GAL_DATA_BLANK_UCHAR) return 1; while(uc<ucf);
      break;


    case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
      do if(*c++==GAL_DATA_BLANK_CHAR) return 1; while(c<cf);
      break;


    case GAL_DATA_TYPE_STRING:
      do if(*str++==GAL_DATA_BLANK_STRING) return 1; while(str<strf);
      break;


    case GAL_DATA_TYPE_USHORT:
      do if(*us++==GAL_DATA_BLANK_USHORT) return 1; while(us<usf);
      break;


    case GAL_DATA_TYPE_SHORT:
      do if(*s++==GAL_DATA_BLANK_SHORT) return 1; while(s<sf);
      break;


    case GAL_DATA_TYPE_UINT:
      do if(*ui++==GAL_DATA_BLANK_UINT) return 1; while(ui<uif);
      break;


    case GAL_DATA_TYPE_INT:
      do if(*in++==GAL_DATA_BLANK_INT) return 1; while(in<inf);
      break;


    case GAL_DATA_TYPE_ULONG:
      do if(*ul++==GAL_DATA_BLANK_ULONG) return 1; while(ul<ulf);
      break;


    case GAL_DATA_TYPE_LONG:
      do if(*l++==GAL_DATA_BLANK_LONG) return 1; while(l<lf);
      break;


    case GAL_DATA_TYPE_LONGLONG:
      do if(*L++==GAL_DATA_BLANK_LONGLONG) return 1; while(L<Lf);
      break;


      /* Note that a NaN value is not equal to another NaN value, so we
         can't use the easy check for cases were the blank value is
         NaN. Also note that `isnan' is actually a macro, so it works for
         both float and double types.*/
    case GAL_DATA_TYPE_FLOAT:
      if(isnan(GAL_DATA_BLANK_FLOAT))
        do if(isnan(*f++)) return 1; while(f<ff);
      else
        do if(*f++==GAL_DATA_BLANK_FLOAT) return 1; while(f<ff);
      break;


    case GAL_DATA_TYPE_DOUBLE:
      if(isnan(GAL_DATA_BLANK_DOUBLE))
        do if(isnan(*d++)) return 1; while(d<df);
      else
        do if(*d++==GAL_DATA_BLANK_FLOAT) return 1; while(d<df);
      break;


    case GAL_DATA_TYPE_COMPLEX:
      if(isnan(GAL_DATA_BLANK_FLOAT))
          do
            if(isnan(GSL_COMPLEX_P_REAL(cx))
               && isnan(GSL_COMPLEX_P_IMAG(cx)) )
              return 1;
          while(++cx<cxf);
      else
        do
          if( GSL_COMPLEX_P_REAL(cx) == GAL_DATA_BLANK_FLOAT
              && GSL_COMPLEX_P_IMAG(cx) == GAL_DATA_BLANK_FLOAT)
            return 1;
        while(++cx<cxf);
      break;


    case GAL_DATA_TYPE_DCOMPLEX:
      if(isnan(GAL_DATA_BLANK_DOUBLE))
          do
            if(isnan(GSL_COMPLEX_P_REAL(dcx))
               && isnan(GSL_COMPLEX_P_IMAG(dcx)) )
              return 1;
          while(++dcx<dcxf);
      else
        do
          if( GSL_COMPLEX_P_REAL(dcx) == GAL_DATA_BLANK_FLOAT
              && GSL_COMPLEX_P_IMAG(dcx) == GAL_DATA_BLANK_FLOAT)
            return 1;
        while(++dcx<dcxf);
      break;


    default:
      error(EXIT_FAILURE, 0, "a bug! type value (%d) not recognized "
            "in `gal_data_blank_to_value'", data->type);
    }

  /* If there was a blank value, then the function would have returned with
     a value of 1. So if it reaches here, then we can be sure that there
     was no blank values, hence, return 0. */
  return 0;
}





/* Output a data-set of the the same size as the input, but with an
   unsigned character type that has a value of 1 for data that are blank
   and 0 for those that aren't. */
gal_data_t *
gal_data_flag_blank(gal_data_t *data)
{
  gal_data_t *out;

  /* 'value' will only be read from one of these based on the
     datatype. Which the caller assigned. If there is any problem, it is
     their responsability, not this function's.*/
  void *A=data->array;
  size_t S=data->size;
  unsigned char     *uc = A,   *ucf = A+S, *o;
  char               *c = A,    *cf = A+S;
  char            **str = A, **strf = A+S;
  unsigned short    *us = A,   *usf = A+S;
  short              *s = A,    *sf = A+S;
  unsigned int      *ui = A,   *uif = A+S;
  int               *in = A,   *inf = A+S;
  unsigned long     *ul = A,   *ulf = A+S;
  long               *l = A,    *lf = A+S;
  LONGLONG           *L = A,    *Lf = A+S;
  float              *f = A,    *ff = A+S;
  double             *d = A,    *df = A+S;
  gsl_complex_float *cx = A,   *cxf = A+S;
  gsl_complex      *dcx = A,  *dcxf = A+S;


  /* Allocate the output array. */
  out=gal_data_alloc(NULL, GAL_DATA_TYPE_UCHAR, data->ndim, data->dsize,
                     data->wcs, 0, data->minmapsize, data->name, data->unit,
                     data->comment);
  o=out->array;


  /* Go over the pixels and set the output values. */
  switch(data->type)
    {
    case GAL_DATA_TYPE_BIT:
      error(EXIT_FAILURE, 0, "Currently Gnuastro doesn't support bit "
            "datatype, please get in touch with us to implement it.");

    case GAL_DATA_TYPE_UCHAR:
      do *o++ = *uc==GAL_DATA_BLANK_UCHAR; while(++uc<ucf);
      break;


    case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
      do *o++ = *c==GAL_DATA_BLANK_CHAR; while(++c<cf);
      break;


    case GAL_DATA_TYPE_STRING:
      do *o++ = *str==GAL_DATA_BLANK_STRING; while(++str<strf);
      break;


    case GAL_DATA_TYPE_USHORT:
      do *o++ = *us==GAL_DATA_BLANK_USHORT; while(++us<usf);
      break;


    case GAL_DATA_TYPE_SHORT:
      do *o++ = *s==GAL_DATA_BLANK_SHORT; while(++s<sf);
      break;


    case GAL_DATA_TYPE_UINT:
      do *o++ = *ui==GAL_DATA_BLANK_UINT; while(++ui<uif);
      break;


    case GAL_DATA_TYPE_INT:
      do *o++ = *in==GAL_DATA_BLANK_INT; while(++in<inf);
      break;


    case GAL_DATA_TYPE_ULONG:
      do *o++ = *ul==GAL_DATA_BLANK_ULONG; while(++ul<ulf);
      break;


    case GAL_DATA_TYPE_LONG:
      do *o++ = *l==GAL_DATA_BLANK_LONG; while(++l<lf);
      break;


    case GAL_DATA_TYPE_LONGLONG:
      do *o++ = *L==GAL_DATA_BLANK_LONGLONG; while(++L<Lf);
      break;


      /* Note that a NaN value is not equal to another NaN value, so we
         can't use the easy check for cases were the blank value is
         NaN. Also note that `isnan' is actually a macro, so it works for
         both float and double types.*/
    case GAL_DATA_TYPE_FLOAT:
      if(isnan(GAL_DATA_BLANK_FLOAT))
        do *o++ = isnan(*f); while(++f<ff);
      else
        do *o++ = *f==GAL_DATA_BLANK_FLOAT; while(++f<ff);
      break;


    case GAL_DATA_TYPE_DOUBLE:
      if(isnan(GAL_DATA_BLANK_DOUBLE))
        do *o++ = isnan(*d); while(++d<df);
      else
        do *o++ = *d==GAL_DATA_BLANK_DOUBLE; while(++d<df);
      break;


    case GAL_DATA_TYPE_COMPLEX:
      if(isnan(GAL_DATA_BLANK_FLOAT))
          do
            *o++ = ( isnan(GSL_COMPLEX_P_REAL(cx))
                     && isnan(GSL_COMPLEX_P_IMAG(cx)) );
          while(++cx<cxf);
      else
        do
          *o++ = ( GSL_COMPLEX_P_REAL(cx) == GAL_DATA_BLANK_FLOAT
                   && GSL_COMPLEX_P_IMAG(cx) == GAL_DATA_BLANK_FLOAT );
        while(++cx<cxf);
      break;


    case GAL_DATA_TYPE_DCOMPLEX:
      if(isnan(GAL_DATA_BLANK_DOUBLE))
          do
            *o++ = ( isnan(GSL_COMPLEX_P_REAL(dcx))
                     && isnan(GSL_COMPLEX_P_IMAG(dcx)) );
          while(++dcx<dcxf);
      else
        do
          *o++ = ( GSL_COMPLEX_P_REAL(dcx) == GAL_DATA_BLANK_FLOAT
                   && GSL_COMPLEX_P_IMAG(dcx) == GAL_DATA_BLANK_FLOAT );
        while(++dcx<dcxf);
      break;


    default:
      error(EXIT_FAILURE, 0, "type value (%d) not recognized "
            "in `gal_data_flag_blank'", data->type);
    }

  /* Return */
  return out;
}




















/*************************************************************
 **************       Types and copying       ***************
 *************************************************************/
char *
gal_data_type_string(int type)
{
  switch(type)
    {
    case GAL_DATA_TYPE_BIT:
      return "bit";

    case GAL_DATA_TYPE_UCHAR:
      return "unsigned char";

      /* CFITSIO says "int for keywords, char for table columns". Here we
         are only assuming table columns. So in practice this also applies
         to TSBYTE.*/
    case GAL_DATA_TYPE_CHAR: case GAL_DATA_TYPE_LOGICAL:
      return "char";

    case GAL_DATA_TYPE_STRING:
      return "string";

    case GAL_DATA_TYPE_USHORT:
      return "unsigned short";

    case GAL_DATA_TYPE_SHORT:
      return "short";

    case GAL_DATA_TYPE_UINT:
      return "unsigned int";

    case GAL_DATA_TYPE_INT:
      return "int";

    case GAL_DATA_TYPE_ULONG:
      return "unsigned long";

    case GAL_DATA_TYPE_LONG:
      return "long";

    case GAL_DATA_TYPE_LONGLONG:
      return "LONGLONG";

    case GAL_DATA_TYPE_FLOAT:
      return "float";

    case GAL_DATA_TYPE_DOUBLE:
      return "double";

    case GAL_DATA_TYPE_COMPLEX:
      return "complex float";

    case GAL_DATA_TYPE_DCOMPLEX:
      return "complex double";

    default:
      error(EXIT_FAILURE, 0, "type value of %d not recognized in "
            "`gal_data_type_string'", type);
    }

  /* Any of the cases above should return this function, so if control
     reaches here, there is a bug. */
  error(EXIT_FAILURE, 0, "a bug! Please contact us at %s so we can address "
        "the problem. For some reason control has reached the end of "
        "the `gal_data_type_string' function. This must not happen",
        PACKAGE_BUGREPORT);
  return NULL;
}





gal_data_t *
gal_data_copy(gal_data_t *in)
{
  return gal_data_copy_to_new_type(in, in->type);
}





int
gal_data_out_type(gal_data_t *first, gal_data_t *second)
{
  return first->type > second->type ? first->type : second->type;
}





/* The two input `f' and `s' datasets can be any type. But `of' and `os'
   will have type `type', if freeinputs is non-zero, then the input arrays
   will be freed if they needed to be changed to a new type. */
void
gal_data_to_same_type(gal_data_t *f,   gal_data_t *s,
                      gal_data_t **of, gal_data_t **os,
                      int type, int freeinputs)
{
  /* Change first dataset into the new type if necessary. */
  if( f->type != type )
    {
      *of=gal_data_copy_to_new_type(f, type);
      if(freeinputs)
        gal_data_free(f, 0);
    }
  else
    *of=f;

  /* Change second dataset into the new type if necessary. */
  if( s->type != type )
    {
      *os=gal_data_copy_to_new_type(s, type);
      if(freeinputs)
        gal_data_free(s, 0);
    }
  else
    *os=s;
}




















/*************************************************************
 **************              Read              ***************
 *************************************************************/
/* If the data structure was correctly created (the string was a number),
   then return its pointer. Otherwise, return NULL. */
gal_data_t *
gal_data_string_to_number(char *string)
{
  long dsize[1]={1};
  int fnz=-1, lnz=0;     /* `F'irst (or `L'ast) `N'on-`Z'ero. */
  void *ptr, *numarr;
  char *tailptr, *cp;
  int type, forcedfloat=0;

  /* Define the pointers. */
  unsigned char     uc;
  char               c;
  unsigned short    us;
  short              s;
  unsigned int      ui;
  int                i;
  unsigned long     ul;
  long               l;
  LONGLONG           L;
  float              f;
  double             d;

  /* First see if the number is a double. */
  d=strtod(string, &tailptr);
  if(*tailptr=='f') { if(tailptr[1]=='\0') forcedfloat=1; else return NULL; }
  else if (*tailptr!='\0')  return NULL;

  /* See if the number is actually an integer: */
  if( forcedfloat==0 && ceil(d) == d )
    {
      /* If the number is negative, put it in the signed types (based on
         its value). If its zero or positive, then put it in the unsigned
         types. */
      if( d < 0 )
        {
          if     (d>CHAR_MIN)   {c=d; ptr=&c; type=GAL_DATA_TYPE_CHAR;}
          else if(d>SHRT_MIN)   {s=d; ptr=&s; type=GAL_DATA_TYPE_SHORT;}
          else if(d>INT_MIN)    {i=d; ptr=&i; type=GAL_DATA_TYPE_INT;}
          else if(d>LONG_MIN)   {l=d; ptr=&l; type=GAL_DATA_TYPE_LONG;}
          else                  {L=d; ptr=&L; type=GAL_DATA_TYPE_LONGLONG;}
        }
      else
        {
          if     (d<=UCHAR_MAX) {uc=d; ptr=&uc; type=GAL_DATA_TYPE_UCHAR;}
          else if(d<=USHRT_MAX) {us=d; ptr=&us; type=GAL_DATA_TYPE_USHORT;}
          else if(d<=UINT_MAX)  {ui=d; ptr=&ui; type=GAL_DATA_TYPE_UINT;}
          else if(d<=ULONG_MAX) {ul=d; ptr=&ul; type=GAL_DATA_TYPE_ULONG;}
          else                  {L=d;  ptr=&L;  type=GAL_DATA_TYPE_LONGLONG;}
        }
    }
  else
    {
      /* The maximum number of decimal digits to store in float or double
         precision floating point are:

         float:  23 mantissa bits + 1 hidden bit: log(224)÷log(10) = 7.22
         double: 52 mantissa bits + 1 hidden bit: log(253)÷log(10) = 15.95

         FLT_DIG (at least 6 in ISO C) keeps the number of digits (not zero
         before or after) that can be represented by a single precision
         floating point number. If there are more digits, then we should
         store the value as a double precision.

         Note that the number can have non-digit characters that we don't
         want, like: `.', `e', `E', `,'. */
      for(cp=string;*cp!='\0';++cp)
        if(isdigit(*cp) && *cp!='0' && fnz==-1)
          fnz=cp-string;

      /* In the previous loop, we went to the end of the string, so `cp'
         now points to its `\0'. We just have to iterate backwards! */
      for(;cp!=string;--cp)
        if(isdigit(*cp) && *cp!='0')
          {
            lnz=cp-string;
            break;
          }

      /* Calculate the number of decimal digits and decide if it the number
         should be a float or a double. */
      if( lnz-fnz < FLT_DIG || ( d<FLT_MAX && d>FLT_MIN ) )
        { f=d; ptr=&f; type=GAL_DATA_TYPE_FLOAT; }
      else
        {      ptr=&d; type=GAL_DATA_TYPE_DOUBLE; }
    }

  /* Return the pointer to the data structure. */
  numarr=gal_data_alloc_number(type, ptr);
  return gal_data_alloc(numarr, type, 1, dsize, NULL, 0, -1,
                        NULL, NULL, NULL);
}



















/*************************************************************
 **************    Type minimum and maximums   ***************
 *************************************************************/
/* Put the minimum (or maximum for the `gal_data_type_max') value for the
   type in the space (that must already be allocated before the call to
   this function) pointed to by in.  */
void
gal_data_type_min(int type, void *in)
{
  switch(type)
    {
    case GAL_DATA_TYPE_UCHAR:    *(unsigned char *)in  = 0;            break;
    case GAL_DATA_TYPE_CHAR:     *(char *)in           = CHAR_MIN;     break;
    case GAL_DATA_TYPE_USHORT:   *(unsigned short *)in = 0;            break;
    case GAL_DATA_TYPE_SHORT:    *(short *)in          = SHRT_MIN;     break;
    case GAL_DATA_TYPE_UINT:     *(unsigned int *)in   = 0;            break;
    case GAL_DATA_TYPE_INT:      *(int *)in            = INT_MIN;      break;
    case GAL_DATA_TYPE_ULONG:    *(unsigned long *)in  = 0;            break;
    case GAL_DATA_TYPE_LONG:     *(long *)in           = LONG_MIN;     break;
    case GAL_DATA_TYPE_LONGLONG: *(LONGLONG *)in       = LONGLONG_MIN; break;
    case GAL_DATA_TYPE_FLOAT:    *(float *)in          = -FLT_MAX;     break;
    case GAL_DATA_TYPE_DOUBLE:   *(double *)in         = -DBL_MAX;     break;
    default:
      error(EXIT_FAILURE, 0, "type code %d not recognized in "
            "`gal_data_type_min'", type);
    }
}





void
gal_data_type_max(int type, void *in)
{
  switch(type)
    {
    case GAL_DATA_TYPE_UCHAR:    *(unsigned char *)in  = UCHAR_MAX;    break;
    case GAL_DATA_TYPE_CHAR:     *(char *)in           = CHAR_MAX;     break;
    case GAL_DATA_TYPE_USHORT:   *(unsigned short *)in = USHRT_MAX;    break;
    case GAL_DATA_TYPE_SHORT:    *(short *)in          = SHRT_MAX;     break;
    case GAL_DATA_TYPE_UINT:     *(unsigned int *)in   = UINT_MAX;     break;
    case GAL_DATA_TYPE_INT:      *(int *)in            = INT_MAX;      break;
    case GAL_DATA_TYPE_ULONG:    *(unsigned long *)in  = ULONG_MAX;    break;
    case GAL_DATA_TYPE_LONG:     *(long *)in           = LONG_MAX;     break;
    case GAL_DATA_TYPE_LONGLONG: *(LONGLONG *)in       = LONGLONG_MAX; break;
    case GAL_DATA_TYPE_FLOAT:    *(float *)in          = FLT_MAX;      break;
    case GAL_DATA_TYPE_DOUBLE:   *(double *)in         = DBL_MAX;      break;
    default:
      error(EXIT_FAILURE, 0, "type code %d not recognized in "
            "`gal_data_type_min'", type);
    }
}




















/*************************************************************
 **************      Arithmetic operations     ***************
 *************************************************************/
char *
gal_data_operator_string(int operator)
{
  switch(operator)
    {
    case GAL_DATA_OPERATOR_PLUS:         return "+";
    case GAL_DATA_OPERATOR_MINUS:        return "-";
    case GAL_DATA_OPERATOR_MULTIPLY:     return "*";
    case GAL_DATA_OPERATOR_DIVIDE:       return "/";
    case GAL_DATA_OPERATOR_MODULO:       return "%";

    case GAL_DATA_OPERATOR_LT:           return "<";
    case GAL_DATA_OPERATOR_LE:           return "<=";
    case GAL_DATA_OPERATOR_GT:           return ">";
    case GAL_DATA_OPERATOR_GE:           return ">=";
    case GAL_DATA_OPERATOR_EQ:           return "==";
    case GAL_DATA_OPERATOR_NE:           return "!=";
    case GAL_DATA_OPERATOR_AND:          return "and";
    case GAL_DATA_OPERATOR_OR:           return "or";
    case GAL_DATA_OPERATOR_NOT:          return "not";
    case GAL_DATA_OPERATOR_ISBLANK:      return "isblank";
    case GAL_DATA_OPERATOR_WHERE:        return "where";

    case GAL_DATA_OPERATOR_BITAND:       return "bitand";
    case GAL_DATA_OPERATOR_BITOR:        return "bitor";
    case GAL_DATA_OPERATOR_BITXOR:       return "bitxor";
    case GAL_DATA_OPERATOR_BITLSH:       return "lshift";
    case GAL_DATA_OPERATOR_BITRSH:       return "rshift";
    case GAL_DATA_OPERATOR_BITNOT:       return "bitnot";

    case GAL_DATA_OPERATOR_ABS:          return "abs";
    case GAL_DATA_OPERATOR_POW:          return "pow";
    case GAL_DATA_OPERATOR_SQRT:         return "sqrt";
    case GAL_DATA_OPERATOR_LOG:          return "log";
    case GAL_DATA_OPERATOR_LOG10:        return "log10";

    case GAL_DATA_OPERATOR_MINVAL:       return "minvalue";
    case GAL_DATA_OPERATOR_MAXVAL:       return "maxvalue";
    case GAL_DATA_OPERATOR_MIN:          return "min";
    case GAL_DATA_OPERATOR_MAX:          return "max";
    case GAL_DATA_OPERATOR_AVERAGE:      return "average";
    case GAL_DATA_OPERATOR_MEDIAN:       return "median";

    case GAL_DATA_OPERATOR_TO_UCHAR:     return "uchar";
    case GAL_DATA_OPERATOR_TO_CHAR:      return "char";
    case GAL_DATA_OPERATOR_TO_USHORT:    return "ushort";
    case GAL_DATA_OPERATOR_TO_SHORT:     return "short";
    case GAL_DATA_OPERATOR_TO_UINT:      return "uint";
    case GAL_DATA_OPERATOR_TO_INT:       return "int";
    case GAL_DATA_OPERATOR_TO_ULONG:     return "ulong";
    case GAL_DATA_OPERATOR_TO_LONG:      return "long";
    case GAL_DATA_OPERATOR_TO_LONGLONG:  return "longlong";
    case GAL_DATA_OPERATOR_TO_FLOAT:     return "float";
    case GAL_DATA_OPERATOR_TO_DOUBLE:    return "double";

    default:
      error(EXIT_FAILURE, 0, "Operator code %d not recognized in "
            "gal_data_operator_to_string", operator);
    }

  error(EXIT_FAILURE, 0, "A bug! Please contact us to fix the problem. "
        "for some reason, control of the `gal_data_operator_to_string' "
        "function has reached its end! This should not have happened");
  return NULL;
}





static int
data_arithmetic_nearest_compiled_type(int intype)
{
  switch(intype)
    {
    case GAL_DATA_TYPE_UCHAR:
      if(GAL_CONFIG_BIN_OP_UCHAR) return GAL_DATA_TYPE_UCHAR;
      else
        {
          if     (GAL_CONFIG_BIN_OP_USHORT)   return GAL_DATA_TYPE_USHORT;
          else if(GAL_CONFIG_BIN_OP_SHORT)    return GAL_DATA_TYPE_SHORT;
          else if(GAL_CONFIG_BIN_OP_UINT)     return GAL_DATA_TYPE_UINT;
          else if(GAL_CONFIG_BIN_OP_INT)      return GAL_DATA_TYPE_INT;
          else if(GAL_CONFIG_BIN_OP_ULONG)    return GAL_DATA_TYPE_ULONG;
          else if(GAL_CONFIG_BIN_OP_LONG)     return GAL_DATA_TYPE_LONG;
          else if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_CHAR:
      if(GAL_CONFIG_BIN_OP_CHAR) return GAL_DATA_TYPE_CHAR;
      else
        {
          if     (GAL_CONFIG_BIN_OP_SHORT)    return GAL_DATA_TYPE_SHORT;
          else if(GAL_CONFIG_BIN_OP_INT)      return GAL_DATA_TYPE_INT;
          else if(GAL_CONFIG_BIN_OP_LONG)     return GAL_DATA_TYPE_LONG;
          else if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_USHORT:
      if(GAL_CONFIG_BIN_OP_USHORT) return GAL_DATA_TYPE_USHORT;
      else
        {
          if     (GAL_CONFIG_BIN_OP_UINT)     return GAL_DATA_TYPE_UINT;
          else if(GAL_CONFIG_BIN_OP_INT)      return GAL_DATA_TYPE_INT;
          else if(GAL_CONFIG_BIN_OP_ULONG)    return GAL_DATA_TYPE_ULONG;
          else if(GAL_CONFIG_BIN_OP_LONG)     return GAL_DATA_TYPE_LONG;
          else if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_SHORT:
      if(GAL_CONFIG_BIN_OP_SHORT) return GAL_DATA_TYPE_SHORT;
      else
        {
          if     (GAL_CONFIG_BIN_OP_INT)      return GAL_DATA_TYPE_INT;
          else if(GAL_CONFIG_BIN_OP_LONG)     return GAL_DATA_TYPE_LONG;
          else if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_UINT:
      if(GAL_CONFIG_BIN_OP_UINT) return GAL_DATA_TYPE_UINT;
      else
        {
          if     (GAL_CONFIG_BIN_OP_ULONG)    return GAL_DATA_TYPE_ULONG;
          else if(GAL_CONFIG_BIN_OP_LONG)     return GAL_DATA_TYPE_LONG;
          else if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_INT:
      if(GAL_CONFIG_BIN_OP_INT) return GAL_DATA_TYPE_INT;
      else
        {
          if     (GAL_CONFIG_BIN_OP_LONG)     return GAL_DATA_TYPE_LONG;
          else if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_ULONG:
      if(GAL_CONFIG_BIN_OP_ULONG) return GAL_DATA_TYPE_ULONG;
      else
        {
          if     (GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_LONG:
      if(GAL_CONFIG_BIN_OP_LONG) return GAL_DATA_TYPE_LONG;
      else
        {
          if     (GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
          else if(GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_LONGLONG:
      if(GAL_CONFIG_BIN_OP_LONGLONG) return GAL_DATA_TYPE_LONGLONG;
      else
        {
          if     (GAL_CONFIG_BIN_OP_FLOAT)    return GAL_DATA_TYPE_FLOAT;
          else if(GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_FLOAT:
      if(GAL_CONFIG_BIN_OP_FLOAT) return GAL_DATA_TYPE_FLOAT;
      else
        {
          if     (GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
        }
      break;

    case GAL_DATA_TYPE_DOUBLE:
      if         (GAL_CONFIG_BIN_OP_DOUBLE)   return GAL_DATA_TYPE_DOUBLE;
      break;

    default:
      error(EXIT_FAILURE, 0, "type %d not recognized in "
            "binary_type_for_convert_to_compiled_type", intype);
    }

  return 0;
}





/* Note that for signed types, we won't be considering the unsigned types
   of the larger types. */
gal_data_t *
data_arithmetic_convert_to_compiled_type(gal_data_t *in, unsigned char flags)
{
  int ntype;
  char *typestring;
  gal_data_t *out=NULL;

  /* Set the best compiled type. */
  ntype=data_arithmetic_nearest_compiled_type(in->type);

  /* If type is not compiled, then convert the dataset to the first
     compiled larger type. */
  if(in->type==ntype)
    out=in;
  else
    {
      if(ntype)
        {
          out=gal_data_copy_to_new_type(in, ntype);
          if(flags & GAL_DATA_ARITH_FREE)
            { gal_data_free(in, 0); in=NULL; }
        }
      else
        {
          typestring=gal_data_type_string(in->type);
          error(EXIT_FAILURE, 0, "The given %s type data given to "
                "binary operators is not compiled for native operation "
                "and no larger types are compiled either.\n\nThe "
                "largest type (which can act as a fallback for any "
                "input type is double, so configure Gnuastro again "
                "with `--enable-bin-op-double' to not get this error "
                "any more. However, if you commonly deal with %s type "
                "data, also enable %s with a similar option at "
                "configure time to greatly decrease running time and "
                "avoid unnecessary RAM and CPU resources. Run"
                "`./configure --help' in Gnuastro's top source "
                "directory (after unpacking the tarball) for the full "
                "list of options", typestring, typestring, typestring);
        }
    }

  /* Return the output data structure */
  if(out==NULL)
    error(EXIT_FAILURE, 0, "A bug! Please contact us at %s, so we can fix "
          "the problem. For some reason, the `out' array in "
          "`binary_convert_to_compiled_type' is not set", PACKAGE_BUGREPORT);
  return out;
}





gal_data_t *
gal_data_arithmetic(int operator, unsigned char flags, ...)
{
  va_list va;
  gal_data_t *d1, *d2, *d3, *out=NULL;

  /* Prepare the variable arguments (starting after the flags argument). */
  va_start(va, flags);

  /* Depending on the operator do the job: */
  switch(operator)
    {

    /* Binary operators with any data type. */
    case GAL_DATA_OPERATOR_PLUS:
    case GAL_DATA_OPERATOR_MINUS:
    case GAL_DATA_OPERATOR_MULTIPLY:
    case GAL_DATA_OPERATOR_DIVIDE:
    case GAL_DATA_OPERATOR_LT:
    case GAL_DATA_OPERATOR_LE:
    case GAL_DATA_OPERATOR_GT:
    case GAL_DATA_OPERATOR_GE:
    case GAL_DATA_OPERATOR_EQ:
    case GAL_DATA_OPERATOR_NE:
    case GAL_DATA_OPERATOR_AND:
    case GAL_DATA_OPERATOR_OR:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=data_arithmetic_binary(operator, flags, d1, d2);
      break;

    case GAL_DATA_OPERATOR_NOT:
      d1 = va_arg(va, gal_data_t *);
      out=data_arithmetic_not(d1, flags);
      break;

    case GAL_DATA_OPERATOR_ISBLANK:
      d1 = va_arg(va, gal_data_t *);
      out = gal_data_flag_blank(d1);
      if(flags & GAL_DATA_ARITH_FREE) gal_data_free(d1, 0);
      break;

    case GAL_DATA_OPERATOR_WHERE:
      d1 = va_arg(va, gal_data_t *);    /* Output value/array.        */
      d2 = va_arg(va, gal_data_t *);    /* Condition (unsigned char). */
      d3 = va_arg(va, gal_data_t *);    /* If true value/array.       */
      data_arithmetic_where(flags, d1, d2, d3);
      out=d1;
      break;


    /* Unary function operators. */
    case GAL_DATA_OPERATOR_SQRT:
    case GAL_DATA_OPERATOR_LOG:
    case GAL_DATA_OPERATOR_LOG10:
    case GAL_DATA_OPERATOR_MINVAL:
    case GAL_DATA_OPERATOR_MAXVAL:
      d1 = va_arg(va, gal_data_t *);
      out=data_arithmetic_unary_function(operator, flags, d1);
      break;

    case GAL_DATA_OPERATOR_ABS:
      d1 = va_arg(va, gal_data_t *);
      out=data_arithmetic_abs(flags, d1);
      break;

    case GAL_DATA_OPERATOR_MIN:
    case GAL_DATA_OPERATOR_MAX:
    case GAL_DATA_OPERATOR_SUM:
    case GAL_DATA_OPERATOR_AVERAGE:
    case GAL_DATA_OPERATOR_MEDIAN:
      d1 = va_arg(va, gal_data_t *);
      out=data_arithmetic_multioperand(operator, flags, d1);
      break;


    /* Binary function operators. */
    case GAL_DATA_OPERATOR_POW:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=data_arithmetic_binary_function_flt(operator, flags, d1, d2);
      break;


    /* Binary operators that only work on integer types. */
    case GAL_DATA_OPERATOR_BITAND:
    case GAL_DATA_OPERATOR_BITOR:
    case GAL_DATA_OPERATOR_BITXOR:
    case GAL_DATA_OPERATOR_BITLSH:
    case GAL_DATA_OPERATOR_BITRSH:
    case GAL_DATA_OPERATOR_MODULO:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=data_arithmetic_onlyint_binary(operator, flags, d1, d2);
      break;

    case GAL_DATA_OPERATOR_BITNOT:
      d1 = va_arg(va, gal_data_t *);
      out=data_arithmetic_bitwise_not(flags, d1);
      break;


    /* Conversion operators. */
    case GAL_DATA_OPERATOR_TO_UCHAR:
    case GAL_DATA_OPERATOR_TO_CHAR:
    case GAL_DATA_OPERATOR_TO_USHORT:
    case GAL_DATA_OPERATOR_TO_SHORT:
    case GAL_DATA_OPERATOR_TO_UINT:
    case GAL_DATA_OPERATOR_TO_INT:
    case GAL_DATA_OPERATOR_TO_ULONG:
    case GAL_DATA_OPERATOR_TO_LONG:
    case GAL_DATA_OPERATOR_TO_LONGLONG:
    case GAL_DATA_OPERATOR_TO_FLOAT:
    case GAL_DATA_OPERATOR_TO_DOUBLE:
      d1 = va_arg(va, gal_data_t *);
      out=data_arithmetic_change_type(d1, operator, flags);
      break;


    /* When operator is not recognized. */
    default:
      error(EXIT_FAILURE, 0, "the argument \"%d\" could not be "
            "interpretted as an operator", operator);
    }

  /* End the variable argument structure and return. */
  va_end(va);
  return out;
}
