/*********************************************************************
Fits - View and manipulate FITS extensions and/or headers.
Fits is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef ARGS_H
#define ARGS_H






/* Array of acceptable options. */
struct argp_option program_options[] =
  {

    {
      0, 0, 0, 0,
      "HDUs (extensions):",
      UI_GROUP_EXTENSION
    },
    {
      "remove",
      UI_KEY_REMOVE,
      "STR",
      0,
      "Remove extension from input file.",
      UI_GROUP_EXTENSION,
      &p->remove,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "copy",
      UI_KEY_COPY,
      "STR",
      0,
      "Copy extension to output file.",
      UI_GROUP_EXTENSION,
      &p->copy,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "cut",
      UI_KEY_CUT,
      "STR",
      0,
      "Copy extension to output and remove from input.",
      UI_GROUP_EXTENSION,
      &p->cut,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },



    {
      0, 0, 0, 0,
      "Keywords (in one HDU):",
      UI_GROUP_KEYWORD
    },
    {
      "asis",
      UI_KEY_ASIS,
      "STR",
      0,
      "Write the argument string as is into the header.",
      UI_GROUP_KEYWORD,
      &p->asis,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "delete",
      UI_KEY_DELETE,
      "STR",
      0,
      "Delete a keyword from the header.",
      UI_GROUP_KEYWORD,
      &p->delete,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "rename",
      UI_KEY_RENAME,
      "STR",
      0,
      "Rename keyword, keeping value and comments.",
      UI_GROUP_KEYWORD,
      &p->rename,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "update",
      UI_KEY_UPDATE,
      "STR",
      0,
      "Update a keyword value or comments.",
      UI_GROUP_KEYWORD,
      &p->update,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "write",
      UI_KEY_WRITE,
      "STR",
      0,
      "Write a keyword (with value, comments and units).",
      UI_GROUP_KEYWORD,
      &p->write,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "history",
      UI_KEY_HISTORY,
      "STR",
      0,
      "Add HISTORY keyword, any length is ok.",
      UI_GROUP_KEYWORD,
      &p->history,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "comment",
      UI_KEY_COMMENT,
      "STR",
      0,
      "Add COMMENT keyword, any length is ok.",
      UI_GROUP_KEYWORD,
      &p->comment,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "date",
      UI_KEY_DATE,
      0,
      0,
      "Set the DATE keyword to the current time.",
      UI_GROUP_KEYWORD,
      &p->date,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "printallkeys",
      UI_KEY_PRINTALLKEYS,
      0,
      0,
      "Print all keywords in the selected HDU.",
      UI_GROUP_KEYWORD,
      &p->printallkeys,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },



    {
      "quitonerror",
      UI_KEY_QUITONERROR,
      0,
      0,
      "Quit if there is an error on any action.",
      GAL_OPTIONS_GROUP_OPERATING_MODE,
      &p->quitonerror,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },



    {0}
  };





/* Define the child argp structure. */
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
