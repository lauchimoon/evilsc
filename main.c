#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main() {
  Display *dpy = XOpenDisplay(NULL);

  XWindowAttributes attr;
  XGetWindowAttributes(dpy, XDefaultRootWindow(dpy), &attr);

  int x = 0;
  int y = 0;
  int w = attr.width;
  int h = attr.height;

  XImage *im = XGetImage(dpy, XDefaultRootWindow(dpy), x, y, w, h,
                        AllPlanes, ZPixmap);

  printf("%d\n", im->bits_per_pixel);
  if (!stbi_write_png("out.png", im->width, im->height, 4, im->data, 0))
    printf(":(\n");

  XFree(im);
  XCloseDisplay(dpy);
  return 0;
}
