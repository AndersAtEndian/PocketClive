/* settings.c: Handling configuration settings
   Copyright (c) 2001 Philip Kendall

   $Id: settings.c,v 1.5 2001/12/15 14:22:53 pak21 Exp $

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

#include "settings.h"

/* The current settings of options, etc */
settings_info settings_current;

/* Called on emulator startup */
int settings_init( void )
{
  int error;

  error = settings_defaults( &settings_current );
  if( error ) return error;

  return 0;
}

/* Fill the settings structure with sensible defaults */
int settings_defaults( settings_info *settings )
{
  settings->issue2 = 0;
  settings->joy_kempston = 0;
  settings->tape_traps = 1;
  settings->stereo_ay = 0;

  return 0;
}

/* Copy one settings object to another */
int settings_copy( settings_info *dest, settings_info *src )
{
  dest->issue2       = src->issue2;
  dest->joy_kempston = src->joy_kempston;
  dest->tape_traps   = src->tape_traps;
  dest->stereo_ay    = src->stereo_ay;

  return 0;
}

    
