/* tape.c: tape handling routines
   Copyright (c) 1999-2001 Philip Kendall

   $Id: tape.c,v 1.38 2001/12/14 15:44:01 pak21 Exp $

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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "event.h"
#include "fuse.h"
#include "libspectrum/tape.h"
#include "machine.h"
#include "settings.h"
#include "sound.h"
#include "spectrum.h"
#include "tape.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current tape */
libspectrum_tape tape;

/* Is the emulated tape deck playing? */
int tape_playing;

/* Is there a high input to the EAR socket? */
int tape_microphone;

#define ERROR_MESSAGE_MAX_LENGTH 1024

/* Function prototypes */

static int trap_load_block( libspectrum_tape_rom_block *block );
int trap_check_rom( void );

/* Function defintions */

int tape_init( void )
{
  tape.blocks = NULL;
  tape_playing = 0;
  tape_microphone = 0;
  sound_beeper( 1, tape_microphone );
  return 0;
}

int tape_open( const char *filename )
{
  struct stat file_info; int fd; unsigned char *buffer;

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  /* If we already have a tape file open, close it */
  if( tape.blocks ) {
    error = tape_close();
    if( error ) return error;
  }

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: couldn't open `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", fuse_progname, filename );
    perror( error_message );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  /* First, try opening the file as a .tzx file; if we get back an
     error saying it didn't have the .tzx signature, then try opening
     it as a .tap file */
  error = libspectrum_tzx_create( &tape, buffer, file_info.st_size );
  if( error == LIBSPECTRUM_ERROR_SIGNATURE ) {
    error = libspectrum_tap_create( &tape, buffer, file_info.st_size );
  }

  if( error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf( stderr,
	     "%s: error reading `%s': %s\n",
	     fuse_progname, filename, libspectrum_error_message(error) );
      munmap( buffer, file_info.st_size );
      return error;
  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  /* And the tape is stopped */
  if( tape_playing ) tape_stop();

  return 0;

}

/* Close the active tape file */
int tape_close( void )
{
  int error;

  error = libspectrum_tape_free( &tape );
  if( error ) return error;

  return 0;
}

/* Write the current in-memory tape file out to disk */
int tape_write( const char* filename )
{
  libspectrum_byte *buffer; size_t length;
  FILE *f;

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  length = 0;
  error = libspectrum_tzx_write( &tape, &buffer, &length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf(stderr, "%s: error during libspectrum_tzx_write: %s\n",
	    fuse_progname, libspectrum_error_message(error) );
    return error;
  }

  f=fopen( filename, "wb" );
  if(!f) { 
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error opening `%s'", fuse_progname, filename );
    perror( error_message );
    free( buffer );
    return 1;
  }

  if( fwrite( buffer, 1, length, f ) != length ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error writing to `%s'", fuse_progname, filename );
    perror( error_message );
    fclose(f);
    free( buffer );
    return 1;
  }

  free( buffer );

  if( fclose( f ) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error closing `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  return 0;

}

/* Load the next tape block into memory; returns 0 if a block was
   loaded (even if it had an tape loading error or equivalent) or
   non-zero if there was an error at the emulator level, or tape traps
   are not active */
int tape_load_trap( void )
{
  libspectrum_tape_block *current_block;
  libspectrum_tape_rom_block *rom_block;

  GSList *block_ptr; libspectrum_tape_block *next_block;

  int error;

  /* Do nothing if tape traps aren't active, or the tape is already playing */
  if( ! settings_current.tape_traps || tape_playing ) return 2;

  /* Do nothing if we're not in the correct ROM */
  if( ! trap_check_rom() ) return 3;

  /* Return with error if no tape file loaded */
  if( tape.blocks == NULL ) return 1;

  current_block = (libspectrum_tape_block*)(tape.current_block->data);

  block_ptr = tape.current_block->next;
  /* Loop if we hit the end of the tape */
  if( block_ptr == NULL ) {
    block_ptr = tape.blocks;
  }

  next_block = (libspectrum_tape_block*)(block_ptr->data);
  libspectrum_tape_init_block( next_block );

  /* If this block isn't a ROM loader, start it playing
     Then return with `error' so that we actually do whichever instruction
     it was that caused the trap to hit */
  if( current_block->type != LIBSPECTRUM_TAPE_BLOCK_ROM ) {

    error = tape_play();
    if( error ) return 3;

    return -1;
  }

  rom_block = &(current_block->types.rom);

  /* All returns made via the RET at #05E2 */
  PC = 0x05e2;

  error = trap_load_block( rom_block );
  if( error ) return error;

  /* Peek at the next block. If it's a ROM block, just move along and
     return */
  if( next_block->type == LIBSPECTRUM_TAPE_BLOCK_ROM ) {
    tape.current_block = block_ptr;
    return 0;
  }

  /* If the next block isn't a ROM block, set ourselves up such that the
     next thing to occur is the pause at the end of the current block */
  current_block->types.rom.state = LIBSPECTRUM_TAPE_STATE_PAUSE;

  /* And start the tape playing */
  error = tape_play();
  /* On error, still return without error as we did sucessfully do
     the tape trap, and so don't want to do the trigger instruction */
  if( error ) return 0;

  return 0;

}

static int trap_load_block( libspectrum_tape_rom_block *block )
{
  libspectrum_byte parity, *data;
  int loading, i;

  /* If the block's too short, give up and go home (with carry reset
     to indicate error */
  if( block->length < DE ) { 
    F = ( F & ~FLAG_C );
    return 0;
  }

  data = block->data;
  parity = *data;

  /* If the flag byte (stored in A') does not match, reset carry and return */
  if( *data++ != A_ ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Loading or verifying determined by the carry flag of F' */
  loading = ( F_ & FLAG_C );

  if( loading ) {
    for( i=0; i<DE; i++ ) {
      writebyte( IX+i, *data );
      parity ^= *data++;
    }
  } else {		/* verifying */
    for( i=0; i<DE; i++) {
      parity ^= *data;
      if( *data++ != readbyte(IX+i) ) {
	F = ( F & ~FLAG_C );
	return 0;
      }
    }
  }

  /* If the parity byte does not match, reset carry and return */
  if( *data++ != parity ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Else return with carry set */
  F |= FLAG_C;

  return 0;
}

/* Append to the current tape file in memory; returns 0 if a block was
   saved or non-zero if there was an error at the emulator level, or tape
   traps are not active */
int tape_save_trap( void )
{
  libspectrum_tape_block *block;
  libspectrum_tape_rom_block *rom_block;

  libspectrum_byte parity;

  int i;

  /* Do nothing if tape traps aren't active */
  if( ! settings_current.tape_traps ) return 2;

  /* Check we're in the right ROM */
  if( ! trap_check_rom() ) return 3;

  /* Get a new block to store this data in */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) return 1;

  /* This is a ROM block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_ROM;
  rom_block = &(block->types.rom);

  /* The +2 here is for the flag and parity bytes */
  rom_block->length = DE + 2;

  rom_block->data =
    (libspectrum_byte*)malloc( rom_block->length * sizeof(libspectrum_byte) );
  if( rom_block->data == NULL ) {
    free( block );
    return 1;
  }

  /* First, store the flag byte (and initialise the parity counter) */
  rom_block->data[0] = parity = A;

  /* then the main body of the data, counting parity along the way */
  for( i=0; i<DE; i++) {
    libspectrum_byte b = readbyte( IX+i );
    parity ^= b;
    rom_block->data[i+1] = b;
  }

  /* And finally the parity byte */
  rom_block->data[ DE+1 ] = parity;

  /* Give a 1 second pause after this block */
  rom_block->pause = 1000;

  /* Add the block to the current tape file */
  tape.blocks = g_slist_append( tape.blocks, (gpointer)block );

  /* If we previously didn't have a tape loaded ( implied by
     tape.current_block == NULL ), set up so that we point to the
     start of the tape */
  if( tape.current_block == NULL ) {
    tape.current_block = tape.blocks;
    libspectrum_tape_init_block((libspectrum_tape_block*)tape.blocks->data);
  }

  /* And then return via the RET at #053E */
  PC = 0x053e;

  return 0;

}

/* Check whether we're actually in the right ROM when a tape trap hit */
int trap_check_rom( void )
{
  switch( machine_current->machine ) {
  case SPECTRUM_MACHINE_48:
    return 1;		/* Always OK here */

  case SPECTRUM_MACHINE_128:
  case SPECTRUM_MACHINE_PLUS2:
    /* OK if we're in ROM 1 */
    return( machine_current->ram.current_rom == 1 );

  case SPECTRUM_MACHINE_PLUS3:
    /* OK if we're not in a 64Kb RAM configuration and we're in
       ROM 3 */
    return( ! machine_current->ram.special &&
	    machine_current->ram.current_rom == 3 );

  }

  fprintf( stderr, "Impossible machine type %d", machine_current->machine );
  abort();

}

int tape_play( void )
{
  libspectrum_tape_block* block;

  int error;

  if( tape.blocks == NULL ) return 1;
  
  block = (libspectrum_tape_block*)(tape.current_block->data);

  /* If tape traps are active and the current block is a ROM block, do
     nothing, _unless_ the ROM block has already reached the pause at
     its end which (hopefully) means we're in the magic state involving
     starting slow loading whilst tape traps are active */
  if( settings_current.tape_traps &&
      block->type == LIBSPECTRUM_TAPE_BLOCK_ROM &&
      block->types.rom.state != LIBSPECTRUM_TAPE_STATE_PAUSE )
    return 0;

  /* Otherwise, start the tape going */
  tape_playing = 1;
  tape_microphone = 0;
  sound_beeper( 1, tape_microphone );

  error = tape_next_edge(); if( error ) return error;

  return 0;
}

int tape_stop( void )
{
  tape_playing = 0;
  return 0;
}

int tape_next_edge( void )
{
  int error; libspectrum_error libspec_error;

  libspectrum_dword edge_tstates;
  int flags;

  /* If the tape's not playing, just return */
  if( ! tape_playing ) return 0;

  /* Get the time until the next edge */
  libspec_error = libspectrum_tape_get_next_edge( &tape, &edge_tstates,
						  &flags );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return libspec_error;

  /* Invert the microphone state */
  if( edge_tstates || ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) ) {
    tape_microphone = !tape_microphone;
    sound_beeper( 1, tape_microphone );
  }

  /* If we've been requested to stop the tape, do so and then
     return without stacking another edge */
  if( ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) ||
      ( ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP48 ) && 
	machine_current->machine == SPECTRUM_MACHINE_48 )
    ) {
    error = tape_stop(); if( error ) return error;
    return 0;
  }

  /* If that was the end of a block, tape traps are active _and_ the
     new block is a ROM loader, return without putting another event
     into the queue */
  if( ( flags & LIBSPECTRUM_TAPE_FLAGS_BLOCK ) &&
      settings_current.tape_traps &&
      ((libspectrum_tape_block*)(tape.current_block->data))->type ==
        LIBSPECTRUM_TAPE_BLOCK_ROM
    ) {
    tape_playing = 0;
    return 0;
  }

  /* Otherwise, put this into the event queue which will cause the
     next block to become active when it occurs */
  error = event_add( tstates + edge_tstates, EVENT_TYPE_EDGE );
  if( error ) return error;

  return 0;
}


/* Rewind the current tape file to the start */
int tape_rewind( void )
{
  if( tape.blocks == NULL ) return 1;

  tape.current_block = tape.blocks;

  return 0;
}
