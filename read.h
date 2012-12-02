#ifndef _READ_H_
#define _READ_H_

#define MAX_DIMENSION 10000

int read_jpeg(FILE *infile, const char *filename, int *width, int *height,
          unsigned char **rgb, unsigned char **alpha);

int read_png(FILE *infile, const char *filename, int *width, int *height,
          unsigned char **rgb, unsigned char **alpha);

#endif /* _READ_H_ */
