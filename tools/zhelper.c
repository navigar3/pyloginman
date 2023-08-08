/* Taken from zpipe.c, see below... */

/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler */

/* Version history:
   1.0  30 Oct 2004  First version
   1.1   8 Nov 2004  Add void casting for unused return values
                     Use switch statement for inflate() return values
   1.2   9 Nov 2004  Add assertions to document zlib guarantees
   1.3   6 Apr 2005  Remove incorrect assertion in inf()
   1.4  11 Dec 2005  Add hack to avoid MSDOS end-of-line conversions
                     Avoid some compiler warnings for input and output buffers
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(fdes) setmode(fdes, O_BINARY)
#else
#  define SET_BINARY_MODE(fdes)
#endif

#define CHUNK 262144
//#define CHUNK 524288

#include "cobj.h"

#define _HASHTABLE_H_PRIVATE
#include "hashtable.h"

void * z_prepare_buffer(unsigned int bufflen)
{
  size_t bl = (size_t)bufflen;
  size_t * pbl = (bl) ? &bl : NULL; 
  return new(buffer, pbl);
}

void * z_get_buffer_data(void * buff)
{
  return CALL(((buffer *)buff), get_map); 
}

unsigned int z_get_buffer_data_sz(void * buff)
{
  return CALL(((buffer *)buff), get_mapsz); 
}

int z_destroy_buffer(void * buff)
{
  CALL(((buffer *)buff), destroy);
  
  return 0; 
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int z_inflate(unsigned int slen, char *src, void * dst)
{
    int ret;
    int read_bytes;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    unsigned int s_offs = 0;
    unsigned int offs = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate src buffer ends. */
    do {
        strm.next_in = src + s_offs;
        
        if (s_offs+CHUNK < slen)
        {
          read_bytes = CHUNK;
          s_offs += CHUNK;
        }
        else
        {
          read_bytes = slen - s_offs;
          s_offs = slen;
        }
        
        strm.avail_in = read_bytes;
        if (strm.avail_in == 0)
            break;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            CALL(((buffer *)dst), write, have, out);
            offs += have;
            
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int z_deflate(unsigned int slen, char * src, void * dst)
{
    int ret, flush;
    unsigned int s_offs = 0;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    
    int level = Z_BEST_COMPRESSION;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.next_in = src + s_offs;
        
        if (s_offs+CHUNK < slen)
        {
          strm.avail_in = CHUNK;
          flush = Z_NO_FLUSH;
          s_offs += CHUNK;
        }
        else
        {
          strm.avail_in = slen - s_offs;
          flush = Z_FINISH;
          s_offs = slen;
        }

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            
            CALL(((buffer *)dst), write, have, out);
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}


/* avoid end-of-line conversions */
/*
    SET_BINARY_MODE(0);
    SET_BINARY_MODE(1);

*/
