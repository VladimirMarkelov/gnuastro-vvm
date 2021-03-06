/*********************************************************************
Match - A program to match catalogs and WCS warps
Match is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <argp.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>

#include <gnuastro/fits.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/options.h>
#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/fixedstringmacros.h>

#include "main.h"

#include "ui.h"
#include "authors-cite.h"





/**************************************************************/
/*********      Argp necessary global entities     ************/
/**************************************************************/
/* Definition parameters for the Argp: */
const char *
argp_program_version = PROGRAM_STRING "\n"
                       GAL_STRINGS_COPYRIGHT
                       "\n\nWritten/developed by "PROGRAM_AUTHORS;

const char *
argp_program_bug_address = PACKAGE_BUGREPORT;

static char
args_doc[] = "ASTRdata";

const char
doc[] = GAL_STRINGS_TOP_HELP_INFO PROGRAM_NAME" matches catalogs of objects "
  "or returns the warping matrix necessary to match two images.\n"
  GAL_STRINGS_MORE_HELP_INFO
  /* After the list of options: */
  "\v"
  PACKAGE_NAME" home page: "PACKAGE_URL;




















/**************************************************************/
/*********    Initialize & Parse command-line    **************/
/**************************************************************/
static void
ui_initialize_options(struct matchparams *p,
                      struct argp_option *program_options,
                      struct argp_option *gal_commonopts_options)
{
  size_t i;
  struct gal_options_common_params *cp=&p->cp;


  /* Set the necessary common parameters structure. */
  cp->poptions           = program_options;
  cp->program_name       = PROGRAM_NAME;
  cp->program_exec       = PROGRAM_EXEC;
  cp->program_bibtex     = PROGRAM_BIBTEX;
  cp->program_authors    = PROGRAM_AUTHORS;
  cp->coptions           = gal_commonopts_options;


  /* Modify common options. */
  for(i=0; !gal_options_is_last(&cp->coptions[i]); ++i)
    {
      /* Select individually. */
      switch(cp->coptions[i].key)
        {
        case GAL_OPTIONS_KEY_HDU:
          cp->coptions[i].doc="Extension name or number of first input.";
          break;
        }

      /* Select by group. */
      switch(cp->coptions[i].group)
        {
        case GAL_OPTIONS_GROUP_TESSELLATION:
          cp->coptions[i].doc=NULL; /* Necessary to remove title. */
          cp->coptions[i].flags=OPTION_HIDDEN;
          break;
        }
    }
}





/* Parse a single option: */
error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
  struct matchparams *p = state->input;

  /* Pass `gal_options_common_params' into the child parser.  */
  state->child_inputs[0] = &p->cp;

  /* In case the user incorrectly uses the equal sign (for example
     with a short format or with space in the long format, then `arg`
     start with (if the short version was called) or be (if the long
     version was called with a space) the equal sign. So, here we
     check if the first character of arg is the equal sign, then the
     user is warned and the program is stopped: */
  if(arg && arg[0]=='=')
    argp_error(state, "incorrect use of the equal sign (`=`). For short "
               "options, `=` should not be used and for long options, "
               "there should be no space between the option, equal sign "
               "and value");

  /* Set the key to this option. */
  switch(key)
    {

    /* Read the non-option tokens (arguments): */
    case ARGP_KEY_ARG:
      if(p->input1name)
        {
          if(p->input2name)
            argp_error(state, "only two arguments (input files) should be "
                       "given");
          else
            p->input2name=arg;
        }
      else
        p->input1name=arg;
      break;


    /* This is an option, set its value. */
    default:
      return gal_options_set_from_key(key, arg, p->cp.poptions, &p->cp);
    }

  return 0;
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
/* Read and check ONLY the options. When arguments are involved, do the
   check in `ui_check_options_and_arguments'. */
static void
ui_read_check_only_options(struct matchparams *p)
{

}





static void
ui_check_options_and_arguments(struct matchparams *p)
{
  /* Make sure two input file names were given and if they a FITS file,
     that a HDU is also given for each. */
  if(p->input1name)
    {
      if( gal_fits_name_is_fits(p->input1name) && p->cp.hdu==NULL )
        error(EXIT_FAILURE, 0, "no HDU for first input. When the input is "
              "a FITS file, a HDU must also be specified, you can use the "
              "`--hdu' (`-h') option and give it the HDU number (starting "
              "from zero), extension name, or anything acceptable by "
              "CFITSIO");
    }
  else
    error(EXIT_FAILURE, 0, "no input file is specified: two inputs are "
          "necessary");

  if(p->input2name)
    {
      if( gal_fits_name_is_fits(p->input2name) && p->hdu2==NULL )
        error(EXIT_FAILURE, 0, "no HDU for second input. Please use the "
              "`--hdu2' (`-H') option and give it the HDU number (starting "
              "from zero), extension name, or anything acceptable by "
              "CFITSIO");
    }
  else
    error(EXIT_FAILURE, 0, "second input file not specified: two inputs are "
          "necessary");
}




















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
static void
ui_set_mode(struct matchparams *p)
{
  /* Check if we are in image or catalog mode. We will base the mode on the
     first input, then check with the second. */
  if( gal_fits_name_is_fits(p->input1name) )
    p->mode = ( (gal_fits_hdu_format(p->input1name, p->cp.hdu) == IMAGE_HDU)
                ? MATCH_MODE_WCS
                : MATCH_MODE_CATALOG );
  else
    p->mode=MATCH_MODE_CATALOG;

  /* Now that the mode is set, check the second input's type. */
  if( gal_fits_name_is_fits(p->input2name) )
    {
      if(gal_fits_hdu_format(p->input2name, p->hdu2) == IMAGE_HDU)
        {
          if( p->mode==MATCH_MODE_CATALOG)
            error(EXIT_FAILURE, 0, "%s is a catalog, while %s is an image. "
                  "Both inputs have to be images or catalogs",
                  gal_checkset_dataset_name(p->input1name, p->cp.hdu),
                  gal_checkset_dataset_name(p->input2name, p->hdu2) );
        }
      else
        {
          if( p->mode==MATCH_MODE_WCS)
            error(EXIT_FAILURE, 0, "%s is an image, while %s is a catalog. "
                  "Both inputs have to be images or catalogs",
                  gal_checkset_dataset_name(p->input1name, p->cp.hdu),
                  gal_checkset_dataset_name(p->input2name, p->hdu2));
        }
    }
  else
    if(p->mode==MATCH_MODE_WCS)
      error(EXIT_FAILURE, 0, "%s is an image, while %s is a catalog! Both "
            "inputs have to be images or catalogs",
            gal_checkset_dataset_name(p->input1name, p->cp.hdu),
            gal_checkset_dataset_name(p->input2name, p->hdu2));
}





/* The final aperture must have the following values:

       p->aperture[0]: Major axis length.
       p->aperture[1]: Axis ratio.
       p->aperture[2]: Position angle (relative to first dim).     */
static void
ui_read_columns_aperture(struct matchparams *p, size_t numcols)
{
  size_t apersize=3;
  gal_data_t *newaper=NULL;
  double *naper, *oaper=p->aperture->array;

  /* A general sanity check: the first two elements of aperture cannot be
     zero or negative. */
  if( oaper[0]<=0 )
    error(EXIT_FAILURE, 0, "the first value of `--aperture' cannot be "
          "zero or negative");
  if( p->aperture->size>1 && oaper[1]<=0 )
    error(EXIT_FAILURE, 0, "the second value of `--aperture' cannot be "
          "zero or negative");

  /* Will be needed in more than one case. */
  if(p->aperture->size!=3)
    {
      newaper=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &apersize, NULL,
                             0, -1, NULL, NULL, NULL);
      naper=newaper->array;
    }

  /* Different based on  */
  switch(p->aperture->size)
    {
    case 1:
      naper[0]=oaper[0];
      naper[1]=1;
      naper[2]=0;
      break;

    case 2:
      naper[0] = oaper[0]>oaper[1] ? oaper[0]          : oaper[1];
      naper[1] = oaper[0]>oaper[1] ? oaper[1]/oaper[0] : oaper[0]/oaper[1];
      naper[2] = oaper[0]>oaper[1] ? 0                 : 90;
      break;

    case 3:
      if(oaper[1]>1)
        error(EXIT_FAILURE, 0, "second value to `--aperture' is larger "
              "than one. When three numbers are given to this option, the "
              "second is the axis ratio (which must always be less than 1).");
      break;

    default:
      error(EXIT_FAILURE, 0, "%zu values given to `--aperture'. This "
            "option can only take 1, 2, or 3 values", p->aperture->size);
    }

  /* If a new aperture was defined, then replace it with the exitsting
     one. */
  if(newaper)
    {
      gal_data_free(p->aperture);
      p->aperture=newaper;
    }
}





/* Read catalog columns */
static void
ui_read_columns(struct matchparams *p)
{
  struct gal_options_common_params *cp=&p->cp;
  size_t ccol1n=gal_list_str_number(p->ccol1);
  size_t ccol2n=gal_list_str_number(p->ccol2);
  char *diff_cols_error="%s: the number of columns matched (%zu) "
    "differs from the number of usable calls to `--ccol1' (%zu). "
    "Please give more specific values to `--ccol1' (column "
    "numberes are the only identifiers guaranteed to be unique).";

  /* First check the number of columns given. It might happen that extra
     columns are present in configuration files, so we will only abort if
     the total number isn't enough. When it is too much, we'll just free
     the rest of the list.*/
  if(ccol1n<2 || ccol2n<2)
    error(EXIT_FAILURE, 0, "at least two coordinate columns from each "
          "catalog must be given for the match. Please use repeated "
          "calls to `--ccol1' and `--ccol2' to specify the columns by "
          "name (if they have one) or number (starting from 1).\n\n"
          "You can use this command to list the column information of "
          "a table in the N-th extension/HDU of a FITS file:\n\n"
          "    $ asttable filename.fits -hN -i\n\n"
          "For more information on selecting table columns in Gnuastro, "
          "please run the following command:\n\n"
          "    $ info gnuastro \"selecting table columns\"\n");
  else if(ccol1n>2 || ccol2n>2)
    {
      if(ccol1n>2)
        {
          gal_list_str_free(p->ccol1->next->next, 1);
          p->ccol1->next->next=NULL;
          ccol1n=2;             /* Will be used later. */
        }
      if(ccol2n>2)
        {
          gal_list_str_free(p->ccol2->next->next, 1);
          p->ccol2->next->next=NULL;
          ccol2n=2;             /* Will be used later. */
        }
    }


  /* Read/check the aperture values. */
  if(p->aperture)
    ui_read_columns_aperture(p, ccol1n);
  else
    error(EXIT_FAILURE, 0, "no matching aperture specified. Please use "
          "the `--aperture' option to define the acceptable aperture for "
          "matching the coordinates (in the same units as each "
          "dimension). Please run the following command for more "
          "information.\n\n    $ info %s\n", PROGRAM_EXEC);


  /* Read the columns. */
  if(cp->searchin)
    {
      /* Read the first dataset. */
      p->cols1=gal_table_read(p->input1name, cp->hdu, p->ccol1, cp->searchin,
                              cp->ignorecase, cp->minmapsize);
      if(gal_list_data_number(p->cols1)!=ccol1n)
        error(EXIT_FAILURE, 0, diff_cols_error,
              gal_checkset_dataset_name(p->input1name, cp->hdu),
              gal_list_data_number(p->cols1), ccol1n);

      /* Read the second dataset. */
      p->cols2=gal_table_read(p->input2name, p->hdu2, p->ccol2, cp->searchin,
                              cp->ignorecase, cp->minmapsize);
      if(gal_list_data_number(p->cols2)!=ccol2n)
        error(EXIT_FAILURE, 0, diff_cols_error,
              gal_checkset_dataset_name(p->input2name, p->hdu2),
              gal_list_data_number(p->cols2), ccol2n);
    }
  else
    error(EXIT_FAILURE, 0, "no `--searchin' option specified. Please run "
          "the following command for more information:\n\n"
          "    $ info gnuastro \"selecting table columns\"\n");
}





static void
ui_preparations_out_name(struct matchparams *p)
{
  if(p->logasoutput)
    {
      /* Set the logname (as output). */
      if(p->cp.output)
        gal_checkset_allocate_copy(p->cp.output, &p->logname);
      else
        {
          if(p->cp.tableformat==GAL_TABLE_FORMAT_TXT)
            p->logname=gal_checkset_automatic_output(&p->cp, p->input1name,
                                                     "_matched.txt");
          else
            p->logname=gal_checkset_automatic_output(&p->cp, p->input1name,
                                                     "_matched.fits");
        }

      /* Make sure a file with this name doesn't exist. */
      gal_checkset_writable_remove(p->out1name, 0, p->cp.dontdelete);
    }
  else
    {
      /* Set `p->out1name' and `p->out2name'. */
      if(p->cp.output)
        {
          if( gal_fits_name_is_fits(p->cp.output) )
            {
              gal_checkset_allocate_copy(p->cp.output, &p->out1name);
              gal_checkset_allocate_copy(p->cp.output, &p->out2name);
            }
          else
            {
              p->out1name=gal_checkset_automatic_output(&p->cp, p->cp.output,
                                                        "_matched_1.txt");
              p->out2name=gal_checkset_automatic_output(&p->cp, p->cp.output,
                                                        "_matched_2.txt");
            }
        }
      else
        {
          if(p->cp.tableformat==GAL_TABLE_FORMAT_TXT)
            {
              p->out1name=gal_checkset_automatic_output(&p->cp, p->input1name,
                                                        "_matched_1.txt");
              p->out2name=gal_checkset_automatic_output(&p->cp, p->input2name,
                                                        "_matched_2.txt");
            }
          else
            {
              p->out1name=gal_checkset_automatic_output(&p->cp, p->input1name,
                                                        "_matched.fits");
              gal_checkset_allocate_copy(p->out1name, &p->out2name);
            }
        }

      /* Make sure no file with these names exists. */
      gal_checkset_writable_remove(p->out1name, 0, p->cp.dontdelete);
      gal_checkset_writable_remove(p->out2name, 0, p->cp.dontdelete);

      /* If a log file is necessary, set its name here. */
      if(p->cp.log)
        {
          p->logname = ( p->cp.tableformat==GAL_TABLE_FORMAT_TXT
                         ? PROGRAM_EXEC".txt"
                         : PROGRAM_EXEC".fits" );
          gal_checkset_writable_remove(p->logname, 0, p->cp.dontdelete);
        }
    }
}





static void
ui_preparations(struct matchparams *p)
{
  /* Set the mode of the program. */
  ui_set_mode(p);

  /* Currently Match only works on catalogs. */
  if(p->mode==MATCH_MODE_WCS)
    error(EXIT_FAILURE, 0, "currently Match only works on catalogs, we will "
          "implement the WCS matching routines later");
  else
    ui_read_columns(p);

  /* Set the output filename. */
  ui_preparations_out_name(p);
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/
void
ui_read_check_inputs_setup(int argc, char *argv[], struct matchparams *p)
{
  struct gal_options_common_params *cp=&p->cp;


  /* Include the parameters necessary for argp from this program (`args.h')
     and for the common options to all Gnuastro (`commonopts.h'). We want
     to directly put the pointers to the fields in `p' and `cp', so we are
     simply including the header here to not have to use long macros in
     those headers which make them hard to read and modify. This also helps
     in having a clean environment: everything in those headers is only
     available within the scope of this function. */
#include <gnuastro-internal/commonopts.h>
#include "args.h"


  /* Initialize the options and necessary information.  */
  ui_initialize_options(p, program_options, gal_commonopts_options);


  /* Read the command-line options and arguments. */
  errno=0;
  if(argp_parse(&thisargp, argc, argv, 0, 0, p))
    error(EXIT_FAILURE, errno, "parsing arguments");


  /* Read the configuration files and set the common values. */
  gal_options_read_config_set(&p->cp);


  /* Read the options into the program's structure, and check them and
     their relations prior to printing. */
  ui_read_check_only_options(p);


  /* Print the option values if asked. Note that this needs to be done
     after the option checks so un-sane values are not printed in the
     output state. */
  gal_options_print_state(&p->cp);


  /* Check that the options and arguments fit well with each other. Note
     that arguments don't go in a configuration file. So this test should
     be done after (possibly) printing the option values. */
  ui_check_options_and_arguments(p);


  /* Read/allocate all the necessary starting arrays. */
  ui_preparations(p);
}




















/**************************************************************/
/************      Free allocated, report         *************/
/**************************************************************/
void
ui_free_report(struct matchparams *p, struct timeval *t1)
{
  /* Free the allocated arrays: */
  free(p->cp.hdu);
  free(p->out1name);
  free(p->out2name);
  free(p->cp.output);

  /* Print the final message.
  if(!p->cp.quiet)
    gal_timing_report(t1, PROGRAM_NAME" finished in: ", 0);
  */
}
