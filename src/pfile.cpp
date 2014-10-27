/*(LGPLv2.1)
---------------------------------------------------------------------------
	pfile.cpp - Portable File Access Toolkit
---------------------------------------------------------------------------
 * Copyright (C) 2002, 2003, 2009 David Olofson
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

#define	DBG(x)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kobolog.h"
#include "pfile.h"


/*----------------------------------------------------------
	Constructor/Destructor
----------------------------------------------------------*/

pfile_t::pfile_t(FILE *file)
{
	f = file;
	_status = 0;
	bufsize = bufused = bufpos = 0;
	buffer = NULL;
}


pfile_t::~pfile_t()
{
	buffer_close();
}


/*----------------------------------------------------------
	Internal buffer control
----------------------------------------------------------*/

int pfile_t::buffer_init()
{
	buffer_close();

	bufsize = 1024;
	buffer = (char *)malloc(bufsize);
	if(!buffer)
		return status(-1);

	bufused = bufpos = 0;
	return _status;
}


int pfile_t::buffer_write()
{
	DBG(log_printf(D3LOG, "pfile_t::buffer_write() %d bytes\n", bufused);)
	if(fwrite(buffer, bufused, 1, f) != 1)
		return status(-1);

	bufused = bufpos = 0;
	return _status;
}


int pfile_t::buffer_read(int len)
{
	buffer_close();

	bufsize = len;
	buffer = (char *)malloc(bufsize);
	if(!buffer)
		return status(-1);

	if(fread(buffer, bufsize, 1, f) != 1)
	{
		buffer_close();
		return status(-1);
	}

	bufused = bufsize;
	bufpos = 0;
	return status(len);
}


void pfile_t::buffer_close()
{
	free(buffer);
	buffer = NULL;
	bufsize = bufused = bufpos = 0;
}


int pfile_t::read(void *data, int len)
{
	if(_status < 0)
		return _status;

	if(buffer)
	{
		if(bufpos + len > bufused)
			return -1;
		memcpy(data, buffer + bufpos, len);
		bufpos += len;
		return len;
	}
	else
	{
		if(fread(data, len, 1, f) != 1)
			return status(-1);
		return len;
	}
}


int pfile_t::write(void *data, int len)
{
	if(_status < 0)
		return _status;
	DBG(log_printf(D3LOG, "pfile_t::write(block of %d bytes)\n", len);)

	if(bufused + len > bufsize)
	{
		int nbs = (bufused + len) * 3 / 2;
		char *nb = (char *)realloc(buffer, nbs);
		if(!nb)
			return -1;
		buffer = nb;
		bufsize = nbs;
	}
	memcpy(buffer + bufused, data, len);
	bufused += len;
	return len;
}



/*----------------------------------------------------------
	Status
----------------------------------------------------------*/

int pfile_t::status()
{
	int os = _status;
	_status = 0;
	return os;
}


int pfile_t::status(int new_status)
{
	if(_status >= 0)
		_status = new_status;
	return _status;
}


/*----------------------------------------------------------
	Little endian read/write API
----------------------------------------------------------*/

int pfile_t::read(unsigned int &x)
{
	unsigned char b[4];
	int result = read(&b, 4);
	if(4 == result)
		x = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
	return result;
}


int pfile_t::read(int &x)
{
	unsigned int ux;
	int result = read(ux);
	if(4 == result)
		x = (int)ux;
	return result;
}


int pfile_t::write(unsigned int x)
{
	DBG(log_printf(D3LOG, "pfile_t::write(int)\n");)
	unsigned char b[4];
	b[0] = x & 0xff;
	b[1] = (x >> 8) & 0xff;
	b[2] = (x >> 16) & 0xff;
	b[3] = (x >> 24) & 0xff;
	return status(write(&b, 4));
}


int pfile_t::write(int x)
{
	return status(write((unsigned int)x));
}


/*----------------------------------------------------------
	Unbuffered Write API
----------------------------------------------------------*/
int pfile_t::write_ub(void *data, int len)
{
	DBG(log_printf(D3LOG, "pfile_t::write_ub(block of %d bytes)\n", len);)
	if(fwrite(data, len, 1, f) != 1)
		return status(-1);

	return _status;
}


int pfile_t::write_ub(unsigned int x)
{
	unsigned char b[4];
	b[0] = x & 0xff;
	b[1] = (x >> 8) & 0xff;
	b[2] = (x >> 16) & 0xff;
	b[3] = (x >> 24) & 0xff;
	return status(write_ub(&b, 4));
}


int pfile_t::write_ub(int x)
{
	return status(write_ub((unsigned int)x));
}


/*----------------------------------------------------------
	RIFF style chunk API
----------------------------------------------------------*/

int pfile_t::chunk_read()
{
	unsigned int size;

	chunk_writing = 0;
	if(read(chunk_id) != 4)
		return _status;
	if(read(size) != 4)
		return _status;

	return buffer_read(size);
}


int pfile_t::chunk_write(int id)
{
	chunk_writing = 1;
	chunk_id = id;
	buffer_init();
	return _status;
}


int pfile_t::chunk_end()
{
	if(chunk_writing)
	{
		write_ub(chunk_id);
		write_ub(bufused);
		buffer_write();
	}
	buffer_close();
	return _status;
}
