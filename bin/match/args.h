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
#ifndef ARGS_H
#define ARGS_H






/* Array of acceptable options. */
struct argp_option program_options[] =
  {
    /* Input file parameters. */
    {
      "hdu2",
      GAL_OPTIONS_KEY_HDU,
      "STR/INT",
      0,
      "Extension name or number of second input.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->hdu2,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },




    /* Outputs. */
    {
      "logasoutput",
      UI_KEY_LOGASOUTPUT,
      0,
      0,
      "No rearranging of inputs, output is log file",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->logasoutput,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },




    /* Catalog matching. */
    {
      0, 0, 0, 0,
      "Catalog matching",
      UI_GROUP_CATALOGMATCH
    },
    {
      "ccol1",
      UI_KEY_CCOL1,
      "STR",
      0,
      "Column name/number of first catalog.",
      UI_GROUP_CATALOGMATCH,
      &p->ccol1,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "ccol2",
      UI_KEY_CCOL2,
      "STR",
      0,
      "Column name/number of second catalog.",
      UI_GROUP_CATALOGMATCH,
      &p->ccol2,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "aperture",
      UI_KEY_APERTURE,
      "FLT[,FLT[,FLT]]",
      0,
      "Acceptable aperture for matching.",
      UI_GROUP_CATALOGMATCH,
      &p->aperture,
      GAL_TYPE_FLOAT64,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_csv_float64
    },


    {0}
  };





/* Define the child argp structure
   -------------------------------

   NOTE: these parts can be left untouched.*/
struct argp
gal_options_common_child = {gal_commonopts_options,
                            gal_options_common_argp_parse,
                            NULL, NULL, NULL, NULL, NULL};

/* Use the child argp structure in list of children (only one for now). */
struct argp_child
children[]=
{
  {&gal_options_common_child, 0, NULL, 0},
  {0, 0, 0, 0}
};

/* Set all the necessary argp parameters. */
struct argp
thisargp = {program_options, parse_opt, args_doc, doc, children, NULL, NULL};
#endif
