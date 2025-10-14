#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PROGRAM_NAME "evilsc"
#define DEFAULT_FNAME "out"
#define DEFAULT_DELAY 0

#define streq(a, b) (strcmp((a), (b)) == 0)

char *filename_raw = DEFAULT_FNAME;
int delay = DEFAULT_DELAY;

int get_shift(unsigned long mask) {
  if (mask == 0)
    return 0;

  int shift;
  for (shift = 0; (mask & 1) == 0; ++shift)
    mask >>= 1;

  return shift;
}

int numeric(char *s) {
  for (int i = 0; s[i]; ++i)
    if (!isdigit(s[i]))
      return 0;

  return 1;
}

int process_args(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    if (streq(argv[i], "-o") || streq(argv[i], "--output"))
      filename_raw = argv[i + 1];

    if (streq(argv[i], "-d") || streq(argv[i], "--delay")) {
      int index = i + 1;
      if (!numeric(argv[index])) {
        printf("%s: delay parameter ('%s') must be numeric.\n", PROGRAM_NAME, argv[index]);
        return 1;
      }

      delay = atoi(argv[index]);
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  int result = process_args(argc, argv);
  if (result > 0)
    return 1;

  int filename_raw_len = strlen(filename_raw);

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    printf("%s: could not open display.\n", PROGRAM_NAME);
    return 1;
  }

  XWindowAttributes attr;
  XGetWindowAttributes(dpy, XDefaultRootWindow(dpy), &attr);

  int x = 0;
  int y = 0;
  int w = attr.width;
  int h = attr.height;
  int channels = 4;

  if (delay > 0)
    usleep(delay*1000000);

  XImage *im = XGetImage(dpy, XDefaultRootWindow(dpy), x, y, w, h,
                        AllPlanes, ZPixmap);
  if (!im) {
    printf("%s: could not read display.\n", PROGRAM_NAME);
    XCloseDisplay(dpy);
    return 1;
  }

  char *data;

  // Color is stored as BGRA
  if (im->byte_order == LSBFirst) {
    data = malloc(sizeof(char)*w*h*channels);
    if (!data) {
      printf("%s: could not allocate memory for image.\n", PROGRAM_NAME);
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

  char *output_filename = calloc(filename_raw_len + 4 + 1, sizeof(char)); // .png + '\0'
  strcat(output_filename, filename_raw);

  if (!strstr(output_filename, ".png"))
    strcat(output_filename, ".png");

  if (!stbi_write_png(output_filename, w, h, channels, data, 0))
    printf("%s: there was an error writing to '%s'.\n", PROGRAM_NAME, output_filename);

  free(output_filename);
  free(data);
  XFree(im);
  XCloseDisplay(dpy);
  return 0;
}
