/****************************************************************************
    read.c - read and write jpeg and png images routines.

    Copyright (C) 2012 Christopher Oliver <current-input-port@gmail.com>
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
#include <png.h>
#include "read.h"
#include "util.h"


int
read_png(FILE *infile, const char *filename, int *width, int *height,
	 unsigned char **rgb, unsigned char **alpha)
{
  int ret = 0;

  png_structp png_ptr;
  png_infop info_ptr;
  volatile png_bytepp row_pointers = NULL;
  png_uint_32 w, h;
  int bit_depth, color_type, interlace_type;

  if (!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					 (png_voidp) NULL,
					 (png_error_ptr) NULL,
					 (png_error_ptr) NULL))
      || !(info_ptr = png_create_info_struct(png_ptr)))
    out_of_memory();

#if PNG_LIBPNG_VER_MAJOR>1 || (PNG_LIBPNG_VER_MAJOR==1 && PNG_LIBPNG_VER_MINOR>=4)
  if (setjmp(png_jmpbuf((png_ptr))))
#else
  if (setjmp(png_ptr->jmpbuf))
#endif
    goto bugout;

  png_init_io(png_ptr, infile);
  png_read_info(png_ptr, info_ptr);

  if (!png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
		    &interlace_type, (int *) NULL, (int *) NULL))
    goto bugout;

  /* Protect against integer overflow */
  if (w >= MAX_DIMENSION || h >= MAX_DIMENSION)
    {
      fprintf(stderr, "Unreasonable dimension found in image file %s\n",
	      filename);
      goto bugout;
    }

  *width = (int) w;
  *height = (int) h;

  *alpha = NULL;
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA
      || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    *alpha = xmalloc(*width * *height);

  if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
    png_set_expand(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY
      || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  png_set_packing(png_ptr);

  row_pointers = xcalloc(*height, sizeof(png_bytep));

  for (int i = 0; i < *height; i++)
    row_pointers[i] = xmalloc((*alpha ? 4 : 3) * *width);

  png_read_image(png_ptr, row_pointers);

  *rgb = xmalloc(3 * *width * *height);

  unsigned char *ptr = *rgb;
  unsigned char *alpha_ptr = *alpha;
  if (*alpha == NULL)
    for (int i = 0; i < *height; i++)
      {
	memcpy(ptr, row_pointers[i], 3 * *width);
	ptr += 3 * *width;
      }
  else
    for (int i = 0; i < *height; i++)
      {
	unsigned char *src = row_pointers[i];

	for (int j = *width; j--;)
	  {
	    *ptr++ = *src++;
	    *ptr++ = *src++;
	    *ptr++ = *src++;
	    *alpha_ptr++ = *src++;
	  }
      }

  ret = 1;

 bugout:
  if (row_pointers)
    for (int i = 0; i < *height; i++)
      free(row_pointers[i]);
  free(row_pointers);

  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
  return (ret);
}


static jmp_buf jpeg_panic;

void jpeg_panic_handler(j_common_ptr cinfo)
{
  longjmp(jpeg_panic, 1);
}

int
read_jpeg(FILE *infile, const char *filename, int *width, int *height,
	  unsigned char **rgb, unsigned char **alpha)
{
  int ret = 0;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  volatile unsigned char *new_rgb = NULL;

  *alpha = NULL;

  cinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = jpeg_panic_handler;
  if (setjmp(jpeg_panic))
    {
      free((unsigned char *)new_rgb);
      goto bugout;
    }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  /* Prevent against integer overflow */
  if (cinfo.output_width >= MAX_DIMENSION
      || cinfo.output_height >= MAX_DIMENSION)
    {
      fprintf(stderr, "Unreasonable dimension found in image file %s\n",
	      filename);
      goto bugout;
    }

  *width = cinfo.output_width;
  *height = cinfo.output_height;

  new_rgb = xmalloc(3 * cinfo.output_width * cinfo.output_height);

  unsigned char *ptr = (unsigned char *)new_rgb;
  if (cinfo.output_components == 3)
    while (cinfo.output_scanline < cinfo.output_height)
      {
	jpeg_read_scanlines(&cinfo, &ptr, 1);
	ptr += 3 * cinfo.output_width;
      }
  else if (cinfo.output_components == 1)
    while (cinfo.output_scanline < cinfo.output_height)
      {
	unsigned char *src = ptr + 2 * cinfo.output_width;

	jpeg_read_scanlines(&cinfo, &src, 1);
	for (int i = cinfo.output_width; i--; ptr += 3, src++)
	  memset(ptr, *src, 3);
      }

  jpeg_finish_decompress(&cinfo);
  *rgb = (unsigned char *)new_rgb;
  ret = 1;
 
 bugout:

  jpeg_destroy_decompress(&cinfo);

  return (ret);
}
