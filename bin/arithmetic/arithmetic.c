/*********************************************************************
Arithmetic - Do arithmetic operations on images.
Arithmetic is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/wcs.h>
#include <gnuastro/fits.h>
#include <gnuastro/threads.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>
#include <gnuastro/arithmetic.h>

#include <gnuastro-internal/checkset.h>

#include "main.h"

#include "operands.h"
#include "arithmetic.h"














/***************************************************************/
/*************          Internal functions         *************/
/***************************************************************/
#define SET_NUM_OP(CTYPE) {                                \
    CTYPE a=*(CTYPE *)(data->array); if(a>0) return a;    }

static size_t
pop_number_of_operands(struct arithmeticparams *p, gal_data_t *data,
                       char *token_string)
{
  /* Check if its a number. */
  if(data->size>1)
    error(EXIT_FAILURE, 0, "the first popped operand to the \"%s\" "
          "operator must be a number, not an array", token_string);

  /* Check its type and return the value. */
  switch(data->type)
    {

    /* For the integer types, if they are unsigned, then just pass their
       value, if they are signed, you have to make sure they are zero or
       positive. */
    case GAL_TYPE_UINT8:   SET_NUM_OP(uint8_t);     break;
    case GAL_TYPE_INT8:    SET_NUM_OP(int8_t);      break;
    case GAL_TYPE_UINT16:  SET_NUM_OP(uint16_t);    break;
    case GAL_TYPE_INT16:   SET_NUM_OP(int16_t);     break;
    case GAL_TYPE_UINT32:  SET_NUM_OP(uint32_t);    break;
    case GAL_TYPE_INT32:   SET_NUM_OP(int32_t);     break;
    case GAL_TYPE_UINT64:  SET_NUM_OP(uint64_t);    break;
    case GAL_TYPE_INT64:   SET_NUM_OP(int64_t);     break;

    /* Floating point numbers are not acceptable in this context. */
    case GAL_TYPE_FLOAT32:
    case GAL_TYPE_FLOAT64:
      error(EXIT_FAILURE, 0, "the first popped operand to the \"%s\" "
            "operator must be an integer type", token_string);

    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, data->type);
    }

  /* If control reaches here, then the number must have been a negative
     value, so print an error. */
  error(EXIT_FAILURE, 0, "the first popped operand to the \"%s\" operator "
        "cannot be zero or a negative number", token_string);
  return 0;
}




















/**********************************************************************/
/****************         Filtering operators         *****************/
/**********************************************************************/
#define ARITHMETIC_FILTER_DIM 10

struct arithmetic_filter_p
{
  int      operator;            /* The type of filtering.          */
  size_t     *fsize;            /* Filter size.                    */
  size_t   *hpfsize;            /* Positive Half-filter size.      */
  size_t   *hnfsize;            /* Negative Half-filter size.      */
  gal_data_t *input;            /* Input dataset.                  */
  gal_data_t   *out;            /* Output dataset.                 */

  int      hasblank;            /* If the dataset has blank values.*/
};





/* Main filtering work function. */
static void *
arithmetic_filter(void *in_prm)
{
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct arithmetic_filter_p *afp=(struct arithmetic_filter_p *)tprm->params;
  gal_data_t *input=afp->input;

  size_t ind,index;
  int out_type_checked=0;
  gal_data_t *result=NULL;
  size_t *hpfsize=afp->hpfsize, *hnfsize=afp->hnfsize;
  size_t *tsize, *dsize=input->dsize, *fsize=afp->fsize;
  size_t i, j, coord[ARITHMETIC_FILTER_DIM], ndim=input->ndim;
  size_t start[ARITHMETIC_FILTER_DIM], end[ARITHMETIC_FILTER_DIM];
  gal_data_t *tile=gal_data_alloc(NULL, input->type, ndim, afp->fsize, NULL,
                                  0, -1, NULL, NULL, NULL);

  /* Prepare the tile. */
  free(tile->array);
  tsize=tile->dsize;
  tile->block=input;


  /* Go over all the pixels that were assigned to this thread. */
  for(i=0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
    {
      /* Get the coordinate of the pixel. */
      ind=tprm->indexs[i];
      gal_dimension_index_to_coord(ind, ndim, dsize, coord);

      /* See which dimensions need trimming. */
      tile->size=1;
      for(j=0;j<ndim;++j)
        {
          /* Estimate the coordinate of the filter's starting point. Note
             that we are dealing with size_t (unsigned int) type here, so
             there are no negatives. A negative result will produce an
             extremely large number, so instead of checking for negative,
             we can just see if the result of a subtraction is less than
             the width of the input. */
          if( (coord[j] - hnfsize[j] > dsize[j])
              || (coord[j] + hpfsize[j] >= dsize[j]) )
            {
              start[j] = ( (coord[j] - hnfsize[j] > dsize[j])
                           ? 0 : coord[j] - hnfsize[j] );
              end[j]   = ( (coord[j] + hpfsize[j] >= dsize[j])
                           ? dsize[j]
                           : coord[j] + hpfsize[j] + 1);
              tsize[j] = end[j] - start[j];
            }
          else  /* We are NOT on the edge (given requested filter width). */
            {
              tsize[j] = fsize[j];
              start[j] = coord[j] - hnfsize[j];
            }
          tile->size *= tsize[j];
        }

      /* Set the tile's starting pointer. */
      index=gal_dimension_coord_to_index(ndim, dsize, start);
      tile->array=gal_data_ptr_increment(input->array, index, input->type);

      /* Do the necessary calculation. */
      switch(afp->operator)
        {
        case ARITHMETIC_OP_FILTER_MEDIAN:
          result=gal_statistics_median(tile, 0);
          break;

        case ARITHMETIC_OP_FILTER_MEAN:
          result=gal_statistics_mean(tile);
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. `afp->operator' code %d is not "
                "recognized", PACKAGE_BUGREPORT, __func__, afp->operator);
        }

      /* Make sure the output array type and result's type are the same. We
         only need to do this once, but we'll suffice to once for each
         thread for simplicify of the code, it is too negligible to have
         any real affect. */
      if( out_type_checked==0)
        {
          if(result->type!=afp->out->type )
            error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s so "
                  "we can address the problem. The tyes of `result' and "
                  "`out' aren't the same, they are respectively: `%s' and "
                  "`%s'", __func__, PACKAGE_BUGREPORT,
                  gal_type_name(result->type, 1),
                  gal_type_name(afp->out->type, 1));
          out_type_checked=1;
        }

      /* Copy the result into the output array. */
      memcpy(gal_data_ptr_increment(afp->out->array, ind, afp->out->type),
             result->array, gal_type_sizeof(afp->out->type));

      /* Clean up. */
      gal_data_free(result);
    }


  /* Clean up. */
  tile->array=NULL;
  tile->block=NULL;
  gal_data_free(tile);


  /* Wait for all the other threads to finish, then return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}





static void
wrapper_for_filter(struct arithmeticparams *p, char *token, int operator)
{
  size_t i=0, ndim, one=1;
  int type=GAL_TYPE_INVALID;
  struct arithmetic_filter_p afp={0};
  size_t fsize[ARITHMETIC_FILTER_DIM];
  gal_data_t *tmp, *tmp2, *zero, *comp, *fsize_list=NULL;
  size_t hnfsize[ARITHMETIC_FILTER_DIM], hpfsize[ARITHMETIC_FILTER_DIM];


  /* Get the input's number of dimensions. */
  afp.input=operands_pop(p, token);
  afp.operator=operator;
  ndim=afp.input->ndim;
  afp.hnfsize=hnfsize;
  afp.hpfsize=hpfsize;
  afp.fsize=fsize;


  /* A small sanity check. */
  if(ndim>ARITHMETIC_FILTER_DIM)
    error(EXIT_FAILURE, 0, "%s: currently only datasets with less than "
          "%d dimensions are acceptable. The input has %zu dimensions",
          __func__, ARITHMETIC_FILTER_DIM, ndim);


  /* A zero value for checking the value of input widths. */
  zero=gal_data_alloc(NULL, GAL_TYPE_INT32, 1, &one, NULL, 1, -1, NULL,
                      NULL, NULL);


  /* Based on the dimensions of the popped operand, pop the necessary
     number of operands. */
  for(i=0;i<ndim;++i)
    gal_list_data_add(&fsize_list, operands_pop(p, token));


  /* Make sure the filter size only has single values. */
  i=0;
  for(tmp=fsize_list; tmp!=NULL; tmp=tmp->next)
    {
      ++i;
      if(tmp->size!=1)
        error(EXIT_FAILURE, 0, "the filter length values given to the "
              "filter operators can only be numbers. Value number %zu has "
              "%zu elements, so its an array", ndim-i-1, tmp->size);
    }


  /* If the input only has one element, filtering makes no sense, so don't
     waste time, just add the input onto the stack. */
  if(afp.input->size==1) afp.out=afp.input;
  else
    {
      /* Allocate an array for the size of the filter and fill it in. The
         values must be written in the inverse order since the user gives
         dimensions with the FITS standard. */
      i=ndim-1;
      for(tmp=fsize_list; tmp!=NULL; tmp=tmp->next)
        {
          /* Make sure the user has given an integer type. */
          if(tmp->type==GAL_TYPE_FLOAT32 || tmp->type==GAL_TYPE_FLOAT64)
            error(EXIT_FAILURE, 0, "lengths of filter along dimensions "
                  "must be integer values, not floats. The given length "
                  "along dimension %zu is a float", ndim-i);

          /* Make sure it isn't negative. */
          comp=gal_arithmetic(GAL_ARITHMETIC_OP_GT, 0, tmp, zero);
          if( *(uint8_t *)(comp->array) == 0 )
            error(EXIT_FAILURE, 0, "lengths of filter along dimensions "
                  "must be positive. The given length in dimension %zu"
                  "is either zero or negative", ndim-i);

          /* Convert the input into size_t and put it into the array that
             keeps the filter size. */
          tmp2=gal_data_copy_to_new_type(tmp, GAL_TYPE_SIZE_T);
          fsize[ i ] = *(size_t *)(tmp2->array);
          gal_data_free(tmp2);

          /* If the width is larger than the input's size, change the width
             to the input's size. */
          if( fsize[i] > afp.input->dsize[i] )
            error(EXIT_FAILURE, 0, "%s: the filter size along dimension %zu "
                  "(%zu) is greater than the input's length in that "
                  "dimension (%zu)", __func__, i, fsize[i],
                  afp.input->dsize[i]);

          /* Go onto the previous dimension. */
          --i;
        }


      /* Set the half filter sizes. Note that when the size is an odd
         number, the number of pixels before and after the actual pixel are
         equal, but for an even number, we will look into one element more
         when looking before than the ones after. */
      for(i=0;i<ndim;++i)
        {
          if( fsize[i]%2 )
            hnfsize[i]=hpfsize[i]=fsize[i]/2;
          else
            { hnfsize[i]=fsize[i]/2; hpfsize[i]=fsize[i]/2-1; }
        }


      /* See if the input has blank pixels. */
      afp.hasblank=gal_blank_present(afp.input, 1);

      /* Set the type of the output dataset. */
      switch(operator)
        {
        case ARITHMETIC_OP_FILTER_MEDIAN:
          type=afp.input->type;
          break;

        case ARITHMETIC_OP_FILTER_MEAN:
          type=GAL_TYPE_FLOAT64;
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s to fix "
                "the problem. The `operator' code %d is not recognized",
                PACKAGE_BUGREPORT, __func__, operator);
        }

      /* Allocate the output dataset. Note that filtering doesn't change
         the units of the dataset. */
      afp.out=gal_data_alloc(NULL, type, ndim, afp.input->dsize,
                             afp.input->wcs, 0, afp.input->minmapsize,
                             NULL, afp.input->unit, NULL);

      /* Spin off threads for each pixel. */
      gal_threads_spin_off(arithmetic_filter, &afp, afp.input->size,
                           p->cp.numthreads);
    }


  /* Add the output to the top of the stack. */
  operands_add(p, NULL, afp.out);


  /* Clean up and add the output on top of the stack */
  gal_data_free(afp.input);
  gal_list_data_free(fsize_list);
}




















/***************************************************************/
/*************      Reverse Polish algorithm       *************/
/***************************************************************/
/* This function implements the reverse polish algorithm as explained
   in the Wikipedia page.

   NOTE that in ui.c, the input linked list of tokens was ordered to
   have the same order as what the user provided. */
void
reversepolish(struct arithmeticparams *p)
{
  int op=0, nop=0;
  char *filename, *hdu;
  unsigned int numop, i;
  gal_list_str_t *token;
  gal_data_t *d1=NULL, *d2=NULL, *d3=NULL;
  unsigned char flags = ( GAL_ARITHMETIC_INPLACE | GAL_ARITHMETIC_FREE
                          | GAL_ARITHMETIC_NUMOK );


  /* Prepare the processing: */
  p->operands=NULL;
  p->addcounter=p->popcounter=0;


  /* Go over each input token and do the work. */
  for(token=p->tokens;token!=NULL;token=token->next)
    {
      /* If we have a name or number, then add it to the operands linked
         list. Otherwise, pull out two members and do the specified
         operation on them. */
      if(gal_fits_name_is_fits(token->v))
        operands_add(p, token->v, NULL);
      else if( (d1=gal_data_copy_string_to_number(token->v)) )
        operands_add(p, NULL, d1);
      else
        {

          /* Order is the same as in the manual. */
          /* Simple arithmetic operators. */
          if      (!strcmp(token->v, "+" ))
            { op=GAL_ARITHMETIC_OP_PLUS;          nop=2;  }
          else if (!strcmp(token->v, "-" ))
            { op=GAL_ARITHMETIC_OP_MINUS;         nop=2;  }
          else if (!strcmp(token->v, "x" ))
            { op=GAL_ARITHMETIC_OP_MULTIPLY;      nop=2;  }
          else if (!strcmp(token->v, "/" ))
            { op=GAL_ARITHMETIC_OP_DIVIDE;        nop=2;  }
          else if (!strcmp(token->v, "%" ))
            { op=GAL_ARITHMETIC_OP_MODULO;        nop=2;  }

          /* Mathematical Operators. */
          else if (!strcmp(token->v, "abs"))
            { op=GAL_ARITHMETIC_OP_ABS;           nop=1;  }
          else if (!strcmp(token->v, "pow"))
            { op=GAL_ARITHMETIC_OP_POW;           nop=2;  }
          else if (!strcmp(token->v, "sqrt"))
            { op=GAL_ARITHMETIC_OP_SQRT;          nop=1;  }
          else if (!strcmp(token->v, "log"))
            { op=GAL_ARITHMETIC_OP_LOG;           nop=1;  }
          else if (!strcmp(token->v, "log10"))
            { op=GAL_ARITHMETIC_OP_LOG10;         nop=1;  }

          /* Statistical/higher-level operators. */
          else if (!strcmp(token->v, "minvalue"))
            { op=GAL_ARITHMETIC_OP_MINVAL;        nop=1;  }
          else if (!strcmp(token->v, "maxvalue"))
            { op=GAL_ARITHMETIC_OP_MAXVAL;        nop=1;  }
          else if (!strcmp(token->v, "numvalue"))
            { op=GAL_ARITHMETIC_OP_NUMVAL;        nop=1;  }
          else if (!strcmp(token->v, "sumvalue"))
            { op=GAL_ARITHMETIC_OP_SUMVAL;        nop=1;  }
          else if (!strcmp(token->v, "meanvalue"))
            { op=GAL_ARITHMETIC_OP_MEANVAL;       nop=1;  }
          else if (!strcmp(token->v, "stdvalue"))
            { op=GAL_ARITHMETIC_OP_STDVAL;        nop=1;  }
          else if (!strcmp(token->v, "medianvalue"))
            { op=GAL_ARITHMETIC_OP_MEDIANVAL;     nop=1;  }
          else if (!strcmp(token->v, "min"))
            { op=GAL_ARITHMETIC_OP_MIN;           nop=-1; }
          else if (!strcmp(token->v, "max"))
            { op=GAL_ARITHMETIC_OP_MAX;           nop=-1; }
          else if (!strcmp(token->v, "num"))
            { op=GAL_ARITHMETIC_OP_NUM;           nop=-1; }
          else if (!strcmp(token->v, "sum"))
            { op=GAL_ARITHMETIC_OP_SUM;           nop=-1; }
          else if (!strcmp(token->v, "mean"))
            { op=GAL_ARITHMETIC_OP_MEAN;          nop=-1; }
          else if (!strcmp(token->v, "std"))
            { op=GAL_ARITHMETIC_OP_STD;           nop=-1; }
          else if (!strcmp(token->v, "median"))
            { op=GAL_ARITHMETIC_OP_MEDIAN;        nop=-1; }

          /* Conditional operators. */
          else if (!strcmp(token->v, "lt" ))
            { op=GAL_ARITHMETIC_OP_LT;            nop=2;  }
          else if (!strcmp(token->v, "le"))
            { op=GAL_ARITHMETIC_OP_LE;            nop=2;  }
          else if (!strcmp(token->v, "gt" ))
            { op=GAL_ARITHMETIC_OP_GT;            nop=2;  }
          else if (!strcmp(token->v, "ge"))
            { op=GAL_ARITHMETIC_OP_GE;            nop=2;  }
          else if (!strcmp(token->v, "eq"))
            { op=GAL_ARITHMETIC_OP_EQ;            nop=2;  }
          else if (!strcmp(token->v, "ne"))
            { op=GAL_ARITHMETIC_OP_NE;            nop=2;  }
          else if (!strcmp(token->v, "and"))
            { op=GAL_ARITHMETIC_OP_AND;           nop=2;  }
          else if (!strcmp(token->v, "or"))
            { op=GAL_ARITHMETIC_OP_OR;            nop=2;  }
          else if (!strcmp(token->v, "not"))
            { op=GAL_ARITHMETIC_OP_NOT;           nop=1;  }
          else if (!strcmp(token->v, "isblank"))
            { op=GAL_ARITHMETIC_OP_ISBLANK;       nop=1;  }
          else if (!strcmp(token->v, "where"))
            { op=GAL_ARITHMETIC_OP_WHERE;         nop=3;  }

          /* Bitwise operators. */
          else if (!strcmp(token->v, "bitand"))
            { op=GAL_ARITHMETIC_OP_BITAND;        nop=2;  }
          else if (!strcmp(token->v, "bitor"))
            { op=GAL_ARITHMETIC_OP_BITOR;         nop=2;  }
          else if (!strcmp(token->v, "bitxor"))
            { op=GAL_ARITHMETIC_OP_BITXOR;        nop=2;  }
          else if (!strcmp(token->v, "lshift"))
            { op=GAL_ARITHMETIC_OP_BITLSH;        nop=2;  }
          else if (!strcmp(token->v, "rshift"))
            { op=GAL_ARITHMETIC_OP_BITRSH;        nop=2;  }
          else if (!strcmp(token->v, "bitnot"))
            { op=GAL_ARITHMETIC_OP_BITNOT;        nop=1;  }

          /* Type conversion. */
          else if (!strcmp(token->v, "uint8"))
            { op=GAL_ARITHMETIC_OP_TO_UINT8;      nop=1;  }
          else if (!strcmp(token->v, "int8"))
            { op=GAL_ARITHMETIC_OP_TO_INT8;       nop=1;  }
          else if (!strcmp(token->v, "uint16"))
            { op=GAL_ARITHMETIC_OP_TO_UINT16;     nop=1;  }
          else if (!strcmp(token->v, "int16"))
            { op=GAL_ARITHMETIC_OP_TO_INT16;      nop=1;  }
          else if (!strcmp(token->v, "uint32"))
            { op=GAL_ARITHMETIC_OP_TO_UINT32;     nop=1;  }
          else if (!strcmp(token->v, "int32"))
            { op=GAL_ARITHMETIC_OP_TO_INT32;      nop=1;  }
          else if (!strcmp(token->v, "uint64"))
            { op=GAL_ARITHMETIC_OP_TO_UINT64;     nop=1;  }
          else if (!strcmp(token->v, "int64"))
            { op=GAL_ARITHMETIC_OP_TO_INT64;      nop=1;  }
          else if (!strcmp(token->v, "float32"))
            { op=GAL_ARITHMETIC_OP_TO_FLOAT32;    nop=1;  }
          else if (!strcmp(token->v, "float64"))
            { op=GAL_ARITHMETIC_OP_TO_FLOAT64;    nop=1;  }

          /* Filters. */
          else if (!strcmp(token->v, "filter-median"))
            { op=ARITHMETIC_OP_FILTER_MEDIAN;     nop=0;  }
          else if (!strcmp(token->v, "filter-mean"))
            { op=ARITHMETIC_OP_FILTER_MEAN;       nop=0;  }


          /* Finished checks with known operators */
          else
            error(EXIT_FAILURE, 0, "the argument \"%s\" could not be "
                  "interpretted as a FITS file, number, or operator",
                  token->v);


          /* See if the arithmetic library must be called or not. */
          if(nop)
            {
              /* Pop the necessary number of operators. Note that the
                 operators are poped from a linked list (which is
                 last-in-first-out). So for the operators which need a
                 specific order, the first poped operand is actally the
                 last (right most, in in-fix notation) input operand.*/
              switch(nop)
                {
                case 1:
                  d1=operands_pop(p, token->v);
                  break;

                case 2:
                  d2=operands_pop(p, token->v);
                  d1=operands_pop(p, token->v);
                  break;

                case 3:
                  d3=operands_pop(p, token->v);
                  d2=operands_pop(p, token->v);
                  d1=operands_pop(p, token->v);
                  break;

                case -1:
                  /* This case is when the number of operands is itself an
                     operand. So the first popped operand must be an
                     integer number, we will use that to construct a linked
                     list of any number of operands within the single `d1'
                     pointer. */
                  d1=NULL;
                  numop=pop_number_of_operands(p, operands_pop(p, token->v),
                                               token->v);
                  for(i=0;i<numop;++i)
                    gal_list_data_add(&d1, operands_pop(p, token->v));
                  break;

                default:
                  error(EXIT_FAILURE, 0, "no operators, `%s' needs %d "
                        "operand(s)", token->v, nop);
                }


              /* Run the arithmetic operation. Note that `gal_arithmetic'
                 is a variable argument function (like printf). So the
                 number of arguments it uses depend on the operator. So
                 when the operator doesn't need three operands, the extra
                 arguments will be ignored. */
              operands_add(p, NULL, gal_arithmetic(op, flags, d1, d2, d3));
            }

          /* No need to call the arithmetic library, call the proper
             wrappers directly. */
          else
            {
              switch(op)
                {
                case ARITHMETIC_OP_FILTER_MEAN:
                case ARITHMETIC_OP_FILTER_MEDIAN:
                  wrapper_for_filter(p, token->v, op);
                  break;

                default:
                  error(EXIT_FAILURE, 0, "%s: a bug! please contact us at "
                        "%s to fix the problem. The code %d is not "
                        "recognized for `op'", __func__, PACKAGE_BUGREPORT,
                        op);
                }
            }
        }
    }

  /* If there is more than one node in the operands stack then the user has
     given too many operands which is an error. */
  if(p->operands->next!=NULL)
    error(EXIT_FAILURE, 0, "too many operands");


  /* If the final operand has a filename, but its `data' element is NULL,
     then the file hasn't actually be read yet. In this case, we need to
     read the contents of the file and put the resulting dataset into the
     operands `data' element. This can happen for example if no operators
     are called and there is only one filename as an argument (which can
     happen in scripts). */
  if(p->operands->data==NULL && p->operands->filename)
    {
      /* Read the desired image and report it if necessary. */
      hdu=p->operands->hdu;
      filename=p->operands->filename;
      if( gal_fits_name_is_fits(filename) )
        {
          p->operands->data=gal_fits_img_read(filename,hdu,p->cp.minmapsize,
                                              0, 0);
          p->refdata.wcs=p->operands->data->wcs;
          p->refdata.nwcs=p->operands->data->nwcs;
          p->operands->data->wcs=NULL;
          if(!p->cp.quiet) printf(" - %s (hdu %s) is read.\n", filename, hdu);
        }
      else
        error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s to fix "
              "the problem. While `operands->data' is NULL, the filename "
              "(`%s') is not recognized as a FITS file", __func__,
              PACKAGE_BUGREPORT, filename);
    }


  /* If the final data structure has more than one element, write it as a
     FITS file. Otherwise, print it in the standard output. */
  d1=p->operands->data;
  if(d1->size==1)
    {
      /* To simplify the printing process, we will first change it to
         double, then use printf's `%g' to print it, so integers will be
         printed as an integer.  */
      d2=gal_data_copy_to_new_type(d1, GAL_TYPE_FLOAT64);
      printf("%g\n", *(double *)d2->array);
      if(d2!=d1) gal_data_free(d2);
    }
  else
    {
      /* Put a copy of the WCS structure from the reference image, it
         will be freed while freeing d1. */
      d1->wcs=p->refdata.wcs;
      gal_fits_img_write(d1, p->cp.output, NULL, PROGRAM_NAME);
      if(!p->cp.quiet)
        printf(" - Output written to %s\n", p->cp.output);
    }


  /* Clean up, note that above, we copied the pointer to `refdata->wcs'
     into `d1', so it is freed when freeing d1. */
  gal_data_free(d1);
  free(p->refdata.dsize);

  /* Clean up. Note that the tokens were taken from the command-line
     arguments, so the string within each token linked list must not be
     freed. */
  gal_list_str_free(p->tokens, 0);
  free(p->operands);
}



















/***************************************************************/
/*************             Top function            *************/
/***************************************************************/
void
imgarith(struct arithmeticparams *p)
{
  /* Parse the arguments */
  reversepolish(p);
}
