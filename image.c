#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xmu/WinUtil.h>
#include "image.h"
#include "util.h"
#include "read.h"

#define NUM_COLORS 256

int read_image(const char *filename, struct image *image)
{
  char buf[4];
  unsigned char *ubuf = (unsigned char *) buf;
  int success = 0;
  FILE *file;

  if (image->rgb_data)
    free(image->rgb_data);
  if (image->alpha_data)
    free(image->alpha_data);

  if ((file = fopen(filename, "rb")) == NULL)
    return 0;

  if (fread(buf, 1, 4, file) == 4)
    {
      rewind(file);
      if ((ubuf[0] == 0x89) && !strncmp("PNG", buf + 1, 3))
	success = read_png(file, filename,
			   &image->width, &image->height,
			   &image->rgb_data, &image->alpha_data);
      else if ((ubuf[0] == 0xff) && (ubuf[1] == 0xd8))
	success =
	  read_jpeg(file, filename,
		    &image->width, &image->height,
		    &image->rgb_data, &image->alpha_data);
      else
	fprintf(stderr, "Unknown image format\n");
      
      image->area = image->width * image->height;
    }

  fclose(file);
  return (success == 1);
}


void free_image_buffers(struct image *image)
{
  free(image->rgb_data);
  image->rgb_data = NULL;
  free(image->alpha_data);
  image->alpha_data = NULL;
}


static void computeShift(unsigned long mask,
			 unsigned char *left_shift,
			 unsigned char *right_shift)
{
  int left = 0, right = 8;
  if (mask != 0)
    {
      while ((mask & 0x01) == 0)
	{
	  left++;
	  mask >>= 1;
	}
      while ((mask & 0x01) == 1)
	{
	  right--;
	  mask >>= 1;
	}
    }
  *left_shift = left;
  *right_shift = right;
}


Pixmap imageToPixmap(Display * dpy, struct image *image, int scr, Window win)
{
  const int depth = DefaultDepth(dpy, scr);
  Visual *visual = DefaultVisual(dpy, scr);
  Colormap colormap = DefaultColormap(dpy, scr);

  int width = image->width, height = image->height, area = image->area;
  Pixmap pixmap = XCreatePixmap(dpy, win, width, height, depth);

  char *pixmap_data = NULL;
  switch (depth)
    {
    case 32:
    case 24:
      pixmap_data = xmalloc(4 * area);
      break;
    case 16:
    case 15:
      pixmap_data = xmalloc(2 * area);
      break;
    case 8:
      pixmap_data = xmalloc(area);
      break;
    default:
      break;
    }

  XImage *ximage = XCreateImage(dpy, visual, depth, ZPixmap, 0,
				pixmap_data, width, height,
				8, 0);

  int entries;
  XVisualInfo v_template;
  v_template.visualid = XVisualIDFromVisual(visual);
  XVisualInfo *visual_info = XGetVisualInfo(dpy, VisualIDMask,
					    &v_template, &entries);

  unsigned char *rgb = image->rgb_data;
  switch (visual_info->class)
    {
    case PseudoColor:
      {
	XColor *colors;
	int *closest_color;

	colors = xmalloc(sizeof(XColor) * NUM_COLORS);
	closest_color = xmalloc(sizeof(int) * NUM_COLORS);

	for (unsigned int i = 0; i < NUM_COLORS; i++)
	  colors[i].pixel = i;
	XQueryColors(dpy, colormap, colors, NUM_COLORS);

	for (int i = 0; i < NUM_COLORS; i++)
	  {
	    // find the closest color in the colormap
	    double distance, distance_squared, min_distance = 0;
	    for (int ii = 0; ii < NUM_COLORS; ii++)
	      {
		distance = colors[ii].red - ((i & 0xe0) << 8);
		distance_squared = distance * distance;
		distance = colors[ii].green - ((i & 0x1c) << 11);
		distance_squared += distance * distance;
		distance = colors[ii].blue - ((i & 0x03) << 14);
		distance_squared += distance * distance;

		if ((ii == 0) || (distance_squared < min_distance))
		  {
		    min_distance = distance_squared;
		    closest_color[i] = ii;
		  }
	      }
	  }

	for (int j = 0; j < height; j++)
	  for (int i = 0; i < width; i++)
	    {
	      unsigned short pixel_ix;
	      
	      pixel_ix = (*rgb++ & 0xe0);
	      pixel_ix |= (*rgb++ & 0xe0) >> 3;
	      pixel_ix |= *rgb++ >> 6;
	      
	      XPutPixel(ximage, i, j, colors[closest_color[pixel_ix]].pixel);
	  }
	free(colors);
	free(closest_color);
      }
      break;
    case TrueColor:
      {
	unsigned char red_left_shift;
	unsigned char red_right_shift;
	unsigned char green_left_shift;
	unsigned char green_right_shift;
	unsigned char blue_left_shift;
	unsigned char blue_right_shift;

	computeShift(visual_info->red_mask, &red_left_shift,
		     &red_right_shift);
	computeShift(visual_info->green_mask, &green_left_shift,
		     &green_right_shift);
	computeShift(visual_info->blue_mask, &blue_left_shift,
		     &blue_right_shift);

	unsigned long pixel;
	unsigned long red, green, blue;
	for (int j = 0; j < height; j++)
	  {
	    for (int i = 0; i < width; i++)
	      {
		red = *rgb++ >> red_right_shift;
		green = *rgb++ >> green_right_shift;
		blue = *rgb++ >> blue_right_shift;

		pixel = (((red << red_left_shift) & visual_info->red_mask)
			 | ((green << green_left_shift)
			    & visual_info->green_mask)
			 | ((blue << blue_left_shift)
			    & visual_info->blue_mask));

		XPutPixel(ximage, i, j, pixel);
	      }
	  }
      }
      break;
    default:
      {
	fprintf(stderr, "Unsupported visual for image\n");
	return (pixmap);
      }
    }

  GC gc = XCreateGC(dpy, win, 0, NULL);
  XPutImage(dpy, pixmap, gc, ximage, 0, 0, 0, 0, width, height);
  XFreeGC(dpy, gc);
  XFree(visual_info);
  free(pixmap_data);
  // Set ximage data to NULL since pixmap data was deallocated above
  ximage->data = NULL;
  XDestroyImage(ximage);

  return (pixmap);
}


// Declare constants for a Bresenham stepper from (0,0) to (X,Y).
// X and Y are assumed to be positive integers.
#define BSTEP_DEFN_CONST(TAG, X, Y)                             \
  int TAG##_y = (Y);                                            \
  int TAG##_x = (X);                                            \
  int TAG##_frac_slope = TAG##_y % TAG##_x;                     \
  int TAG##_int_slope = (TAG##_y - TAG##_frac_slope) / TAG##_x; \
  int TAG##_no_jump = 2 * TAG##_frac_slope;                     \
  int TAG##_jump = TAG##_no_jump - 2 * TAG##_x;

// Declare state variables for a given Bresenham stepper.
#define BSTEP_INIT_STATE(TAG, OUT)		\
  int OUT = 0;					\
  int TAG##_##OUT##_err = TAG##_jump + TAG##_x;

// Compute next Bresenham step.
#define BSTEP_NEXT(TAG, OUT)			\
  if (TAG##_##OUT##_err > 0) {			\
    OUT++;					\
    TAG##_##OUT##_err += TAG##_jump;		\
  }						\
  else						\
    TAG##_##OUT##_err += TAG##_no_jump;		\
    OUT += TAG##_int_slope;


void resize_background(struct image *image, const int w, const int h) 
{
  int width = image->width;
  int height = image->height;
  if (width==w && height==h)
    return;

  int new_area = w * h;

  unsigned char *rgb_data = image->rgb_data;
  unsigned char *new_rgb = (unsigned char *) xmalloc(3 * new_area);

  unsigned char *rgb = new_rgb;

  BSTEP_DEFN_CONST(JLOOP, h - 1, 256 * height - 256);
  BSTEP_DEFN_CONST(ILOOP, w - 1, 256 * width - 256);

  BSTEP_INIT_STATE(JLOOP, y);
  for (int j = 0; j < h; j++)
    {
      BSTEP_INIT_STATE(ILOOP, x);
      for (int i = 0; i < w; i++, rgb += 3)
	{
	  int ix0 = x >> 8;
	  int t = x & 255;
	  int ixb = (ix0 + 1 >= width) ? 0 : 3;

	  int iy0 = y >> 8;
	  int u = 255 - (y & 255);
	  int iyb = (iy0 + 1 >= height) ? 0 : 3 * width;

	  unsigned int weight[4];
	  weight[1] = (t * u) >> 8;
	  weight[0] = u - weight[1];
	  weight[3] = t - weight[1];
	  weight[2] = 255 + weight[1] - t - u;

	  unsigned char *pixels[4];
	  pixels[0] = rgb_data + 3 * (iy0 * width + ix0);
	  pixels[1] = pixels[0] + ixb;
	  pixels[2] = pixels[0] + iyb;
	  pixels[3] = pixels[2] + ixb;

	  for (int j = 0; j < 3; j++) {
	    unsigned int sum = 0;

	    for (int i = 0; i < 4; i++)
	      sum += (unsigned int) (weight[i] * pixels[i][j]);
	    rgb[j] = (sum + 255)>>8;
	  }

	  BSTEP_NEXT(ILOOP, x);
	}
      BSTEP_NEXT(JLOOP, y);
    }

  free(image->alpha_data);
  image->alpha_data = NULL;

  free(rgb_data);
  image->rgb_data = new_rgb;

  image->width = w;
  image->height = h;

  image->area = new_area;
}

#define MAX(A,B) (A < B) ? B : A;
#define MIN(A,B) (A > B) ? B : A;

void merge_with_background(struct image *panel, struct image *background,
			   int xoffset, int yoffset)
{
  if (panel->alpha_data)
    {
      int h_start = MAX(0, xoffset);
      int h_end = MIN(background->width, panel->width + xoffset);
      int v_start = MAX(0, yoffset);
      int v_end = MIN(background->height, panel->height + yoffset);
      
      if (h_end <= h_start || v_end <= v_start)
	return;

      int num_cols = h_end - h_start;
      int rows = v_end - v_start;

      int panel_start =
	panel->width * (v_start - yoffset) + h_start - xoffset;
      unsigned char *rowalpha =	panel->alpha_data + panel_start;
      unsigned char *rowdst = panel->rgb_data + 3 * panel_start;
      unsigned char *rowsrc =
	background->rgb_data + 3 * (background->width * v_start + h_start);

      while (rows--)
	{
	  unsigned char *dst = rowdst, *src = rowsrc, *alpha = rowalpha;
	  int cols = num_cols;

	  while (cols--)
	    {
	      for (int k = 3; k--; dst++)
		*dst = (*dst * *alpha + *src++ * (255 - *alpha)) >> 8;
	      alpha++;
	    }

	  rowdst += 3 * panel->width;
	  rowalpha += panel->width;
	  rowsrc += 3 * background->width;
	}
    }
}


void frame_background(struct image *image,
		      unsigned int width, unsigned int height,
		      int xoffset, int yoffset, XftColor *color)
{
  int area = width * height;
  unsigned char
    r = color->color.red >> 8,
    g = color->color.green >> 8,
    b = color->color.blue >> 8;
  unsigned char *new_rgb = xmalloc(3 * area);

  unsigned char *dst = new_rgb;
  for (int i = area; i--;)
    {
      *dst++ = r;
      *dst++ = g;
      *dst++ = b;
    }

  int h_start = MAX(0, xoffset);
  int h_end = MIN(width, image->width + xoffset);
  int v_start = MAX(0, yoffset);
  int v_end = MIN(height, image->height + yoffset);
  if (h_end > h_start && v_end > v_start)
    {
      int cols = h_end - h_start;
      int rows = v_end - v_start;
      unsigned char *dst = new_rgb + 3 * (width * v_start + h_start);
      unsigned char *src =
	image->rgb_data + 3 * (image->width * (v_start - yoffset) +
			       h_start - xoffset);

      while (rows--)
	{
	  memcpy(dst, src, 3*cols);
	  dst += 3 * width;
	  src += 3 * image->width;
	}
    }

  free(image->rgb_data);
  free(image->alpha_data);
  image->rgb_data = new_rgb;
  image->width = width;
  image->height = height;
  image->area = area;
}


void tile_background(struct image *image, int width, int height,
		     int xoffset, int yoffset)
{
  int col_start, row_num;

  if ((col_start = -xoffset % image->width) < 0)
    col_start += image->width;

  if ((row_num = -yoffset % image->height) < 0)
    row_num += image->height;

  unsigned char *new_rgb = xmalloc(3 * width * height);

  unsigned char *rgb = new_rgb;
  unsigned char *row =  image->rgb_data + 3 * row_num * image->width;
  unsigned char *image_end =
    image->rgb_data + 3 * image->height * (image->width);
  for (int i = height; i--;)
    {
      unsigned char *col = row + 3 * col_start;
      unsigned char *row_end = row + 3 * image->width;

      for (int j = width; j--;)
	{
	  *rgb++ = *col++;
	  *rgb++ = *col++;
	  *rgb++ = *col++;

	  if (col == row_end)
	    col = row;
	}

      row += 3 * image->width;
      if (row == image_end)
	row = image->rgb_data;
    }

  free(image->alpha_data);
  image->alpha_data = NULL;
  free(image->rgb_data);
  image->rgb_data = new_rgb;
  image->width = width;
  image->height = height;
  image->area = width * height;
  return;
}
