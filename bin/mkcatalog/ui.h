/*********************************************************************
MakeCatalog - Make a catalog from an input and labeled image.
MakeCatalog is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef UI_H
#define UI_H

/* For common options groups. */
#include <gnuastro-internal/options.h>





/* Option groups particular to this program. */
enum program_args_groups
{
  UI_GROUP_UPPERLIMIT = GAL_OPTIONS_GROUP_AFTER_COMMON,
  UI_GROUP_COLUMNS_IDS,
  UI_GROUP_COLUMNS_POSITION_PIXEL,
  UI_GROUP_COLUMNS_POSITION_WCS,
  UI_GROUP_COLUMNS_BRIGHTNESS,
  UI_GROUP_COLUMNS_MORPHOLOGY,
};





/* Available letters for short options:

   f g k l u v w x y z
   H J L W X Y
*/
enum option_keys_enum
{
  /* With short-option version. */
  UI_KEY_OBJECTSFILE     = 'O',         /* General settings. */
  UI_KEY_CLUMPSFILE      = 'C',
  UI_KEY_SKYFILE         = 's',
  UI_KEY_STDFILE         = 't',
  UI_KEY_SKYSUBTRACTED   = 'E',
  UI_KEY_THRESHOLD       = 'R',
  UI_KEY_ENVSEED         = 'e',

  UI_KEY_IDS             = 'i',         /* Catalog columns. */
  UI_KEY_HOSTOBJID       = 'j',
  UI_KEY_NUMCLUMPS       = 'c',
  UI_KEY_AREA            = 'a',
  UI_KEY_X               = 'x',
  UI_KEY_Y               = 'y',
  UI_KEY_RA              = 'r',
  UI_KEY_DEC             = 'd',
  UI_KEY_BRIGHTNESS      = 'b',
  UI_KEY_MAGNITUDE       = 'm',
  UI_KEY_MAGNITUDEERR    = 'G',
  UI_KEY_UPPERLIMITMAG   = 'u',
  UI_KEY_SN              = 'n',
  UI_KEY_SEMIMAJOR       = 'A',
  UI_KEY_SEMIMINOR       = 'B',
  UI_KEY_AXISRATIO       = 'Q',
  UI_KEY_POSITIONANGLE   = 'p',

  /* Only with long version (start with a value 1000, the rest will be set
     automatically). */
  UI_KEY_OBJECTSHDU      = 1000,        /* General settings. */
  UI_KEY_CLUMPSHDU,
  UI_KEY_SKYHDU,
  UI_KEY_STDHDU,
  UI_KEY_ZEROPOINT,
  UI_KEY_SFMAGNSIGMA,
  UI_KEY_SFMAGAREA,
  UI_KEY_UPMASKFILE,
  UI_KEY_UPMASKHDU,
  UI_KEY_UPNUM,
  UI_KEY_UPRANGE,
  UI_KEY_UPSIGMACLIP,
  UI_KEY_UPNSIGMA,

  UI_KEY_OBJID,                         /* Catalog columns. */
  UI_KEY_IDINHOSTOBJ,
  UI_KEY_CLUMPSAREA,
  UI_KEY_WEIGHTAREA,
  UI_KEY_GEOX,
  UI_KEY_GEOY,
  UI_KEY_CLUMPSX,
  UI_KEY_CLUMPSY,
  UI_KEY_CLUMPSGEOX,
  UI_KEY_CLUMPSGEOY,
  UI_KEY_W1,
  UI_KEY_W2,
  UI_KEY_GEOW1,
  UI_KEY_GEOW2,
  UI_KEY_CLUMPSW1,
  UI_KEY_CLUMPSW2,
  UI_KEY_CLUMPSGEOW1,
  UI_KEY_CLUMPSGEOW2,
  UI_KEY_CLUMPSBRIGHTNESS,
  UI_KEY_NORIVERBRIGHTNESS,
  UI_KEY_CLUMPSMAGNITUDE,
  UI_KEY_UPPERLIMIT,
  UI_KEY_RIVERAVE,
  UI_KEY_RIVERNUM,
  UI_KEY_SKY,
  UI_KEY_STD,
  UI_KEY_GEOSEMIMAJOR,
  UI_KEY_GEOSEMIMINOR,
  UI_KEY_GEOAXISRATIO,
  UI_KEY_GEOPOSITIONANGLE,
};





void
ui_read_check_inputs_setup(int argc, char *argv[], struct mkcatalogparams *p);

void
ui_free_report(struct mkcatalogparams *p, struct timeval *t1);

#endif
