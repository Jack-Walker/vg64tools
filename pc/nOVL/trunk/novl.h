/*
*   nOVL -- novl.h
*   Copyright (c) 2009  Marshall B. Rogers [mbr@64.vg]
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public
*   Licence along with this program; if not, write to the Free
*   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
*   Boston, MA  02110-1301, USA
*/
#ifndef __NOVL_H__
#define __NOVL_H__

#include <stdio.h>
#include <stdint.h>
#include <elf.h>
#include <libelf.h>
#include <gelf.h>
#include <glib.h>


/* ----------------------------------------------
   Constants
   ---------------------------------------------- */

#define NOVL_VERSION_MAJOR  1
#define NOVL_VERSION_MINOR  0
#define NOVL_VERSION_MICRO  0
#define NOVL_VERSION_STR    "v1.0.0"
#define NOVL_NAME           "nOVL"
#define NOVL_FULL_NAME      "Nintendo Overlay Tool"
#define NOVL_AUTHOR         "Marshall R. <mbr@64.vg>"

/* Comment out for no debug text */
#define NOVL_DEBUG

/* Uncomment this to disable colors in output
#define NOVL_NO_COLORS */


#include "mesg.h"
#include "func.h"


/* ----------------------------------------------
   Data types
   ---------------------------------------------- */

typedef void (*novlDoReloc)
(
    uint32_t *, /* Pointer to instruction in memory */
    uint32_t,   /* Address                          */
    int,        /* Type of relocation to perform    */
    int         /* Amount to translate addresses    */        
);

/* Settings structure */
struct novl_settings_t
{
    int mode;
    int no_colors;
    int human_readable;
    uint32_t ovl_base;
    
};


/* ----------------------------------------------
   Variable declarations
   ---------------------------------------------- */

extern char * novl_str_reloc_types[];
extern novlDoReloc novl_reloc_jt[];
extern struct novl_settings_t settings;


/* ----------------------------------------------
   Function declarations
   ---------------------------------------------- */

extern char * novl_str_reloc ( int );
extern void novl_conv ( uint32_t, char *, char * );
extern uint32_t novl_reloc_mk ( int, int, int );

/* Relocation functions */
extern int novl_reloc_do ( uint32_t *, uint32_t, int, int );

   

#endif /* __NOVL_H__ */

