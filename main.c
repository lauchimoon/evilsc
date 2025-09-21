#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PROGRAM_NAME "evilsc"

int get_shift(unsigned long mask) {
  if (mask == 0)
    return 0;

  int shift;
  for (shift = 0; (mask & 1) == 0; ++shift)
    mask >>= 1;

  return shift;
}

int main() {
  Display *dpy = XOpenDisplay(NULL);
  if (!dpy)
    return 1;

  XWindowAttributes attr;
  XGetWindowAttributes(dpy, XDefaultRootWindow(dpy), &attr);

  int x = 0;
  int y = 0;
  int w = attr.width;
  int h = attr.height;
  int channels = 4;

  XImage *im = XGetImage(dpy, XDefaultRootWindow(dpy), x, y, w, h,
                        AllPlanes, ZPixmap);
  if (!im) {
    printf("%s: could not read display\n", PROGRAM_NAME);
    XCloseDisplay(dpy);
    return 1;
  }

  unsigned char *data;

  // Color is stored as BGRA
  if (im->byte_order == LSBFirst) {
    data = malloc(sizeof(unsigned char)*w*h*channels);
    if (!data) {
      printf("%s: could not allocate memory for image\n", PROGRAM_NAME);
      XCloseDisplay(dpy);
      XFree(im);
      return 1;
    }

    int rshift = get_shift(im->red_mask);
    int gshift = get_shift(im->green_mask);
    int bshift = get_shift(im->blue_mask);

    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        unsigned long p = XGetPixel(im, x, y);

        unsigned char r = (p & im->red_mask) >> rshift;
        unsigned char g = (p & im->green_mask) >> gshift;
        unsigned char b = (p & im->blue_mask) >> bshift;

        int idx = (y*w + x)*4;
        data[idx] = r;
        data[idx + 1] = g;
        data[idx + 2] = b;
        data[idx + 3] = 0xff;
      }
    }
  } else
    data = im->data;

  if (!stbi_write_png("out.png", w, h, channels, data, 0))
    printf("%s: there was an error writing output file.\n", PROGRAM_NAME);

  free(data);
  XFree(im);
  XCloseDisplay(dpy);
  return 0;
}
