/*(LGPLv2.1)
---------------------------------------------------------------------------
	vidmodes.h - Video Mode Manager for Kobo Deluxe
---------------------------------------------------------------------------
 * Copyright 2007, 2009 David Olofson
 * Copyright 2015 David Olofson (Kobo Redux)
 *
 * This library is free software;  you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation;  either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library  is  distributed  in  the hope that it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KOBO_VIDMODES_H
#define KOBO_VIDMODES_H

#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------------------------------
	Data Types and Structures
-------------------------------------------------------------------------*/

/* Video mode categories */
typedef enum
{
	VMM_ALL =	0xffffffff,
	VMM_DETECT =	0x00000001,	/* Available modes detected by SDL */
	VMM_DESKTOP =	0x00000002,	/* Automatic desktop resolution */
	VMM_4_3 =	0x00000010,	/* 4:3 modes */
	VMM_3_2 =	0x00000020,	/* 3:2 modes */
	VMM_5_4 =	0x00000040,	/* 5:4 modes */
	VMM_16_10 =	0x00000080,	/* 16:10 modes */
	VMM_16_9 =	0x00000100,	/* 16:9 modes */
	VMM_PC =	0x00010000,	/* Standard PC/Mac modes */
	VMM_WPC =	0x00020000,	/* Widescreen PC/Mac modes */
	VMM_TV =	0x00040000,	/* Standard TV modes */
	VMM_WTV =	0x00080000,	/* Widescreen TV modes */
	VMM_CINEMA =	0x00100000,	/* Cinematography resolutions */
	VMM_LORES =	0x10000000	/* Special low resolution modes */
} VMM_Flags;

#define	VMM__RECOMMENDED	(VMM_DESKTOP | VMM_16_9)
#define	VMM__COMMON		(VMM_PC | VMM_16_10 | VMM_16_9)
#define	VMM__WIDESCREEN		(VMM_16_10 | VMM_16_9)
#define	VMM__NONWIDESCREEN	(VMM_TV | VMM_4_3 | VMM_3_2 | VMM_5_4)

/* Special video mode IDs */
typedef enum
{
	VMID_CUSTOM =		0,	/* Custom size/resolution */
	VMID_DESKTOP =		1,	/* Desktop resolution */
	VMID_FULLWINDOW =	2	/* Fullscreen window */
} VMM_ModeID;

typedef struct VMM_Mode VMM_Mode;
struct VMM_Mode
{
	VMM_Mode	*next;		/* Next mode in list */
	char		*name;		/* Name, if any */
	VMM_ModeID	id;		/* Somewhat constant mode ID */
	int		width;		/* Display width in pixels */
	int		height;		/* Display height in pixels */
	int		flags;		/* (WMM_Flags) */
	float		aspect;		/* Pixel aspect ratio */
};


/*-------------------------------------------------------------------------
	Gathering and constructing modes
-------------------------------------------------------------------------*/
int vmm_Init(int show, int hide);
void vmm_Close(void);


/*-------------------------------------------------------------------------
	Scanning
-------------------------------------------------------------------------*/
VMM_Mode *vmm_First(void);
VMM_Mode *vmm_Next(VMM_Mode *current);


/*-------------------------------------------------------------------------
	Indexing
-------------------------------------------------------------------------*/
VMM_Mode *vmm_FindMode(int id);


/*-------------------------------------------------------------------------
	Searching
-------------------------------------------------------------------------*/
/* TODO: VMM_Mode *vmm_Find(int width, int height, int aspect); */

#ifdef __cplusplus
};
#endif

#endif /* KOBO_VIDMODES_H */
