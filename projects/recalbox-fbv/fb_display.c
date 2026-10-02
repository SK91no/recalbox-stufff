/*`
    fbv  --  simple image viewer for the linux framebuffer
    Copyright (C) 2000  Tomasz Sterna
    Copyright (C) 2003  Mateusz Golicz

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <linux/fb.h>
#include "config.h"

/* Public Use Functions:
 *
 * extern void fb_display(unsigned char *rgbbuff,
 *     int x_size, int y_size,
 *     int x_pan, int y_pan,
 *     int x_offs, int y_offs);
 *
 * extern void getCurrentRes(int *x,int *y);
 *
 */

unsigned short red[256], green[256], blue[256];
struct fb_cmap map332 = {
  0,
  256,
  red,
  green,
  blue,
  NULL
};
unsigned short red_b[256], green_b[256], blue_b[256];
struct fb_cmap map_back = {
  0,
  256,
  red_b,
  green_b,
  blue_b,
  NULL
};


int openFB(const char* name);

void closeFB(int fh);

void getVarScreenInfo(int fh, struct fb_var_screeninfo* var);

//void setVarScreenInfo(int fh, struct fb_var_screeninfo* var);

void getFixScreenInfo(int fh, struct fb_fix_screeninfo* fix);

void set332map(int fh);

void* convertRGB2FB(int fh, unsigned char* rgbbuff, unsigned long count, int bpp, int* cpp);

void blit2FB(int fh, void* fbbuff, unsigned char* alpha, unsigned int pic_xs, unsigned int pic_ys, unsigned int scr_xs,
             unsigned int scr_ys, unsigned int xp, unsigned int yp, unsigned int xoffs, unsigned int yoffs, int cpp,
             int clear_before);

void fb_display(unsigned char* rgbbuff, unsigned char* alpha, int x_size, int y_size, int x_pan, int y_pan, int x_offs,
                int y_offs, int clear_before)
{
  struct fb_var_screeninfo var;
  struct fb_fix_screeninfo fix;
  int bp = 0;

  /* get the framebuffer device handle */
  int fh = openFB(NULL);

  /* read current video mode */
  getVarScreenInfo(fh, &var);
  getFixScreenInfo(fh, &fix);

  int x_stride = (int)((fix.line_length * 8) / var.bits_per_pixel);

  /* correct panning */
  if (x_pan > x_size - x_stride) x_pan = 0;
  if (y_pan > y_size - (int)var.yres) y_pan = 0;
  /* correct offset */
  if (x_offs + x_size > x_stride) x_offs = 0;
  if (y_offs + y_size > (int)var.yres) y_offs = 0;

  /* blit buffer 2 fb */
  unsigned short* fbbuff = convertRGB2FB(fh, rgbbuff, x_size * y_size, var.bits_per_pixel, &bp);

  #if 0
  blit2FB(fh, fbbuff, alpha, x_size, y_size, x_stride, var.yres, x_pan, y_pan, x_offs, y_offs, bp);
  #else
  blit2FB(fh, fbbuff, alpha, x_size, y_size, x_stride, var.yres_virtual, x_pan, y_pan, x_offs, y_offs + var.yoffset,
          bp, clear_before);
  #endif
  free(fbbuff);

  /* close device */
  closeFB(fh);
}

void getCurrentRes(int* x, int* y)
{
  struct fb_var_screeninfo var;
  int fh = openFB(NULL);
  getVarScreenInfo(fh, &var);
  *x = var.xres;
  *y = var.yres;
  closeFB(fh);
}

int openFB(const char* name)
{
  if (name == NULL)
  {
    char* dev = getenv("FRAMEBUFFER");
    if (dev) name = dev;
    else name = DEFAULT_FRAMEBUFFER;
  }

  int fh = open(name, O_RDWR);
  if (fh == -1)
  {
    fprintf(stderr, "open %s: %s\n", name, strerror(errno));
    exit(1);
  }
  return fh;
}

void closeFB(int fh)
{
  close(fh);
}

void getVarScreenInfo(int fh, struct fb_var_screeninfo* var)
{
  if (ioctl(fh, FBIOGET_VSCREENINFO, var))
  {
    fprintf(stderr, "ioctl FBIOGET_VSCREENINFO: %s\n", strerror(errno));
    exit(1);
  }
}

/*void setVarScreenInfo(int fh, struct fb_var_screeninfo* var)
{
  if (ioctl(fh, FBIOPUT_VSCREENINFO, var))
  {
    fprintf(stderr, "ioctl FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
    exit(1);
  }
}*/

void getFixScreenInfo(int fh, struct fb_fix_screeninfo* fix)
{
  if (ioctl(fh, FBIOGET_FSCREENINFO, fix))
  {
    fprintf(stderr, "ioctl FBIOGET_FSCREENINFO: %s\n", strerror(errno));
    exit(1);
  }
}

void make332map(struct fb_cmap* map)
{
  int r = 8, g = 8, b = 4;

  map->red = red;
  map->green = green;
  map->blue = blue;

  int rs = 256 / (r - 1);
  int gs = 256 / (g - 1);
  int bs = 256 / (b - 1);

  for (int i = 0; i < 256; i++)
  {
    map->red[i] = (rs * ((i / (g * b)) % r)) * 255;
    map->green[i] = (gs * ((i / b) % g)) * 255;
    map->blue[i] = (bs * ((i) % b)) * 255;
  }
}

void set8map(int fh, struct fb_cmap* map)
{
  if (ioctl(fh, FBIOPUTCMAP, map) < 0)
  {
    fprintf(stderr, "Error putting colormap");
    exit(1);
  }
}

void get8map(int fh, struct fb_cmap* map)
{
  if (ioctl(fh, FBIOGETCMAP, map) < 0)
  {
    fprintf(stderr, "Error getting colormap");
    exit(1);
  }
}

void set332map(int fh)
{
  make332map(&map332);
  set8map(fh, &map332);
}

void blit2FB(int fh, void* fbbuff, unsigned char* alpha, unsigned int pic_xs, unsigned int pic_ys, unsigned int scr_xs,
             unsigned int scr_ys, unsigned int xp, unsigned int yp, unsigned int xoffs, unsigned int yoffs, int cpp, int clear_before)
{
  int xc = (int)((pic_xs > scr_xs) ? scr_xs : pic_xs);
  int yc = (int)((pic_ys > scr_ys) ? scr_ys : pic_ys);

  unsigned char* fb = (unsigned char*)malloc(scr_xs * scr_ys * cpp);
  if (clear_before != 0) memset(fb, 0, scr_xs * scr_ys * cpp);
  else
  {
    read(fh, fb, scr_xs * scr_ys * cpp);
    lseek(fh, 0, SEEK_SET);
  }

  if (fb == NULL /*MAP_FAILED*/)
  {
    perror("mmap");
    return;
  }

  if (cpp == 1)
  {
    get8map(fh, &map_back);
    set332map(fh);
  }

  unsigned char* fbptr = fb + (yoffs * scr_xs + xoffs) * cpp;
  unsigned char* imptr = fbbuff + (yp * pic_xs + xp) * cpp;

  if (alpha)
  {
    unsigned char* alphaptr = alpha + (yp * pic_xs + xp);

    for (int i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp, alphaptr += pic_xs)
    {
      for (int x = 0; x < xc; x++)
      {
        int from = -1, to = -1;
        for (int v = x; v < xc; v++)
        {
          if (from == -1)
          {
            if (alphaptr[v] > 0x80) from = v;
          }
          else
          {
            if (alphaptr[v] < 0x80)
            {
              to = v;
              break;
            }
          }
        }
        if (from == -1)
          break;

        if (to == -1) to = xc;

        memcpy(fbptr + (from * cpp), imptr + (from * cpp), (to - from - 1) * cpp);
        x += to - from - 1;
      }
    }
  }
  else
    for (int i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp)
      memcpy(fbptr, imptr, xc * cpp);

  if (cpp == 1)
    set8map(fh, &map_back);

  if (write(fh, fb, scr_xs * scr_ys * cpp) != scr_xs * scr_ys * cpp)
    perror("write");
  lseek(fh, 0, SEEK_SET);
  free(fb);
}

inline static unsigned char make8color(unsigned char r, unsigned char g, unsigned char b)
{
  return ((((r >> 5) & 7) << 5) | (((g >> 5) & 7) << 2) | ((b >> 6) & 3));
}

inline static unsigned short make15color(unsigned char r, unsigned char g, unsigned char b)
{
  return ((((r >> 3) & 31) << 10) | (((g >> 3) & 31) << 5) | ((b >> 3) & 31));
}

inline static unsigned short make15color_bgr(unsigned char r, unsigned char g, unsigned char b)
{
  return ((((b >> 3) & 31) << 10) | (((g >> 3) & 31) << 5) | ((r >> 3) & 31));
}

inline static unsigned short make16color(unsigned char r, unsigned char g, unsigned char b)
{
  return ((((r >> 3) & 31) << 11) | (((g >> 2) & 63) << 5) | ((b >> 3) & 31));
}

void* convertRGB2FB(int fh, unsigned char* rgbbuff, unsigned long count, int bpp, int* cpp)
{
  void* fbbuff = NULL;
  u_int8_t* c_fbbuff = NULL;
  u_int16_t* s_fbbuff = NULL;
  u_int32_t* i_fbbuff = NULL;

  switch (bpp)
  {
    case 8:
    {
      *cpp = 1;
      c_fbbuff = (unsigned char*) malloc(count * sizeof(unsigned char));
      for (unsigned long i = 0; i < count; i++)
        c_fbbuff[i] = make8color(rgbbuff[i * 3], rgbbuff[i * 3 + 1], rgbbuff[i * 3 + 2]);
      fbbuff = (void*) c_fbbuff;
      break;
    }
    case 15:
    {
      struct fb_var_screeninfo var;
      getVarScreenInfo(fh, &var);
      int is_bgr555 = (var.red.offset == 0 && var.green.offset == 5 && var.blue.offset == 10) ? 1 : 0;
      *cpp = 2;
      s_fbbuff = (unsigned short*) malloc(count * sizeof(unsigned short));
      if (is_bgr555 != 0)
        for (unsigned long i = 0; i < count; i++)
          s_fbbuff[i] = make15color_bgr(rgbbuff[i * 3], rgbbuff[i * 3 + 1], rgbbuff[i * 3 + 2]);
      else
        for (unsigned long i = 0; i < count; i++)
          s_fbbuff[i] = make15color(rgbbuff[i * 3], rgbbuff[i * 3 + 1], rgbbuff[i * 3 + 2]);
      fbbuff = (void*) s_fbbuff;
      break;
    }
    case 16:
    {
      struct fb_var_screeninfo var;
      getVarScreenInfo(fh, &var);
      int is_bgr555 = (var.red.offset == 0 && var.green.offset == 5 && var.blue.offset == 10) ? 1 : 0;
      *cpp = 2;
      s_fbbuff = (unsigned short*) malloc(count * sizeof(unsigned short));
      if (is_bgr555 != 0)
        for (unsigned long i = 0; i < count; i++)
          s_fbbuff[i] = make15color_bgr(rgbbuff[i * 3], rgbbuff[i * 3 + 1], rgbbuff[i * 3 + 2]);
      else
        for (unsigned long i = 0; i < count; i++)
          s_fbbuff[i] = make16color(rgbbuff[i * 3], rgbbuff[i * 3 + 1], rgbbuff[i * 3 + 2]);
      fbbuff = (void*) s_fbbuff;
      break;
    }
    case 24:
    {
      *cpp = 3;
      c_fbbuff = (unsigned char *) malloc(count * 3 * sizeof(unsigned char));
      for(unsigned long i = 0; i < (3 * count); i += 3)
      {
        /* Big endian framebuffer. */
        c_fbbuff[i] = rgbbuff[i+2];
        c_fbbuff[i+1] = rgbbuff[i+1];
        c_fbbuff[i+2] = rgbbuff[i];
      }
      fbbuff = (void *) c_fbbuff;
      break;
    }
    case 32:
    {
      *cpp = 4;
      i_fbbuff = (unsigned int*) malloc(count * sizeof(unsigned int));
      for (unsigned long i = 0; i < count; i++)
        i_fbbuff[i] = ((rgbbuff[i * 3] << 16) & 0xFF0000) | ((rgbbuff[i * 3 + 1] << 8) & 0xFF00) |
                      (rgbbuff[i * 3 + 2] & 0xFF);
      fbbuff = (void*) i_fbbuff;
      break;
    }
    default: fprintf(stderr, "Unsupported video mode! You've got: %dbpp\n", bpp);
      exit(1);
  }
  return fbbuff;
}
