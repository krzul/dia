/*========================================================*/
/*                                                        */
/*  aga.c               version 1.5.0   2005.05.06        */
/*                                                        */
/*  Original source code:                                 */
/*  Copyright (C) 2005 by Przemek Wozniak                 */
/*  wozniak@lanl.gov                                      */
/*                                                        */
/*  Modifications:                                        */
/*  Copyright (C) 2005 by Wojtek Pych, CAMK PAN           */
/*  pych@camk.edu.pl                                      */
/*                                                        */
/*  Written for GNU project C and C++ Compiler            */
/*                                                        */
/*========================================================*/
/*  NOTE:                                                 */
/*  INPUT frames:                                         */
/*  - bad_pix_mask: unsigned short (short, BITPIX=16)     */
/*  - reference:    float (BITPIX=-32)                    */
/*========================================================*/

/***************************************************************************/
/*                                                                         */
/*   This program is free software; you can redistribute it and/or modify  */
/*   it under the terms of the GNU General Public License as published by  */
/*   the Free Software Foundation; either version 2 of the License, or     */
/*   (at your option) any later version.                                   */
/*                                                                         */
/*   This program is distributed in the hope that it will be useful,       */
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*   GNU General Public License for more details.                          */
/*                                                                         */
/*   You should have received a copy of the GNU General Public License     */
/*   along with this program; if not, write to the                         */
/*   Free Software Foundation, Inc.,                                       */
/*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             */
/*                                                                         */
/***************************************************************************/

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#include "defs.h"
#include "funcs.h"

#include "errmess.h"
#include "pfitsio1.h"

void usage();

/*--------------------------------------------------------*/
void usage()
{
  printf("\n\t USAGE: aga  parameter_file instrument_file ");
  printf(" mask_image reference_image image_names_file\n\n");
  exit(1);
}
/*--------------------------------------------------------*/
int read_list(PARAMS par, char *iname,
              char ***inpfname, char ***outfname, char ***kerfname)
{
        char  **linpfname,
              **loutfname,
              **lkerfname,
              *kerptr,
              line[MAX_FNAME];
        int   i;
        FILE  *inf;

  if (!(linpfname=(char **)calloc(1, sizeof(char *))))
    errmess("calloc(linpfname)");
  if (!(loutfname=(char **)calloc(1, sizeof(char *))))
    errmess("calloc(loutfname)");
  if (!(lkerfname=(char **)calloc(1, sizeof(char *))))
    errmess("calloc(lkerfname)");

  if (!(inf=fopen(iname, "r"))) errmess(iname);

  for (i=0; !feof(inf); i++)
  {
    fgets(line, MAX_FNAME, inf);
    if (feof(inf)) break;

    if (!(linpfname=(char **)realloc(linpfname, (i+1)*sizeof(char *))))
      errmess("realloc(linpfname)");
    if (!(loutfname=(char **)realloc(loutfname, (i+1)*sizeof(char *))))
      errmess("realloc(loutfname)");
    if (!(lkerfname=(char **)realloc(lkerfname, (i+1)*sizeof(char *))))
      errmess("realloc(lkerfname)");

    if (par.verbose > 5) printf("line: %s\n", line);
    sscanf(line, "%s", line);
    if (par.verbose > 5) printf("line: %s\n", line);

    if (!(linpfname[i]=(char *)calloc(strlen(line)+1, sizeof(char))))
      errmess("calloc(linpfname[i])");
    strcpy(linpfname[i], line);

    if (!(loutfname[i]=(char *)calloc(strlen(linpfname[i])+3, sizeof(char))))
      errmess("calloc(loutfname[i])");
    sprintf(loutfname[i], "s_%s", linpfname[i]);

    if (!(lkerfname[i]=(char *)calloc(strlen(linpfname[i])+1, sizeof(char))))
      errmess("calloc(lkerfnamefname[i])");
    strcpy(lkerfname[i], linpfname[i]);
    if (!(kerptr=strstr(lkerfname[i], par.fits_ext)))
    {
      printf("aga: strstr(lkerfname[i], par.fits_ext) failure\n");
      printf("lkerfname[%d]: %s\n", i, lkerfname[i]);
      printf("par.fits_ext: %s\n", par.fits_ext);
      exit(6);
    }

    if (kerptr == lkerfname[i])
    {
      printf("aga: '%s' not found in '%s'\n", par.fits_ext, lkerfname[i]);
      printf("lkerfname[%d]: %s\n", i, lkerfname[i]);
      printf("par.fits_ext: %s\n", par.fits_ext);
      exit(7);
    }

    sprintf(kerptr, "ker");

    if (par.verbose > 4)
    {
      printf("linpfname[%d]= %s\n", i, linpfname[i]);
      printf("loutfname[%d]= %s\n", i, loutfname[i]);
      printf("lkerfname[%d]= %s\n", i, lkerfname[i]);
    }
  }

  fclose(inf);

  *inpfname=linpfname;
  *outfname=loutfname;
  *kerfname=lkerfname;

  return(i);
}
/*--------------------------------------------------------*/
int main(int argc, char *argv[])
{
        char            *parfname,
                        *instrname,
                        *maskname,
                        *reffname,
                        *imgfname,
                        **inpfname,
                        **outfname,
                        **kerfname;
        unsigned short  *mask0, *mask1, *flag0, *flag1;
        int             *indx,
                        nsec_x, nsec_y, isec_x, isec_y, x_off, y_off, mode,
                        deg[MAX_NCOMP], i, k, nclip, npix, nim, ntab, nkeep;
        long int        imglen, buflen, veclen, intlen, matlen, usblen,
                        lomlen, lovlen, tablen, domlen, srtlen,
                        *headlen, mheadlen, rheadlen;
        float           *im, *imref, *imdif,
                        sig[MAX_NCOMP], *sort_buf;
        double          **vecs, **mat, *vec, **tab00, **wxy,
                        *ker_norm, chi2_n, good_area, total_area,
                        parity;
        DOM_TABS        *domp;
        PARAMS          par;

/**************************************************************************/

  imref = imdif = NULL;
  mask0 = mask1 = NULL;
  flag0 = flag1 = NULL;
  indx  = NULL;
  vec   = NULL;
  vecs  = mat   = NULL;
  tab00 = wxy   = NULL;
  sort_buf = NULL;

  good_area = 0.0;
  domp = 0;

/*** argument handling ***/
  if (argc != 6) usage();

  parfname = argv[1];
  instrname= argv[2];
  maskname = argv[3];
  reffname = argv[4];
  imgfname = argv[5];

/**************************************************************************/
/*** get parameters ***/
/**************************************************************************/

  par.deg = deg;
  par.sig = sig;

  get_params(parfname, instrname, &par);
  if (par.verbose > 3)
  {
    printf("parfname:   %s\n", parfname);
    printf("instrname:  %s\n", instrname);
    printf("maskname:   %s\n", maskname);
    printf("reffname:   %s\n", reffname);
    printf("imgfname:   %s\n", imgfname);
    printf("--------------\n");
  }

/*** get the list of input, output and kernel files ***/

  if (par.verbose > 2) printf("Reading '%s'\n", imgfname);
  nim=read_list(par, imgfname, &inpfname, &outfname, &kerfname);
  if (par.verbose > 2) printf("%d file-names read read\n", nim);

  for (i=1; i<par.ncomp; i++)
  {
    par.deg[i] = par.deg[i-1] + par.deg_inc;
    par.sig[i] = par.sig[i-1] * par.sig_inc;
  }

  par.nx += 2*par.kerhw;
  par.ny += 2*par.kerhw;

  par.sdeg = par.wdeg;
  if (par.bdeg > par.wdeg)  par.sdeg = par.bdeg;

  par.nwxy  = (par.wdeg+1)*(par.wdeg+2)/2;
  par.nspat = (par.sdeg+1)*(par.sdeg+2)/2;
  par.nbkg  = (par.bdeg+1)*(par.bdeg+2)/2;

  par.nvecs = par.nbkg;
  for (i=0; i<par.ncomp; i++) par.nvecs += (par.deg[i]+1)*(par.deg[i]+2)/2;
  par.ntot = par.nvecs + (par.nvecs - par.nbkg - 1)*(par.nwxy - 1);

  par.ndom = par.ndom_x*par.ndom_y;

  ntab = par.nvecs-par.nbkg+par.nspat;

  if (par.verbose) printf("\t %d image(s) to process\n\n", nim);

/**************************************************************************/
/*** get memory ***/
/**************************************************************************/

  imglen = par.nx*par.ny*sizeof(float);
  buflen = par.nx*par.ny*sizeof(double);
  matlen = par.ntot*sizeof(double *);
  veclen = par.ntot*sizeof(double);
  usblen = par.nx*par.ny*sizeof(unsigned short);
  tablen = ntab*sizeof(double *);
  lomlen = par.nvecs*sizeof(double *);
  lovlen = par.nvecs*sizeof(double);
  domlen = par.ndom*sizeof(DOM_TABS);
  srtlen = par.ndom*sizeof(float);
  intlen = par.ndom*sizeof(int);

  if (par.ntot > par.ndom)  intlen = par.ntot*sizeof(int);

  if (!(im    =   (float *)malloc(imglen))) errmess("malloc(im)");
  if (!(imref =   (float *)malloc(imglen))) errmess("malloc(imref)");
  if (!(imdif =   (float *)malloc(imglen))) errmess("malloc(imdif)");
  if (!(mat   = (double **)malloc(matlen))) errmess("malloc(mat)");
  if (!(tab00 = (double **)malloc(tablen))) errmess("malloc(tab00)");
  if (!(vecs  = (double **)malloc(tablen))) errmess("malloc(vecs)");
  if (!(wxy   = (double **)malloc(tablen))) errmess("malloc(wxy)");

  if (!(domp     = (DOM_TABS *)malloc(domlen))) errmess("malloc(DOM_TABS)");
  if (!(sort_buf =    (float *)malloc(srtlen))) errmess("malloc(sort_buf)");

  for (i=0; i<ntab; i++)
    if (!(tab00[i] = (double *)malloc(buflen))) errmess("malloc(tab00[i])");

  for (k=0; k<par.ndom; k++)
  {
    if (!(domp[k].mat0 = (double **)malloc(lomlen)))
      errmess("malloc(domp[k].mat0)");
    if (!(domp[k].mat1 = (double **)malloc(lomlen)))
      errmess("malloc(domp[k].mat1)");
    if (!(domp[k].mat  = (double **)malloc(lomlen)))
      errmess("malloc(domp[k].mat)");
    if (!(domp[k].vec0 = (double *)malloc(lovlen)))
      errmess("malloc(domp[k].vec0)");
    if (!(domp[k].vec  = (double *)malloc(lovlen)))
      errmess("malloc(domp[k].vec)");

    for (i=0; i<par.nvecs; i++)
    {
      if (!(domp[k].mat0[i] = (double *)malloc(lovlen)))
        errmess("malloc(domp[k].mat0[i])");
      if (!(domp[k].mat1[i] = (double *)malloc(lovlen)))
        errmess("malloc(domp[k].mat1[i])");
      if (!(domp[k].mat[i]  = (double *)malloc(lovlen)))
        errmess("malloc(domp[k].mat[i])");
    }
  }

/* mat is indexed from 1 to n for use with numerical recipes ! */

  for (i=0; i<par.ntot; i++)
  {
    if (!(mat[i] = (double *)malloc(veclen))) errmess("malloc(mat[i])");
    mat[i]--;
  }
  mat--;

  if (!(vec  = (double *)malloc(veclen))) errmess("malloc(vec)");
  if (!(indx =    (int *)malloc(intlen))) errmess("malloc(indx)");

  if (!(mask0 = (unsigned short *)malloc(usblen))) errmess("malloc(mask0)");
  if (!(mask1 = (unsigned short *)malloc(usblen))) errmess("malloc(mask1)");
  if (!(flag0 = (unsigned short *)malloc(usblen))) errmess("malloc(flag0)");
  if (!(flag1 = (unsigned short *)malloc(usblen))) errmess("malloc(flag1)");

  if (!(headlen=(long *)calloc(nim, sizeof(long))))
    errmess("calloc(headlen)");
  if (!(ker_norm=(double *)calloc(nim, sizeof(double))))
    errmess("calloc(ker_norm)");
/**************************************************************************/
/**************************************************************************/

/* get information about header sizes */
  mheadlen=get_headlen(maskname);
  rheadlen=get_headlen(reffname);

  init_difimages(inpfname, outfname, headlen, nim, par);

/**************************************************************************/
  if (par.verbose > 4)
  {
    printf("par.nx0= %d  par.ny0 = %d\n", par.nx0, par.ny0);
    printf("par.kerhw= %d\n", par.kerhw);
    printf("par.nx= %d  par.ny = %d\n", par.nx, par.ny);
  }
  nsec_x = (par.nx0 - 2*par.kerhw)/(par.nx - 2*par.kerhw);
  nsec_y = (par.ny0 - 2*par.kerhw)/(par.ny - 2*par.kerhw);

/**************************************************************************/
/***   main loop over sections of each image  ***/
/**************************************************************************/

  if (par.verbose > 4)
    printf("main loop over sections: nsec_x= %d  nsec_y= %d\n", nsec_x, nsec_y);

  for (isec_x=0; isec_x<nsec_x; isec_x++)
  {
    for (isec_y=0; isec_y<nsec_y; isec_y++)
    {
      y_off = isec_y*(par.ny - 2*par.kerhw);
      x_off = isec_x*(par.nx - 2*par.kerhw);

      mode  = (int)( isec_x  ||  isec_y );

      if (par.verbose > 4)
        printf("isec_x= %d   isec_y= %d   mode= %d\n", isec_x, isec_y, mode);

      read_sector(maskname, mheadlen, par.nx0, par.ny0, x_off, y_off,
                  (char *)mask0, sizeof(unsigned short), par.nx, par.ny);
      read_sector(reffname, rheadlen, par.nx0, par.ny0, x_off, y_off,
                  (char *)imref, sizeof(float), par.nx, par.ny);

      make_vectors(imref, tab00, vecs, wxy, par);
      mask_badpix(imref, mask0, flag0, 0, par);
      get_domains(imref, mask0, domp, par, &par.ndom);
      make_domains(imref, mask0, vecs, domp, par);

      total_area  = (2*par.domhw + 1);
      total_area *= total_area*par.ndom;

/**********************************************/
/***  loop over images for a given section  ***/
/**********************************************/

      for (i=0; i<nim; i++)
      {
        if (par.verbose >= VERB_MED)
          printf("\nSection [%d,%d] of image %d : %s\n",
                 isec_x+1, isec_y+1, i+1, inpfname[i]);

        read_sector(inpfname[i], headlen[i], par.nx0, par.ny0, x_off, y_off,
                    (char *)im, sizeof(float), par.nx, par.ny);
        mask_badpix(im, mask1, flag1, 1, par);
        npix = clean_domains(im, imref, mask0, mask1, vecs, domp, par);

/***     sigma clipping     ***/
        chi2_n = BIG_FLOAT;

        if (par.verbose >= VERB_MED)
        {
          printf("\nLocal sigma clipping of domains:\n");
          printf(" iteration     chi2/dof     N(clip)      N(fit)\n");
        }

        for (k=0; k<=par.n_iter+1; k++)
        {
          good_area = npix/total_area;
          if (good_area < par.min_area) break;

          clone_domains(domp, par.ndom, par.nvecs);

/**************************/
          if (k > 0)
          {
            nclip = local_clip(im, imref, mask1, vecs, wxy, vec, domp, par,
                               &chi2_n, &npix);
            if (par.verbose >= VERB_MED)
              printf("%6d\t%14.4f\t%9d\t%7d\n",
                      k-1, par.gain*chi2_n, nclip, npix);
          }
/**************************/

          expand_matrix(mat, vec, wxy, domp, par);

          ludcmp(mat, par.ntot, indx-1, &parity);
          lubksb(mat, par.ntot, indx-1, vec-1);
        }

        if (par.verbose >= VERB_MED)
        {
          printf("\nSigma clipping of domain distribution:\n");
          printf("iteration   median chi2/dof      sigma    N(good)\n");
        }

        nkeep = par.ndom;
        for (k=1; k<=par.n_iter_dom && nkeep>=par.min_nkeep; k++)
          nkeep = clip_domains(domp, sort_buf, indx, k, &chi2_n, par);

/***   sigma clipping ends  ***/
        chi2_n *= par.gain;

/***   final solution   ***/
        if (good_area < par.min_area)
        {
          printf("\nFailed for section [%d,%d] of image %d : %s\n",
                 isec_x+1, isec_y+1, i+1, inpfname[i]);
          printf("*** [ good_area= %f < %f ] ***\n", good_area, par.min_area);
        }
        else if (chi2_n > par.max_chi2)
        {
          printf("\nFailed for section [%d,%d] of image %d : %s\n",
                 isec_x+1, isec_y+1, i+1, inpfname[i]);
          printf("*** [ chi2_n= %f > %f ] ***\n", chi2_n, par.max_chi2);
        }
        else if (nkeep < par.min_nkeep)
        {
          printf("\nFailed for section [%d,%d] of image %d : %s\n",
                 isec_x+1, isec_y+1, i+1, inpfname[i]);
          printf("*** [ nkeep= %d < %d ] ***\n", nkeep, par.min_nkeep);
        }
        else
        {
          expand_matrix(mat, vec, wxy, domp, par);

          ludcmp(mat, par.ntot, indx-1, &parity);
          lubksb(mat, par.ntot, indx-1, vec-1);

          spatial_convolve(im, imdif, vecs, wxy, vec, par);

/***   output   ***/
          ker_norm[i] = vec[par.nbkg];
          apply_norm(imdif, flag0, flag1, ker_norm[i], par.nx, par.ny,
                     par.bad_value);
          write_sector(outfname[i], headlen[i], par.nx0, par.ny0, x_off, y_off,
                       (char *)imdif, sizeof(float), par.nx, par.ny, par.kerhw);
        }

        if (par.verbose)
          write_kernel(kerfname[i], vec, nsec_x, nsec_y, mode, par, &chi2_n);
      }

/**********************************************/
/***  loop over images ends                 ***/
/**********************************************/
    }
  }

/**********************************************/
/***  loop over sections ends               ***/
/**********************************************/

  if (par.verbose >= VERB_HIGH)
  {
    printf("\nKernel norms:\n\n");
    for (i=0; i<nim; i++)
      printf("%s \t %8.5f\n", kerfname[i], ker_norm[i]);
    printf("\n");
  }

  for (i=0; i<nim; i++)
  {
    free(inpfname[i]);
    free(outfname[i]);
    free(kerfname[i]);
  }

  free(inpfname);
  free(outfname);
  free(kerfname);

  free(headlen);
  free(ker_norm);

  return(0);
}
/*** END ***/
