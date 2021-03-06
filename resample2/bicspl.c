/*========================================================*/
/*                                                        */
/*  bicspl.c            version 1.2.5   2005.04.07        */
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
/* Given coefficients of coordinate transformation        */
/* of order ndeg stored in coeffx and coeffy interpolates */
/* image im1(nx, ny) onto the grid of the reference image */
/* and writes new image into im2(nx, ny).                 */
/*                                                        */
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

#include <stdlib.h>
#include "defs.h"

#include "errmess.h"

/* uses algorithms from numerical recipes for spline contruction */
/* and evaluation modified to do things in one go to minimize    */
/* repetitions in prelimiary computations                        */

#include "spline.h"
#include "splint.h"
#include "poly.h"

void bicspl(float *im1, float *im2, int nx, int ny,
            double *coeffx, double *coeffy, int ndeg, RINGPAR par)
{
        int   i, j, n;
        float *xa, *ya, *fa, *f2a, *f2b, *xc, *yc;

  if (nx > ny)  n = nx;
  else          n = ny;

  if (par.verbose > 2) printf("bicspl: n= %d\n", n);

  if (!(fa =(float *)malloc(n*sizeof(float)))) errmess("malloc(fa)");
  if (!(f2a=(float *)malloc(n*sizeof(float)))) errmess("malloc(f2a)");
  if (!(f2b=(float *)malloc(n*sizeof(float)))) errmess("malloc(f2b)");

  if (!(xa =(float *)malloc(nx*sizeof(float)))) errmess("malloc(xa)");
  if (!(ya =(float *)malloc(ny*sizeof(float)))) errmess("malloc(ya)");

  if (!(xc =(float *)malloc(nx*sizeof(float)))) errmess("malloc(xc)");
  if (!(yc =(float *)malloc(ny*sizeof(float)))) errmess("malloc(yc)");

  for (i=0; i<nx; i++)  xa[i] = i;
  for (i=0; i<ny; i++)  ya[i] = i;

/* do spline contructions and evaluations for all rows */

  for (j=0; j<ny; j++)
  {
    for (i=0; i<nx; i++)
    {
      fa[i] = im1[j*nx+i];
      xc[i] = poly(xa[i], ya[j], ndeg, coeffx);
    }

    spline(xa, fa, nx, 1.0e30, 1.0e30, f2a);
    splint(xa, fa, f2a, nx, xc, f2b, par);

    for(i=0; i<nx; i++) im2[j*nx+i] = f2b[i];
  }

/* do spline contructions and evaluations on all columns */
/* of the result of previous step                        */

  for (i=0; i<nx; i++)
  {
    for (j=0; j<ny; j++)
    {
      fa[j] = im2[j*nx+i];
      yc[j] = poly(xa[i], ya[j], ndeg, coeffy);
    }

    spline(ya, fa, ny, 1.0e30, 1.0e30, f2a);
    splint(ya, fa, f2a, ny, yc, f2b, par);

    for (j=0; j<ny; j++) im2[j*nx+i] = f2b[j];
  }

  free(xa);  free(ya);
  free(xc);  free(yc);

  free(fa);
  free(f2a);
  free(f2b);

  if (par.verbose > 3) printf("bicspl: done\n");

  return;
}
