/*========================================================*/
/*                                                        */
/*  silly_phot.c        version 1.3.1   2005.01.28        */
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

#include "defs.h"

double silly_phot(IMAGE *im, OBJECT *obj, int hw)
{
        int    i, j, cx, cy, nx;
        double flux;

  nx = im->nx;

  cx = obj->x;
  cy = obj->y;

  flux = 0.0;

  for (i=cx-hw; i<=cx+hw; i++)
  {
    for (j=cy-hw; j<=cy+hw; j++)
      flux += im->pix[i+nx*j] - obj->bg;
  }

  return (flux);
}
