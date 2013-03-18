#ifndef _IMAGE_H_
#define _IMAGE_H_

struct image
{
  int height, width, area;
  unsigned char *rgb_data, *alpha_data;
};

int read_image(const char *filename, struct image *image);
void free_image_buffers(struct image *image);
Pixmap imageToPixmap(Display *dpy, struct image *image, int scr, Window win);
void resize_background(struct image *image, const int w, const int h);
void merge_with_background(struct image *panel, struct image *background,
			   int xoffset, int yoffset);
void frame_background(struct image *image,
		      unsigned int width, unsigned int height,
		      int xoffset, int yoffset, XftColor *color);
void tile_background(struct image *image, int width, int height,
		     int xoffset, int yoffset);

#endif /* _IMAGE_H_ */
