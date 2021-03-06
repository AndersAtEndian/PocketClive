/* uidisplay.h: Low-level display routines
   Copyright (c) 2000-2001 Philip Kendall

   $Id: uidisplay.h,v 1.12 2001/08/13 21:49:40 pak21 Exp $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_UIDISPLAY_H
#define FUSE_UIDISPLAY_H

int uidisplay_init(int width, int height);

void uidisplay_putpixel(int x,int y,int colour);
void uidisplay_line(int y);
void uidisplay_lines( int start, int end );
void uidisplay_set_border(int line, int pixel_from, int pixel_to,
			  int colour);

int uidisplay_end(void);

#endif			/* #ifndef FUSE_UIDISPLAY_H */
