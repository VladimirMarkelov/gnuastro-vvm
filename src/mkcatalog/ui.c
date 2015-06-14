/*********************************************************************
MakeCatalog - Make a catalog from an input and labeled image.
MakeCatalog is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <fitsio.h>

#include "timing.h"	/* Includes time.h and sys/time.h   */
#include "checkset.h"
#include "txtarrayvv.h"
#include "commonargs.h"
#include "configfiles.h"
#include "fitsarrayvv.h"

#include "main.h"

#include "ui.h"		        /* Needs main.h                   */
#include "args.h"	        /* Needs main.h, includes argp.h. */


/* Set the file names of the places where the default parameters are
   put. */
#define CONFIG_FILE SPACK CONF_POSTFIX
#define SYSCONFIG_FILE SYSCONFIG_DIR "/" CONFIG_FILE
#define USERCONFIG_FILEEND USERCONFIG_DIR CONFIG_FILE
#define CURDIRCONFIG_FILE CURDIRCONFIG_DIR CONFIG_FILE










/**************************************************************/
/**************       Options and parameters    ***************/
/**************************************************************/
void
readconfig(char *filename, struct mkcatalogparams *p)
{
  int yes;
  FILE *fp;
  size_t lineno=0, len=200;
  char *line, *name, *value;
  struct uiparams *up=&p->up;
  struct commonparams *cp=&p->cp;
  char key='a';	/* Not used, just a place holder. */

  /* When the file doesn't exist or can't be opened, it is ignored. It
     might be intentional, so there is no error. If a parameter is
     missing, it will be reported after all defaults are read. */
  fp=fopen(filename, "r");
  if (fp==NULL) return;


  /* Allocate some space for `line` with `len` elements so it can
     easily be freed later on. The value of `len` is arbitarary at
     this point, during the run, getline will change it along with the
     pointer to line. */
  errno=0;
  line=malloc(len*sizeof *line);
  if(line==NULL)
    error(EXIT_FAILURE, errno, "ui.c: %lu bytes in readdefaults",
	  len * sizeof *line);

  /* Read the tokens in the file:  */
  while(getline(&line, &len, fp) != -1)
    {
      /* Prepare the "name" and "value" strings, also set lineno. */
      STARTREADINGLINE;




      /* Inputs: */
      if(strcmp(name, "hdu")==0)
        allocatecopyset(value, &cp->hdu, &cp->hduset);
      else if (strcmp(name, "mask")==0)
        allocatecopyset(value, &up->maskname, &up->masknameset);
      else if (strcmp(name, "mhdu")==0)
        allocatecopyset(value, &up->mhdu, &up->mhduset);
      else if (strcmp(name, "objlabs")==0)
        allocatecopyset(value, &up->objlabsname, &up->objlabsnameset);
      else if (strcmp(name, "objhdu")==0)
        allocatecopyset(value, &up->objhdu, &up->objhduset);
      else if (strcmp(name, "clumplabs")==0)
        allocatecopyset(value, &up->clumplabsname, &up->clumplabsnameset);
      else if (strcmp(name, "clumphdu")==0)
        allocatecopyset(value, &up->clumphdu, &up->clumphduset);
      else if (strcmp(name, "sky")==0)
        allocatecopyset(value, &up->skyname, &up->skynameset);
      else if (strcmp(name, "skyhdu")==0)
        allocatecopyset(value, &up->skyhdu, &up->skyhduset);
      else if (strcmp(name, "std")==0)
        allocatecopyset(value, &up->stdname, &up->stdnameset);
      else if (strcmp(name, "stdhdu")==0)
        allocatecopyset(value, &up->stdhdu, &up->stdhduset);
      else if (strcmp(name, "zeropoint")==0)
        {
          if(up->zeropointset) continue;
          anyfloat(value, &p->zeropoint, name, key, SPACK, filename, lineno);
          up->zeropointset=1;
        }


      /* Outputs */
      else if(strcmp(name, "output")==0)
        allocatecopyset(value, &cp->output, &cp->outputset);

      else if(strcmp(name, "intwidth")==0)
        {
          if(up->intwidthset) continue;
          intlzero(value, &p->intwidth, name, key, SPACK, filename, lineno);
          up->intwidthset=1;
        }
      else if(strcmp(name, "floatwidth")==0)
        {
          if(up->floatwidthset) continue;
          intlzero(value, &p->floatwidth, name, key, SPACK, filename, lineno);
          up->floatwidthset=1;
        }
      else if(strcmp(name, "accuwidth")==0)
        {
          if(up->accuwidthset) continue;
          intlzero(value, &p->accuwidth, name, key, SPACK, filename, lineno);
          up->accuwidthset=1;
        }
      else if(strcmp(name, "floatprecision")==0)
        {
          if(up->floatprecisionset) continue;
          intlzero(value, &p->floatprecision, name, key, SPACK,
                   filename, lineno);
          up->floatprecisionset=1;
        }
      else if(strcmp(name, "accuprecision")==0)
        {
          if(up->accuprecisionset) continue;
          intlzero(value, &p->accuprecision, name, key, SPACK,
                   filename, lineno);
          up->accuprecisionset=1;
        }


      /* Catalog columns */
      else if(strcmp(name, "id")==0)
        {
          if(up->idset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATID);
          add_to_sll(&p->clumpcolsll, CATID);
          up->idset=1;
        }
      else if(strcmp(name, "hostobjid")==0)
        {
          if(up->hostobjidset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->clumpcolsll, CATHOSTOBJID);
          up->hostobjidset=1;
        }
      else if(strcmp(name, "idinhostobj")==0)
        {
          if(up->idinhostobjset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->clumpcolsll, CATIDINHOSTOBJ);
          up->idinhostobjset=1;
        }
      else if(strcmp(name, "numclumps")==0)
        {
          if(up->numclumpsset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATNUMCLUMPS);
          up->numclumpsset=1;
        }
      else if(strcmp(name, "area")==0)
        {
          if(up->areaset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATAREA);
          add_to_sll(&p->clumpcolsll, CATAREA);
          up->areaset=1;
        }
      else if(strcmp(name, "clumpsarea")==0)
        {
          if(up->clumpsareaset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSAREA);
          up->clumpsareaset=1;
        }
      else if(strcmp(name, "x")==0)
        {
          if(up->xset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATX);
          add_to_sll(&p->clumpcolsll, CATX);
          up->xset=1;
        }
      else if(strcmp(name, "y")==0)
        {
          if(up->yset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATY);
          add_to_sll(&p->clumpcolsll, CATY);
          up->yset=1;
        }
      else if(strcmp(name, "clumpsx")==0)
        {
          if(up->clumpsxset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSX);
          up->clumpsxset=1;
        }
      else if(strcmp(name, "clumpsy")==0)
        {
          if(up->clumpsyset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSY);
          up->clumpsyset=1;
        }
      else if(strcmp(name, "ra")==0)
        {
          if(up->raset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATRA);
          add_to_sll(&p->clumpcolsll, CATRA);
          up->raset=1;
        }
      else if(strcmp(name, "dec")==0)
        {
          if(up->decset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATDEC);
          add_to_sll(&p->clumpcolsll, CATDEC);
          up->decset=1;
        }
      else if(strcmp(name, "clumpsra")==0)
        {
          if(up->clumpsraset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSRA);
          up->clumpsraset=1;
        }
      else if(strcmp(name, "clumpsdec")==0)
        {
          if(up->clumpsdecset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSDEC);
          up->clumpsdecset=1;
        }
      else if(strcmp(name, "flux")==0)
        {
          if(up->fluxset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATFLUX);
          add_to_sll(&p->clumpcolsll, CATFLUX);
          up->fluxset=1;
        }
      else if(strcmp(name, "clumpsflux")==0)
        {
          if(up->clumpsfluxset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSFLUX);
          p->up.clumpsfluxset=1;
        }
      else if(strcmp(name, "magnitude")==0)
        {
          if(up->magnitudeset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATMAGNITUDE);
          add_to_sll(&p->clumpcolsll, CATMAGNITUDE);
          up->magnitudeset=1;
        }
      else if(strcmp(name, "clumpsmagnitude")==0)
        {
          if(up->clumpsmagnitudeset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATCLUMPSMAGNITUDE);
          up->clumpsmagnitudeset=1;
        }
      else if(strcmp(name, "riverflux")==0)
        {
          if(up->riverfluxset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->clumpcolsll, CATRIVERFLUX);
          up->riverfluxset=1;
        }
      else if(strcmp(name, "rivernum")==0)
        {
          if(up->rivernumset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->clumpcolsll, CATRIVERNUM);
          up->rivernumset=1;
        }
      else if(strcmp(name, "sn")==0)
        {
          if(up->snset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATSN);
          add_to_sll(&p->clumpcolsll, CATSN);
          up->snset=1;
        }
      else if(strcmp(name, "sky")==0)
        {
          if(up->skyset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATSKY);
          add_to_sll(&p->clumpcolsll, CATSKY);
          up->skyset=1;
        }
      else if(strcmp(name, "std")==0)
        {
          if(up->stdset) continue;
          intzeroorone(value, &yes, name, key, SPACK, filename, lineno);
          if(!yes) continue;
          add_to_sll(&p->objcolsll, CATSTD);
          add_to_sll(&p->clumpcolsll, CATSTD);
          up->stdset=1;
        }



      /* Operating modes */
      else if(strcmp(name, "numthreads")==0)
	{
	  if(cp->numthreadsset) continue;
	  sizetlzero(value, &cp->numthreads, name, key, SPACK,
		     filename, lineno);
	  cp->numthreadsset=1;
	}


      else
	error_at_line(EXIT_FAILURE, 0, filename, lineno,
		      "`%s` not recognized.\n", name);
    }

  free(line);
  fclose(fp);
}





void
printvalues(FILE *fp, struct mkcatalogparams *p)
{
  struct uiparams *up=&p->up;
  struct commonparams *cp=&p->cp;

  /* Print all the options that are set. Separate each group with a
     commented line explaining the options in that group. */
  fprintf(fp, "\n# Input image:\n");
  if(cp->hduset)
    PRINTSTINGMAYBEWITHSPACE("hdu", cp->hdu);
  if(up->masknameset)
    PRINTSTINGMAYBEWITHSPACE("mask", up->maskname);
  if(up->mhduset)
    PRINTSTINGMAYBEWITHSPACE("mhdu", up->mhdu);
  if(up->objlabsnameset)
    PRINTSTINGMAYBEWITHSPACE("objlabs", up->objlabsname);
  if(up->objhduset)
    PRINTSTINGMAYBEWITHSPACE("objhdu", up->objhdu);
  if(up->clumplabsnameset)
    PRINTSTINGMAYBEWITHSPACE("clumplabs", up->clumplabsname);
  if(up->clumphduset)
    PRINTSTINGMAYBEWITHSPACE("clumphdu", up->clumphdu);
  if(up->skynameset)
    PRINTSTINGMAYBEWITHSPACE("sky", up->skyname);
  if(up->skyhduset)
    PRINTSTINGMAYBEWITHSPACE("skyhdu", up->skyhdu);
  if(up->stdnameset)
    PRINTSTINGMAYBEWITHSPACE("std", up->stdname);
  if(up->stdhduset)
    PRINTSTINGMAYBEWITHSPACE("stdhdu", up->stdhdu);

  /* Output: */
  fprintf(fp, "\n# Output:\n");
  if(cp->outputset)
    PRINTSTINGMAYBEWITHSPACE("output", cp->output);
  if(up->intwidthset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "intwidth", p->intwidth);
  if(up->floatwidthset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "floatwidth", p->floatwidth);
  if(up->accuwidthset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "accuwidth", p->accuwidth);
  if(up->floatprecisionset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "floatprecision", p->floatprecision);
  if(up->accuprecisionset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "accuprecision", p->accuprecision);

  /* Catalog columns: */
  fprintf(fp, "\n# Catalog columns:\n");
  if(up->idset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "id", 1);
  if(up->hostobjidset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "hostobjid", 1);
  if(up->idinhostobjset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "idinhostobj", 1);
  if(up->numclumpsset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "numclumps", 1);
  if(up->areaset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "area", 1);
  if(up->clumpsareaset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsarea", 1);
  if(up->xset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "x", 1);
  if(up->yset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "y", 1);
  if(up->clumpsxset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsx", 1);
  if(up->clumpsyset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsy", 1);
  if(up->raset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "ra", 1);
  if(up->decset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "dec", 1);
  if(up->clumpsraset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsra", 1);
  if(up->clumpsdecset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsdec", 1);
  if(up->fluxset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "flux", 1);
  if(up->clumpsfluxset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsflux", 1);
  if(up->magnitudeset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "magnitude", 1);
  if(up->clumpsmagnitudeset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "clumpsmagnitude", 1);
  if(up->riverfluxset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "riverflux", 1);
  if(up->rivernumset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "rivernum", 1);
  if(up->snset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "sn", 1);
  if(up->skyset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "sky", 1);
  if(up->stdset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "std", 1);
}






/* Note that numthreads will be used automatically based on the
   configure time. */
void
checkifset(struct mkcatalogparams *p)
{
  struct uiparams *up=&p->up;
  struct commonparams *cp=&p->cp;

  int intro=0;
  if(cp->hduset==0)
    REPORT_NOTSET("hdu");
  if(up->objhduset==0)
    REPORT_NOTSET("objhdu");
  if(up->clumphduset==0)
    REPORT_NOTSET("clumphdu");
  if(up->skyhduset==0)
    REPORT_NOTSET("skyhdu");
  if(up->stdhduset==0)
    REPORT_NOTSET("stdhdu");

  /* Output: */
  if(up->intwidthset==0)
    REPORT_NOTSET("intwidth");
  if(up->floatwidthset==0)
    REPORT_NOTSET("floatwidth");
  if(up->accuwidthset==0)
    REPORT_NOTSET("accuwidth");
  if(up->floatprecisionset==0)
    REPORT_NOTSET("floatprecision");
  if(up->accuprecisionset==0)
    REPORT_NOTSET("accuprecision");

  END_OF_NOTSET_REPORT;
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
void
sanitycheck(struct mkcatalogparams *p)
{
  long tmp;

  /* Set the names of the files. */
  fileorextname(p->up.inputname, p->cp.hdu, p->up.masknameset,
                &p->up.maskname, p->up.mhdu, p->up.mhduset, "mask");
  fileorextname(p->up.inputname, p->cp.hdu, p->up.objlabsnameset,
                &p->up.objlabsname, p->up.objhdu, p->up.objhduset,
                "object labels");
  fileorextname(p->up.inputname, p->cp.hdu, p->up.clumplabsnameset,
                &p->up.clumplabsname, p->up.clumphdu, p->up.clumphduset,
                "clump labels");
  fileorextname(p->up.inputname, p->cp.hdu, p->up.skynameset,
                &p->up.skyname, p->up.skyhdu, p->up.skyhduset,
                "sky value image");
  fileorextname(p->up.inputname, p->cp.hdu, p->up.stdnameset,
                &p->up.stdname, p->up.stdhdu, p->up.stdhduset,
                "sky standard deviation");

  /* Read the number of labels from the two labeled images. */
  readkeyword(p->up.objlabsname, p->up.objhdu, "NOBJS", TLONG, &tmp);
  p->numobjects=tmp;
  readkeyword(p->up.clumplabsname, p->up.clumphdu, "NCLUMPS", TLONG, &tmp);
  p->numclumps=tmp;

  /* Set the output names: */
  if(p->cp.outputset)
    {
      p->ocatname=malloccat(p->cp.output, "_o.txt");
      p->ccatname=malloccat(p->cp.output, "_c.txt");
    }
  else
    {
      automaticoutput(p->up.inputname, "_o.txt", p->cp.removedirinfo,
                      p->cp.dontdelete, &p->ocatname);
      automaticoutput(p->up.inputname, "_c.txt", p->cp.removedirinfo,
                      p->cp.dontdelete, &p->ccatname);
    }
}



















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
void
checksetlong(struct mkcatalogparams *p, char *filename, char *hdu,
             long **array)
{
  int bitpix;
  size_t s0, s1, numblank;

  /* Read the file: */
  filetolong(filename, hdu, array, &bitpix, &numblank, &s0, &s1);

  /* Make sure it has no blank pixels. */
  if(numblank)
    error(EXIT_FAILURE, 0, "The labels images should not have any blank "
          "values. %s (hdu: %s) has %lu blank pixels.", filename,
          hdu, numblank);

  /* Make sure it has an integer type. */
  if(bitpix==FLOAT_IMG || bitpix==DOUBLE_IMG)
    error(EXIT_FAILURE, 0, "The labels image can be any integer type "
          "(BITPIX). However, %s (hdu: %s) is a %s precision floating "
          "point image.", filename, hdu,
          bitpix==FLOAT_IMG ? "single" : "double");

  /* Make sure it is the same size as the input image. */
  if(s0!=p->s0 && s1!=p->s1)
    error(EXIT_FAILURE, 0, "%s (hdu: %s) is %lu x %lu pixels while the "
          "%s (hdu: %s) is %lu x %lu. The images should have the same "
          "size.", filename, hdu, s1, s0, p->up.inputname,
          p->cp.hdu, p->s1, p->s0);
}





void
checksetfloat(struct mkcatalogparams *p, char *filename, char *hdu,
              float **array)
{
  int bitpix;
  size_t s0, s1, numblank;

  /* Read the array: */
  filetofloat(filename, NULL, hdu, NULL, array, &bitpix, &numblank, &s0, &s1);

  /* Make sure it has no blank pixels. */
  if(numblank)
    error(EXIT_FAILURE, 0, "The Sky and Sky standard deviation images "
          "should not have any blank values. %s (hdu: %s) has %lu blank "
          "pixels.", filename, hdu, numblank);

  /* Make sure it has the same size as the image. */
  if(s0!=p->s0 && s1!=p->s1)
    error(EXIT_FAILURE, 0, "%s (hdu: %s) is %lu x %lu pixels while the "
          "%s (hdu: %s) is %lu x %lu. The images should have the same "
          "size.", filename, hdu, s1, s0, p->up.inputname,
          p->cp.hdu, p->s1, p->s0);
}






void
preparearrays(struct mkcatalogparams *p)
{
  int bitpix;
  size_t numblank;


  /* Read the input image: */
  filetofloat(p->up.inputname, p->up.maskname, p->cp.hdu, p->up.mhdu,
              &p->img, &bitpix, &numblank, &p->s0, &p->s1);
  readfitswcs(p->up.inputname, p->cp.hdu, &p->nwcs, &p->wcs);


  /* Read and check the other arrays: */
  checksetlong(p, p->up.objlabsname, p->up.objhdu, &p->objects);
  checksetlong(p, p->up.clumplabsname, p->up.clumphdu, &p->clumps);
  checksetfloat(p, p->up.skyname, p->up.skyhdu, &p->sky);
  checksetfloat(p, p->up.stdname, p->up.stdhdu, &p->std);


  /* Prepare the columns: */
  slltoarray(p->objcolsll, &p->objcols, &p->objncols);
  slltoarray(p->clumpcolsll, &p->clumpcols, &p->clumpncols);


  /* Allocate the catalog arrays: */
  if(p->objncols>0 && p->numobjects>0)
    {
      errno=0; p->objcat=malloc(p->objncols*p->numobjects*sizeof *p->objcat);
      if(p->objcat==NULL)
        error(EXIT_FAILURE, errno, "%lu bytes for p->objcat in "
              "preprarearrays (ui.c)",
              p->objncols*p->numobjects*sizeof *p->objcat);
    }
  else p->objcat=NULL;
  if(p->clumpncols>0 && p->numclumps>0)
    {
      errno=0;
      p->clumpcat=malloc(p->clumpncols*p->numclumps*sizeof *p->clumpcat);
      if(p->clumpcat==NULL)
        error(EXIT_FAILURE, errno, "%lu bytes for p->clumpcat in "
              "preprarearrays (ui.c)",
              p->clumpncols*p->numclumps*sizeof *p->clumpcat);
    }
  else p->clumpcat=NULL;


  /* Clean up */
  freesll(p->objcolsll);
  freesll(p->clumpcolsll);
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/
void
setparams(int argc, char *argv[], struct mkcatalogparams *p)
{
  struct commonparams *cp=&p->cp;

  /* Set the non-zero initial values, the structure was initialized to
     have a zero value for all elements. */
  cp->spack         = SPACK;
  cp->verb          = 1;
  cp->numthreads    = DP_NUMTHREADS;
  cp->removedirinfo = 1;

  /* Read the arguments. */
  errno=0;
  if(argp_parse(&thisargp, argc, argv, 0, 0, p))
    error(EXIT_FAILURE, errno, "Parsing arguments");


  /* Add the user default values and save them if asked. */
  CHECKSETCONFIG;

  /* Check if all the required parameters are set. */
  checkifset(p);

  /* Print the values for each parameter. */
  if(cp->printparams)
    REPORT_PARAMETERS_SET;

  /* Do a sanity check. */
  sanitycheck(p);

  /* Make the array of input images. */
  preparearrays(p);

  /* Everything is ready, notify the user of the program starting. */
  if(cp->verb)
    {
      printf(SPACK_NAME" started on %s", ctime(&p->rawtime));
      printf("  - Input   %s (hdu: %s)\n", p->up.inputname, p->cp.hdu);
      if(p->up.masknameset)
        printf("  - Mask   %s (hdu: %s)\n", p->up.maskname, p->up.mhdu);
      printf("  - Objects %s (hdu: %s)\n", p->up.objlabsname,
             p->up.objhdu);
      printf("  - Clumps  %s (hdu: %s)\n", p->up.clumplabsname,
             p->up.clumphdu);
      printf("  - Sky     %s (hdu: %s)\n", p->up.skyname, p->up.skyhdu);
      printf("  - Sky STD %s (hdu: %s)\n", p->up.stdname, p->up.stdhdu);
    }
}




















/**************************************************************/
/************      Free allocated, report         *************/
/**************************************************************/
void
freeandreport(struct mkcatalogparams *p, struct timeval *t1)
{
  /* Free all the allocated spaces: */
  free(p->sky);
  free(p->std);
  free(p->objcat);
  free(p->cp.hdu);
  free(p->clumps);
  free(p->objects);
  free(p->objcols);
  free(p->clumpcat);
  free(p->ocatname);
  free(p->ccatname);
  free(p->clumpcols);
  free(p->up.objhdu);
  free(p->cp.output);
  free(p->up.skyhdu);
  free(p->up.stdhdu);
  free(p->up.clumphdu);
  if(p->up.mhduset) free(p->up.mhdu);
  if(p->wcs) wcsvfree(&p->nwcs, &p->wcs);
  if(p->up.skynameset) free(p->up.skyname);
  if(p->up.stdnameset) free(p->up.stdname);
  if(p->up.masknameset) free(p->up.maskname);
  if(p->up.objlabsnameset) free(p->up.objlabsname);
  if(p->up.clumplabsnameset) free(p->up.clumplabsname);

  /* Print the final message. */
  reporttiming(t1, SPACK_NAME" finished in", 0);
}
