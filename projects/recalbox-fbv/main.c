/*
	fbv  --  simple image viewer for the linux framebuffer
	Copyright (C) 2000, 2001, 2003, 2004  Mateusz 'mteg' Golicz

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
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "config.h"
#include "fbv.h"

static int opt_clear = 1, opt_alpha = 0, opt_image_info = 1, opt_stretch = 0, opt_delay = 0, opt_enlarge = 0, opt_ignore_aspect = 0, opt_loop = 0;

static int get_fbcon_rotate() {
  char buffer[10];
  FILE * fbcon_fh;
  int rotate=0;
  fbcon_fh = fopen("/sys/class/graphics/fbcon/rotate", "r");
  if (fbcon_fh) {
    fgets(buffer, 10, fbcon_fh);
    rotate = atoi(buffer);
    fclose(fbcon_fh);
  }
  return rotate;
}

static inline void do_enlarge(struct image* i, int screen_width, int screen_height, int ignoreaspect)
{
  if (((i->width > screen_width) || (i->height > screen_height)) && (!ignoreaspect))
    return;
  if ((i->width < screen_width) || (i->height < screen_height))
  {
    int xsize = i->width, ysize = i->height;
    unsigned char* image = NULL, * alpha = NULL;

    if (ignoreaspect)
    {
      if (i->width < screen_width)
        xsize = screen_width;
      if (i->height < screen_height)
        ysize = screen_height;

      goto have_sizes;
    }

    if ((i->height * screen_width / i->width) <= screen_height)
    {
      xsize = screen_width;
      ysize = i->height * screen_width / i->width;
      goto have_sizes;
    }

    if ((i->width * screen_height / i->height) <= screen_width)
    {
      xsize = i->width * screen_height / i->height;
      ysize = screen_height;
      goto have_sizes;
    }
    return;
    have_sizes:
    image = simple_resize(i->rgb, i->width, i->height, xsize, ysize);
    if (i->alpha)
      alpha = alpha_resize(i->alpha, i->width, i->height, xsize, ysize);

    if (i->do_free)
    {
      free(i->alpha);
      free(i->rgb);
    }

    i->rgb = image;
    i->alpha = alpha;
    i->do_free = 1;
    i->width = xsize;
    i->height = ysize;
  }
}


static inline void do_fit_to_screen(struct image* i, int screen_width, int screen_height, int ignoreaspect, int cal)
{
  if ((i->width > screen_width) || (i->height > screen_height))
  {
    unsigned char* new_image = NULL, * new_alpha = NULL;
    int nx_size = i->width, ny_size = i->height;

    if (ignoreaspect)
    {
      if (i->width > screen_width)
        nx_size = screen_width;
      if (i->height > screen_height)
        ny_size = screen_height;
    }
    else
    {
      if ((i->height * screen_width / i->width) <= screen_height)
      {
        nx_size = screen_width;
        ny_size = i->height * screen_width / i->width;
      }
      else
      {
        nx_size = i->width * screen_height / i->height;
        ny_size = screen_height;
      }
    }

    if (cal)
      new_image = color_average_resize(i->rgb, i->width, i->height, nx_size, ny_size);
    else
      new_image = simple_resize(i->rgb, i->width, i->height, nx_size, ny_size);

    if (i->alpha)
      new_alpha = alpha_resize(i->alpha, i->width, i->height, nx_size, ny_size);

    if (i->do_free)
    {
      free(i->alpha);
      free(i->rgb);
    }

    i->rgb = new_image;
    i->alpha = new_alpha;
    i->do_free = 1;
    i->width = nx_size;
    i->height = ny_size;
  }
}

static int always_false()
{
  return 0;
}

int show_image(char* filename)
{
  int (* load)(char*, unsigned char*, unsigned char**, int, int) = NULL;

  unsigned char* image = NULL;
  unsigned char* alpha = NULL;

  int x_size = 0, y_size = 0, screen_width = 0, screen_height = 0;
  int ret = 1;

  int transform_stretch = opt_stretch, transform_enlarge = opt_enlarge,
      transform_cal = (opt_stretch == 2), transform_iaspect = opt_ignore_aspect;

  struct image i;

  if (always_false())
  {
  }
#ifdef FBV_SUPPORT_GIF
  else if (fh_gif_id(filename))
  {
    if (fh_gif_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
      load = fh_gif_load;
  }
#endif
#ifdef FBV_SUPPORT_PNG
  else if (fh_png_id(filename))
  {
    if (fh_png_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
      load = fh_png_load;
  }
#endif
#ifdef FBV_SUPPORT_JPEG
  else if (fh_jpeg_id(filename))
  {
    if (fh_jpeg_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
      load = fh_jpeg_load;
  }
#endif
#ifdef FBV_SUPPORT_BMP
  else if (fh_bmp_id(filename))
  {
    if (fh_bmp_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
      load = fh_bmp_load;
  }
#endif
  else
  {
    fprintf(stderr, "%s: Unable to access file or file format unknown.\n", filename);
    return (1);
  }

  if (!(image = (unsigned char*) malloc(x_size * y_size * 3)))
  {
    fprintf(stderr, "%s: Out of memory.\n", filename);
    goto error_mem;
  }

  if (load(filename, image, &alpha, x_size, y_size) != FH_ERROR_OK)
  {
    fprintf(stderr, "%s: Image data is corrupt?\n", filename);
    goto error_mem;
  }

  // handle image rotation, depending of fbcon rotate property
  int rotate = get_fbcon_rotate();
  if (simple_rotate(image, x_size, y_size, rotate)) {
    if (rotate == 1 || rotate == 3) {
      int old_x = x_size;
      x_size = y_size;
      y_size = old_x;
    }
  }else {
    printf("rotation not possible\n");
  }

  if (!opt_alpha)
  {
    free(alpha);
    alpha = NULL;
  }

  getCurrentRes(&screen_width, &screen_height);
  i.do_free = 0;
  i.width = x_size;
  i.height = y_size;
  i.rgb = image;
  i.alpha = alpha;

  if (transform_stretch)
    do_fit_to_screen(&i, screen_width, screen_height, transform_iaspect, transform_cal);

  if (transform_enlarge)
    do_enlarge(&i, screen_width, screen_height, transform_iaspect);

  if (opt_image_info)
    printf("fbv - The Framebuffer Viewer\n%s\n%d x %d\n", filename, x_size, y_size);

  int x_offs = (i.width < screen_width) ?  (screen_width - i.width) / 2 : 0;
  int y_offs = (i.height < screen_height) ? (screen_height - i.height) / 2 : 0;

  fb_display(i.rgb, i.alpha, i.width, i.height, 0, 0, x_offs, y_offs, opt_clear);
  opt_clear = 0; // Clear only once

  if (opt_delay != 0) usleep(opt_delay * 100000);

  error_mem:
  free(image);
  free(alpha);
  if (i.do_free)
  {
    free(i.rgb);
    free(i.alpha);
  }
  return (ret);
}

void help(char* name)
{
  printf("Usage: %s [options] image1 image2 image3 ...\n\n"
         "Available options:\n"
         " --help        | -h : Show this help\n"
         " --alpha       | -a : Use the alpha channel (if applicable)\n"
         " --dontclear   | -c : Do not clear the screen before and after displaying the image\n"
         " --donthide    | -u : Do not hide the cursor before and after displaying the image\n"
         " --noinfo      | -i : Supress image information\n"
         " --stretch     | -f : Strech (using a simple resizing routine) the image to fit onto screen if necessary\n"
         " --colorstretch| -k : Strech (using a 'color average' resizing routine) the image to fit onto screen if necessary\n"
         " --enlarge     | -e : Enlarge the image to fit the whole screen if necessary\n"
         " --ignore-aspect| -r : Ignore the image aspect while resizing\n"
         " --delay <d>   | -s <delay> : Slideshow, 'delay' is the slideshow delay in tenths of seconds.\n\n"
         " --loop        | -l : Loop after displaying all images\n"
         "Copyright (C) 2000 - 2004 Mateusz Golicz, Tomasz Sterna.\n", name);
}

void sighandler(int s)
{
  _exit(128 + s);
}

int main(int argc, char** argv)
{
  static struct option long_options[] = {{"help",          no_argument,       0, 'h'},
                                         {"noclear",       no_argument,       0, 'c'},
                                         {"alpha",         no_argument,       0, 'a'},
                                         {"noinfo",        no_argument,       0, 'i'},
                                         {"stretch",       no_argument,       0, 'f'},
                                         {"colorstrech",   no_argument,       0, 'k'},
                                         {"delay",         required_argument, 0, 's'},
                                         {"enlarge",       no_argument,       0, 'e'},
                                         {"ignore-aspect", no_argument,       0, 'r'},
                                         {"loop",          no_argument,       0, 'l'},
                                         {0, 0,                               0, 0}};

  int c = argc;
  if (c < 2)
  {
    help(argv[0]);
    fprintf(stderr, "Error: Required argument missing.\n");
    return (1);
  }

  while((c = getopt_long_only(argc, argv, "hcaifks:erl", long_options, NULL)) != EOF)
  {
    switch (c)
    {
      case 'a': opt_alpha = 1;
        break;
      case 'c': opt_clear = 0;
        break;
      case 's': opt_delay = (int) strtol(optarg, NULL, 10);
        break;
      case 'h': help(argv[0]);
        return (0);
      case 'i': opt_image_info = 0;
        break;
      case 'f': opt_stretch = 1;
        break;
      case 'k': opt_stretch = 2;
        break;
      case 'e': opt_enlarge = 1;
        break;
      case 'r': opt_ignore_aspect = 1;
        break;
      case 'l': opt_loop = 1;
        break;
      default: break;
    }
  }

  if (!argv[optind])
  {
    fprintf(stderr, "Required argument missing! Consult %s -h.\n", argv[0]);
    return (1);
  }

  signal(SIGHUP, sighandler);
  signal(SIGINT, sighandler);
  signal(SIGQUIT, sighandler);
  signal(SIGSEGV, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGABRT, sighandler);

  do
  {
    for (int i = optind; argv[i];)
    {
      int r = show_image(argv[i]);

      if (!r) break;

      i += r;
      if (i < optind)
        i = optind;
    }
  } while (opt_loop != 0);

  return (0);
}
