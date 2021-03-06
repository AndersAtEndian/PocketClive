/* specplus3.h: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2001 Philip Kendall

   $Id: specplus3.h,v 1.12 2001/08/13 21:49:40 pak21 Exp $

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

#ifndef FUSE_SPECPLUS3_H
#define FUSE_SPECPLUS3_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

BYTE specplus3_readbyte(WORD address);
BYTE specplus3_read_screen_memory(WORD offset);
void specplus3_writebyte(WORD address, BYTE b);
int specplus3_init( machine_info *machine );
int specplus3_reset(void);

void specplus3_memoryport_write(WORD port, BYTE b);

#endif			/* #ifndef FUSE_SPECPLUS3_H */
