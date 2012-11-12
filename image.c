#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>

struct image
{
  int height, width, area;
  unsigned char *rgba_data;
};

int read_image(const char *filename, struct image *image)
{
  char buf[4];
  unsigned char *ubuf = (unsigned char *) buf;
  int success = 0;
  FILE *file;

  if ((file = fopen(filename, "rb")) == NULL)
    return (success == 1);
  if (fread(buf, 1, 4, file) != 4)
    goto bugout;
  rewind(file);
  if ((ubuf[0] == 0x89) && !strncmp("PNG", buf + 1, 3))
    success = readPng(file, &image->width, &image->height, &image->rgba_data);
  else if ((ubuf[0] == 0xff) && (ubuf[1] == 0xd8))
    success =
      readJpeg(file, &image->width, &image->height, &image->rgba_data);
  else
    fprintf(stderr, "Unknown image format\n");

  image->area = image->width * image->height;

 bugout:
  fclose(file);
  return (success == 1);
}


void free_image_buffers(struct image *image)
{
  free(image->rgba_data);
  image->rgba_data = NULL;
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
	  left_shift++;
	  mask >>= 1;
	}
      while ((mask & 0x01) == 1)
	{
	  right_shift--;
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

  char *pixmap_data;
  switch (depth)
    {
    case 32:
    case 24:
      pixmap_data = malloc(4 * area);
      break;
    case 16:
    case 15:
      pixmap_data = malloc(2 * area);
      break;
    case 8:
      pixmap_data = malloc(area);
      break;
    default:
      break;
    }
  if (!pixmap_data)
    abort();

  XImage *ximage = XCreateImage(dpy, visual, depth, ZPixmap, 0,
				pixmap_data, width, height,
				8, 0);

  int entries;
  XVisualInfo v_template;
  v_template.visualid = XVisualIDFromVisual(visual);
  XVisualInfo *visual_info = XGetVisualInfo(dpy, VisualIDMask,
					    &v_template, &entries);

  unsigned char *rgba = image->rgba_data;
  switch (visual_info->class)
    {
    case PseudoColor:
      {
	XColor xc, *colors;
	xc.flags = DoRed | DoGreen | DoBlue;
	int *closest_color;

	int num_colors = 256;
	if (!(colors = malloc(sizeof(XColor) * num_colors)))
	  abort();
	for (int i = 0; i < num_colors; i++)
	  colors[i].pixel = (unsigned long) i;
	XQueryColors(dpy, colormap, colors, num_colors);

	if (!(closest_color = malloc(sizeof(int) * num_colors)))
	  abort();
	for (int i = 0; i < num_colors; i++)
	  {
	    xc.red = (i & 0xe0) << 8;	// highest 3 bits
	    xc.green = (i & 0x1c) << 11;	// middle 3 bits
	    xc.blue = (i & 0x03) << 14;	// lowest 2 bits

	    // find the closest color in the colormap
	    double distance, distance_squared, min_distance = 0;
	    for (int ii = 0; ii < num_colors; ii++)
	      {
		distance = colors[ii].red - xc.red;
		distance_squared = distance * distance;
		distance = colors[ii].green - xc.green;
		distance_squared += distance * distance;
		distance = colors[ii].blue - xc.blue;
		distance_squared += distance * distance;

		if ((ii == 0) || (distance_squared <= min_distance))
		  {
		    min_distance = distance_squared;
		    closest_color[i] = ii;
		  }
	      }
	  }

	for (int j = 0; j < height; j++)
	  {
	    for (int i = 0; i < width; i++)
	      {
		xc.red = (unsigned short) (*rgba++ & 0xe0);
		xc.green = (unsigned short) (*rgba++ & 0xe0);
		xc.blue = (unsigned short) (*rgba++ & 0xc0);
		// Skip alpha channel.
		rgba++;

		xc.pixel = xc.red | (xc.green >> 3) | (xc.blue >> 6);
		XPutPixel(ximage, i, j,
			  colors[closest_color[xc.pixel]].pixel);
	      }
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
		red = (unsigned long) *rgba++ >> red_right_shift;
		green = (unsigned long) *rgba++ >> green_right_shift;
		blue = (unsigned long) *rgba++ >> blue_right_shift;
		// Skip alpha channel.
		rgba++;

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
