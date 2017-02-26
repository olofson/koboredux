/*(LGPLv2.1)
---------------------------------------------------------------------------
	pfile.h - Portable File Access Toolkit
---------------------------------------------------------------------------
 * Copyright 2002, 2009, 2017 David Olofson
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

#ifndef	_PFILE_H_
#define	_PFILE_H_

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#ifdef WIN32
time_t timegm(struct tm *brokentime);
#endif


    ////////////////////////////////////////////////////////////
   // Note that these classes can keep track of only one
  // chunk at a time! Recursive chunk-inside-chunk is *NOT*
 // supported.
////////////////////////////////////////////////////////////

//Create a little endian "FOURCC" ID code. (That is, 'a' will be
//first if the integer result is stored in little endian format
//in a file.)
#define	MAKE_4CC(a, b, c, d)					\
		(((a)&0xff) | (((b)<<8)&0xff00) |		\
		(((c)<<16)&0xff0000) | (((d)<<24)&0xff000000))


  ////////////////////////////////////////////////////////////
 // pfile_t - Portable File Access Base Class
////////////////////////////////////////////////////////////
// Handles the reading and writing of data from/to file
// via the internal buffer.
//
// Reading can be done either directly from the file, or
// through the internal buffer.
//
// To read through the buffer, the desired number of
// bytes (for example, all bytes of a chunk) must be
// read into the buffer using read_buffer(). Subsequent
// reads will be from the buffer, and end-of-buffer is
// treated like EOF.
//
// Writing is always done to the buffer. To actually
// write data to file, call write_buffer().
//
// IMPORTANT:
//	Written data is *lost* if the pfile object
//	is destroyed before write_buffer() is called!
//
// Reading RIFF style chunks:
//	Use chunk_read() to read the entire chunk into
//	the internal buffer. If this succeeds,
//	chunk_type() and chunk_size() will return
//	information about the chunk, and subsequent
//	read() calls will read from the data section of
//	the chunk. Use chunk_end() to stop reading, and
//	discard the buffer. The next read() will start
//	at the first byte after the end of the chunk.
//
// Writing RIFF style chunks:
//	Use chunk_write() to set the type of the chunk
//	to write. Subsequent write()ing will be done to
//	the internal buffer, which will grow as needed.
//	Use chunk_end() to write the whole chunk, with
//	the correct size and all data. chunk_end() will
//	discard the buffer and return the object to
//	"direct" operation.
//
class pfile_t
{
	FILE	*f;		//Just a plain file handle...
	int	_status;	// < 0 if there was an error
	int	bufsize;	//Actual malloc()ed size of buffer
	int	bufused;	//Current buffer write position
	int	bufpos;		//Current buffer read position
	char	*buffer;	//Read/write buffer
	int	chunk_id;	//Chunk type ID
	int	chunk_writing;	//1 if we're building a chunk for writing

	char	fourccbuf[8];

	// Initialize buffer for writing. Subsequent write() calls
	// will write to the buffer instead of the file.
	int buffer_init();

	// Initialize buffer, and read 'len' bytes into it.
	// Subsequent read() calls will read from the buffer
	// instead of the file.
	int buffer_read(int len);

	//Unbuffered write operations
	int write_ub(const void *data, int len);
	int write_ub(unsigned int x);
	int write_ub(int x);
  public:
	pfile_t(FILE *file);
	virtual ~pfile_t();

	int status();			//Read and reset current status.
	int status(int new_status);	//Set (provided the current status
					// >= 0) and return (new) status

	//These return # of bytes read, or -1 in case of EOF, or an error.
	int read(void *data, int len);
	int read(unsigned int &x);
	int read(int &x);
	int read(int16_t &x);
	int read(int8_t &x);
	int read(struct tm &t);

	void buffer_close();	//Discard the buffer and return to
				//"direct" operation.

	//These return # of bytes written, or -1 in case of EOF, or an error.
	int write(const void *data, int len);
	int write(unsigned int x);
	int write(int x);
	int write(int16_t x);
	int write(int8_t x);
	int write(const struct tm *t);

	int buffer_write();	//Write the whole buffer to the file.
				//This will flush the buffer as well.

	//RIFF style chunk handling
	const char *fourcc2string(unsigned int c);
	int chunk_read();
	int chunk_type()	{ return chunk_id;	}
	int chunk_size()	{ return bufused;	}
	int chunk_write(int id);
	int chunk_end();
};

#endif
