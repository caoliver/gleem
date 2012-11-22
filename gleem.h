void *xmalloc(size_t);

struct image
{
  int height, width, area;
  unsigned char *rgb_data, *alpha_data;
};
