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

int process_args(int argc, char **argv);
int numeric(char *s);
XImage *take_display_screenshot(Display *dpy, XWindowAttributes attr);
char *write_screenshot_to_buffer(Display *dpy, XImage *im,
                                 XWindowAttributes attr, int channels);
void write_bgra(char *data, XImage *im, int w, int h);
int get_shift(unsigned long mask);
void write_pixel(char *data, XImage *im, unsigned long pix,
                 int x, int y, int w,
                 int rshift, int gshift, int bshift);
void save_screenshot(char *data, XWindowAttributes attr, int channels);
char *make_filename(void);

int main(int argc, char **argv) {
  int result = process_args(argc, argv);
  if (result > 0)
    return 1;

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    printf("%s: could not open display.\n", PROGRAM_NAME);
    return 1;
  }

  XWindowAttributes attr;
  XGetWindowAttributes(dpy, XDefaultRootWindow(dpy), &attr);

  if (delay > 0)
    usleep(delay*1000000);

  XImage *screenshot = take_display_screenshot(dpy, attr);
  if (!screenshot) {
    printf("%s: could not read display.\n", PROGRAM_NAME);
    XCloseDisplay(dpy);
    return 1;
  }

  int channels = 4;
  char *data = write_screenshot_to_buffer(dpy, screenshot, attr, channels);
  if (!data) {
    printf("%s: could not allocate memory for image.\n", PROGRAM_NAME);
    XCloseDisplay(dpy);
    XFree(screenshot);
    return 1;
  }

  save_screenshot(data, attr, channels);

  free(data);
  XFree(screenshot);
  XCloseDisplay(dpy);
  return 0;
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

int numeric(char *s) {
  for (int i = 0; s[i]; ++i)
    if (!isdigit(s[i]))
      return 0;

  return 1;
}

XImage *take_display_screenshot(Display *dpy, XWindowAttributes attr) {
  int x = 0;
  int y = 0;
  int w = attr.width;
  int h = attr.height;

  return XGetImage(dpy, XDefaultRootWindow(dpy), x, y, w, h,
                   AllPlanes, ZPixmap);
}

char *write_screenshot_to_buffer(Display *dpy, XImage *im,
                             XWindowAttributes attr, int channels) {
  int w = attr.width;
  int h = attr.height;
  char *data;

  // Color is stored as BGRA
  if (im->byte_order == LSBFirst) {
    data = malloc(sizeof(char)*w*h*channels);
    if (!data)
      return NULL;

    write_bgra(data, im, w, h);
  } else
    data = im->data;

  return data;
}

void write_bgra(char *data, XImage *im, int w, int h) {
  int rshift = get_shift(im->red_mask);
  int gshift = get_shift(im->green_mask);
  int bshift = get_shift(im->blue_mask);

  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      write_pixel(data, im, XGetPixel(im, x, y),
                  x, y, w,
                  rshift, gshift, bshift);
}

int get_shift(unsigned long mask) {
  if (mask == 0)
    return 0;

  int shift;
  for (shift = 0; (mask & 1) == 0; ++shift)
    mask >>= 1;

  return shift;
}

void write_pixel(char *data, XImage *im, unsigned long pix,
                 int x, int y, int w,
                 int rshift, int gshift, int bshift) {
  unsigned char r = (pix & im->red_mask) >> rshift;
  unsigned char g = (pix & im->green_mask) >> gshift;
  unsigned char b = (pix & im->blue_mask) >> bshift;

  int idx = (y*w + x)*4;
  data[idx] = r;
  data[idx + 1] = g;
  data[idx + 2] = b;
  data[idx + 3] = 0xff;
}

void save_screenshot(char *data, XWindowAttributes attr, int channels) {
  int w = attr.width;
  int h = attr.height;

  char *output_filename = make_filename();
  if (!output_filename) {
    printf("%s: could not allocate memory for filename.\n", PROGRAM_NAME);
    return;
  }

  if (!stbi_write_png(output_filename, w, h, channels, data, 0))
    printf("%s: there was an error writing to '%s'.\n", PROGRAM_NAME, output_filename);

  free(output_filename);
}

char *make_filename(void) {
  char *output_filename = calloc(strlen(filename_raw) + 4 + 1, sizeof(char)); // .png + '\0'
  if (!output_filename) 
    return NULL;

  strcat(output_filename, filename_raw);

  if (!strstr(output_filename, ".png"))
    strcat(output_filename, ".png");

  return output_filename;
}
