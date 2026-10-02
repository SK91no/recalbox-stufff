/*
    fbv  --  simple image viewer for the linux framebuffer
    Copyright (C) 2000, 2001, 2003  Mateusz Golicz

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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} PIXEL;


unsigned char* simple_resize(unsigned char* orgin, int ox, int oy, int dx, int dy)
{
  unsigned char* cr = (unsigned char*) malloc(dx * dy * 3);
  assert(cr);
  unsigned char* l = cr;

  // Get 16:16 floating point increment
  double xStep = (double)ox / (double)dx;
  int pfXStep = (int)(xStep * 65536.0);

  for (int j = 0; j < dy; j++, l += dx * 3)
  {
    unsigned char* p = orgin + (j * oy / dy * ox * 3);
    unsigned int ip = 0;
    unsigned int* li = ((unsigned int*)l);
    for (int i = dx >> 2; --i >= 0; )
    {
      unsigned int pip = (ip >> 16) * 3; ip += pfXStep;
      unsigned p1 = p[pip] | (p[pip + 1] << 8) | (p[pip + 2] << 16);
      pip = (ip >> 16) * 3; ip += pfXStep;
      p1 |= (p[pip] << 24);
      unsigned p2 = p[pip + 1] | (p[pip + 2] << 8);
      pip = (ip >> 16) * 3; ip += pfXStep;
      p2 |= (p[pip] << 16) | (p[pip + 1] << 24);
      unsigned p3 = p[pip + 2];
      pip = (ip >> 16) * 3; ip += pfXStep;
      p3 |= (p[pip] << 8) | (p[pip + 1] << 16) | (p[pip + 2] << 24);
      li[0] = p1;
      li[1] = p2;
      li[2] = p3;
      li += 3;
    }
    unsigned char* lb = ((unsigned char*)li);
    for (int i = dx & 3; --i >= 0; ip += pfXStep)
    {
      unsigned int pip = (ip >> 16) * 3;
      lb[0] = p[pip];
      lb[1] = p[pip + 1];
      lb[2] = p[pip + 2];
      lb += 3;
    }
  }
  return (cr);
}

unsigned char* simple_rotate(unsigned char* image, int cols, int rows, int rotate)
{
  PIXEL* tmp_buffer;
  PIXEL* origin = (PIXEL*)image;
  tmp_buffer = malloc(rows*cols*3);
  if (tmp_buffer) {
    int j = 0;
    for(int row=0; row < rows; row++) {
      for(int col=0; col < cols; col++) {
        PIXEL* n = tmp_buffer;
        const PIXEL*o = origin+row*cols+col;
        switch(rotate) {
          case 0:
            n = (tmp_buffer) + row*cols +col;
            break;
          case 1:
            n = (tmp_buffer)+(col*rows) + (rows-row-1);
            break;
          case 2:
            n = (tmp_buffer) + ((rows*cols)-1) - j;
            j++;
            break;
          case 3:
            n = (tmp_buffer) + (cols -1 - col) * rows + row;
            break;
        }
        *n = *o;
      }
    }
    memcpy(origin, tmp_buffer, rows*cols*3);
    free(tmp_buffer);
  }else {
    return(NULL);
  }
  return (unsigned char*)origin;
}

unsigned char* alpha_resize(unsigned char* alpha, int ox, int oy, int dx, int dy)
{
  unsigned char* cr = (unsigned char*) malloc(dx * dy);
  unsigned char* l = cr;

  for (int j = 0; j < dy; j++, l += dx)
  {
    unsigned char* p = alpha + (j * oy / dy * ox);
    for (int i = 0, k = 0; i < dx; i++)
      l[k++] = p[i * ox / dx];
  }

  return (cr);
}

unsigned char* color_average_resize(unsigned char* orgin, int ox, int oy, int dx, int dy)
{
  unsigned char* cr = (unsigned char*) malloc(dx * dy * 3);
  assert(cr);
  unsigned char* p = cr;

  for (int j = 0; j < dy; j++)
  {
    for (int i = 0; i < dx; i++, p += 3)
    {
      int xa = i * ox / dx;
      int ya = j * oy / dy;
      int xb = (i + 1) * ox / dx;
      if (xb >= ox) xb = ox - 1;
      int yb = (j + 1) * oy / dy;
      if (yb >= oy) yb = oy - 1;
      int r = 0, g = 0, b =0, sq = 0;
      for (int l = ya; l <= yb; l++)
      {
        unsigned char* q = orgin + ((l * ox + xa) * 3);
        for (int k = xa; k <= xb; k++, q += 3, sq++)
        {
          r += q[0];
          g += q[1];
          b += q[2];
        }
      }
      p[0] = r / sq;
      p[1] = g / sq;
      p[2] = b / sq;
    }
  }
  return (cr);
}
