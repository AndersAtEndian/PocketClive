/* specplus2.c: Spectrum +2 specific routines
   Copyright (c) 1999-2001 Philip Kendall

   $Id: specplus2.c,v 1.13 2001/08/27 10:58:22 pak21 Exp $

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

#include <config.h>

#include <stdio.h>

#include "spec128.h"
#include "specplus2.h"
#include "spectrum.h"

/* The +2 emulation just uses the 128K routines */

int specplus2_init( machine_info *machine )
{
  int error;

  machine->machine = SPECTRUM_MACHINE_128;

  machine->reset = spec128_reset;

  machine_set_timings( machine, 3.54690e6, 24, 128, 24, 52, 311, 8865);

  machine->ram.read_memory = spec128_readbyte;
  machine->ram.read_screen = spec128_read_screen_memory;
  machine->ram.write_memory = spec128_writebyte;

  error = machine_allocate_roms( machine, 2 );
  if( error ) return error;
  error = machine_read_rom( machine, 0, "plus2-0.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 1, "plus2-1.rom" );
  if( error ) return error;

  machine->peripherals = spec128_peripherals;

  machine->ay.present = 1;

  return 0;

}
