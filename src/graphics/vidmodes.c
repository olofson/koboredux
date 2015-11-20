/*(LGPLv2.1)
---------------------------------------------------------------------------
	vidmodes.c - Video Mode Manager for Kobo Deluxe
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

#include "vidmodes.h"
#include <stdlib.h>
#include <string.h>

typedef struct VMM_IMode
{
	int		flags, width, height, id;
	const char	*name;
} VMM_IMode;

static VMM_IMode modetab[] =
{
	// Special modes
	{0,			0,	0,	VMID_CUSTOM,	"Custom"},
	{VMM_DESKTOP,		0,	0,	VMID_DESKTOP,	"Desktop"},
	{VMM_DESKTOP,		0,	0,	VMID_FULLWINDOW,"Fullwindow"},

	// 4:3 modes
	{VMM_4_3 | VMM_PC,	320,	200,	0x04300,	"CGA"},
	{VMM_4_3 | VMM_PC,	320,	240,	0x04301,	"QVGA"},
	{VMM_4_3,		320,	256,	0x04302,	"Amiga LoRes"},
	{VMM_4_3 | VMM_LORES,	400,	300,	0x04310,	NULL},
	{VMM_4_3 | VMM_LORES,	512,	384,	0x04320,	NULL},
	{VMM_4_3 | VMM_PC,	640,	480,	0x04330,	"VGA"},
	{VMM_4_3,		640,	512,	0x04331,	"Amiga HiRes"},
	{VMM_4_3 | VMM_TV,	768,	576,	0x04340,	"PAL"},
	{VMM_4_3 | VMM_PC,	800,	600,	0x04350,	"SVGA"},
	{VMM_4_3 | VMM_LORES,	960,	720,	0x04360,	NULL},
	{VMM_4_3 | VMM_PC,	1024,	768,	0x04370,	"XGA"},
	{VMM_4_3 | VMM_PC,	1152,	864,	0x04380,	"XGA+"},
	{VMM_4_3 | VMM_PC,	1280,	960,	0x04390,	"SXGA−/UVGA"},
	{VMM_4_3 | VMM_PC,	1400,	1050,	0x043a0,	"SXGA+"},
	{VMM_4_3 | VMM_PC,	1600,	1200,	0x043b0,	"UXGA"},
	{VMM_4_3 | VMM_PC,	1920,	1440,	0x043c0,	NULL},
	{VMM_4_3 | VMM_PC,	2048,	1536,	0x043d0,	"QXGA"},

	// 3:2 modes
	{VMM_3_2 | VMM_LORES,	360,	240,	0x03200,	"WQVGA"},
	{VMM_3_2 | VMM_LORES,	720,	480,	0x03240,	"NTSC"},
	{VMM_3_2,		1152,	768,	0x03280,	"WXGA"},
	{VMM_3_2,		1280,	864,	0x03290,	NULL},
	{VMM_3_2,		1440,	960,	0x032a0,	"FWXGA+"},

	// 5:4 modes
	{VMM_5_4 | VMM_LORES,	320,	256,	0x05400,	"Amiga LoRes"},
	{VMM_5_4 | VMM_LORES,	640,	512,	0x05430,	"Amiga HiRes"},
	{VMM_5_4 | VMM_PC,	1280,	1024,	0x05490,	"SXGA"},
	{VMM_5_4,		2560,	2048,	0x054e0,	"QSXGA"},

	// 16:10 modes
	{VMM_16_10 | VMM_LORES,	320,	200,	0x10a00,	"CGA"},
	{VMM_16_10 | VMM_PC,	640,	400,	0x10a30,	NULL},
	{VMM_16_10 | VMM_LORES,	840,	525,	0x10a50,	NULL},
	{VMM_16_10 | VMM_PC,	1280,	800,	0x10a90,	"WXGA"},
	{VMM_16_10 | VMM_PC,	1680,	1050,	0x10ab0,	"WSXGA+"},
	{VMM_16_10 | VMM_PC,	1920,	1200,	0x10ac0,	"WUXGA"},
	{VMM_16_10 | VMM_PC,	2560,	1600,	0x10ae0,	"WQXGA"},

	// 16:9 modes
	{VMM_16_9 | VMM_LORES,	480,	270,	0x10910,	NULL},
	{VMM_16_9,		640,	360,	0x10920,	"nHD"},
	{VMM_16_9 | VMM_LORES,	678,	384,	0x10930,	NULL},
	{VMM_16_9 | VMM_TV,	854,	480,	0x10950,	"WVGA"},
	{VMM_16_9 | VMM_LORES,	960,	540,	0x10960,	"qHD"},
	{VMM_16_9,		1280,	720,	0x10990,	"HD 720"},
	{VMM_16_9 | VMM_TV,	1356,	768,	0x109a0,	"Flat TV"},
	{VMM_16_9 | VMM_TV,	1920,	1080,	0x109c0,	"HD 1080"},
	{VMM_16_9 | VMM_PC,	2560,	1440,	0x109e0,	"QHD"},
	{VMM_16_9 | VMM_PC,	3840,	2160,	0x109f0,	"UHD"},
	{VMM_16_9 | VMM_PC,	5120,	2880,	0x109f1,	"UHD+"},
	{VMM_16_9 | VMM_CINEMA,	7680,	4320,	0x109f2,	"FUHD"},

	// Cinema modes
	{VMM_CINEMA,		2048,	1152,	0x10b10,	"2K"},
	{VMM_CINEMA | VMM_PC,	4096,	2160,	0x10ba0,	"4K"},

	// End of table
	{0, -1}
};


/*-------------------------------------------------------------------------
	Gathering and constructing modes
-------------------------------------------------------------------------*/

static VMM_Mode *vmm_modes = NULL;

int vmm_Init(int show, int hide)
{
	int i;
	VMM_Mode *lastm = NULL;
	vmm_Close();

/*
TODO: Include detected SDL modes, if requested.
 */

	for(i = 0; modetab[i].width != -1; ++i)
	{
		VMM_Mode *m;
		double nominal_aspect;
		VMM_IMode *im = &modetab[i];
		if(im->flags & hide)
			continue;
		if(!(im->flags & show))
			continue;
		m = calloc(1, sizeof(VMM_Mode));
		if(!m)
			return -1;	/* Out of memory! */
		m->id = im->id;
		m->width = im->width;
		m->height = im->height;
		m->flags = im->flags;
		if(im->flags & VMM_3_2)
			nominal_aspect = 3.0f / 2.0f;
		else if(im->flags & VMM_5_4)
			nominal_aspect = 5.0f / 4.0f;
		else if(im->flags & VMM_16_10)
			nominal_aspect = 16.0f / 10.0f;
		else if(im->flags & VMM_16_9)
			nominal_aspect = 16.0f / 9.0f;
		else
			nominal_aspect = 4.0f / 3.0f;
		if(im->width)
			m->aspect = nominal_aspect / im->width * im->height;
		else
			m->aspect = 1.0f;
		if(im->name)
			m->name = strdup(im->name);
		if(lastm)
			lastm->next = m;
		else
			vmm_modes = m;
		lastm = m;
	}

	return 0;
}


void vmm_Close(void)
{
	VMM_Mode *m = vmm_First();
	while(m)
	{
		VMM_Mode *dm = m;
		m = vmm_Next(m);
		free(dm->name);
		free(dm);
	}
	vmm_modes = NULL;
}


/*-------------------------------------------------------------------------
	Scanning
-------------------------------------------------------------------------*/

VMM_Mode *vmm_First(void)
{
	return vmm_modes;
}


VMM_Mode *vmm_Next(VMM_Mode *current)
{
	return current->next;
}


/*-------------------------------------------------------------------------
	Indexing
-------------------------------------------------------------------------*/

VMM_Mode *vmm_FindMode(int id)
{
	VMM_Mode *m = vmm_First();
	while(m)
	{
		if(m->id == id)
			return m;
		m = vmm_Next(m);
	}
	return NULL;
}
