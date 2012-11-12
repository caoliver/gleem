/****************************************************************************
    jpeg.c - read and write jpeg images using libjpeg routines
    Copyright (C) 2002 Hari Nair <hari@alumni.caltech.edu>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>
#include "const.h"

int
read_jpeg(FILE *infile, int *width, int *height, unsigned char **rgba)
{
  int ret = 0;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  /* Prevent against integer overflow */
  if (cinfo.output_width >= MAX_DIMENSION
      || cinfo.output_height >= MAX_DIMENSION)
    {
      fprintf(stderr, "Unreasonable dimension found in image file\n");
      goto bugout;
    }

  *width = cinfo.output_width;
  *height = cinfo.output_height;

  if ((*rgba = malloc(4 * cinfo.output_width * cinfo.output_height)) == NULL)
    abort();

  unsigned char *ptr = *rgba;
  if (cinfo.output_components == 3)
    {
      while (cinfo.output_scanline < cinfo.output_height)
	{
	  unsigned char *src = ptr + cinfo.output_width;

	  jpeg_read_scanlines(&cinfo, &src, 1);
	  for (int i = cinfo.output_width; i--; ptr += 4, src += 3)
	    {
	      memmove(&ptr[0], src, 3);
	      ptr[3] = 0xFF;
	    }
	}
    }
  else if (cinfo.output_components == 1)
    while (cinfo.output_scanline < cinfo.output_height)
      {
	unsigned char *src = ptr + 3 * cinfo.output_width;

	jpeg_read_scanlines(&cinfo, &src, 1);
	for (int i = cinfo.output_width; i--; ptr += 4, src++)
	  {
	    memset(ptr, *src, 3);
	    ptr[3] = 0xFF;
	  }
      }

  jpeg_finish_decompress(&cinfo);

  ret = 1;

 bugout:
  jpeg_destroy_decompress(&cinfo);

  return (ret);
}
