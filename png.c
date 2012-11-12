/****************************************************************************
    png.c - read and write png images using libpng routines.
    Distributed with Xplanet.
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

#include <png.h>
#include "const.h"

int
read_png(FILE *infile, int *width, int *height, unsigned char **rgba)
{
  int ret = 0;

  png_structp png_ptr;
  png_infop info_ptr;
  png_bytepp row_pointers;

  unsigned char *ptr = NULL;
  png_uint_32 w, h;
  int bit_depth, color_type, interlace_type;
  int i;
  int has_alpha;

  if (!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					 (png_voidp) NULL,
					 (png_error_ptr) NULL,
					 (png_error_ptr) NULL))
      || !(info_ptr = png_create_info_struct(png_ptr)))
    abort();

#if PNG_LIBPNG_VER_MAJOR>1 || (PNG_LIBPNG_VER_MAJOR==1 && PNG_LIBPNG_VER_MINOR>=4)
  if (setjmp(png_jmpbuf((png_ptr))))
#else
  if (setjmp(png_ptr->jmpbuf))
#endif
    goto bugout;

  png_init_io(png_ptr, infile);
  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
	       &interlace_type, (int *) NULL, (int *) NULL);

  /* Prevent against integer overflow */
  if (w >= MAX_DIMENSION || h >= MAX_DIMENSION)
    {
      fprintf(stderr, "Unreasonable dimension found in image file\n");
      goto bugout;
    }

  *width = (int) w;
  *height = (int) h;

  has_alpha = (color_type == PNG_COLOR_TYPE_RGB_ALPHA
	       || color_type == PNG_COLOR_TYPE_GRAY_ALPHA);

  /* Change a paletted/grayscale image to RGB */
  if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
    png_set_expand(png_ptr);

  /* Change a grayscale image to RGB */
  if (color_type == PNG_COLOR_TYPE_GRAY
      || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  /* If the PNG file has 16 bits per channel, strip them down to 8 */
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  /* use 1 byte per pixel */
  png_set_packing(png_ptr);

  if ((row_pointers = malloc(*height * sizeof(png_bytep))) == NULL)
    abort();

  for (i = 0; i < *height; i++)
    if ((row_pointers[i] = malloc((has_alpha ? 4 : 3) * *width)) == NULL)
      abort();

  png_read_image(png_ptr, row_pointers);

  if ((*rgba = malloc(4 * *width * *height)) == NULL)
    abort();

  ptr = *rgba;
  if (has_alpha)
    for (i = 0; i < *height; i++)
      {
	memcpy(ptr, row_pointers[i], 4 * *width);
	ptr += 4 * *width;
      }
  else
    for (i = 0; i < *height; i++)
      {
	int j;
	unsigned char *src = row_pointers[i];

	for (j = 0; j < *width; j++)
	  {
	    memcpy(ptr, src, 3);
	    ptr[3] = 0xFF;
	    src += 3;
	    ptr += 4;
	  }
      }

  ret = 1;			/* data reading is OK */

  for (i = 0; i < *height; i++)
    if (row_pointers[i] != NULL)
      free(row_pointers[i]);

  free(row_pointers);

 bugout:
  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
  return (ret);
}
