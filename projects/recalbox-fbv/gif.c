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
#include "config.h"

#ifdef FBV_SUPPORT_GIF

  #include "fbv.h"
  #include <stdio.h>
  //#include <sys/types.h>
  //#include <sys/stat.h>
  #include <fcntl.h>
  //#include <setjmp.h>
  #include <gif_lib.h>
  //#include <signal.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <string.h>

  #define min(a, b) ((a) < (b) ? (a) : (b))
  #define gflush do { return(FH_ERROR_FILE); } while(false)
  #define grflush do { DGifCloseFile(gft, NULL); return(FH_ERROR_FORMAT); } while(false)
  #define mgrflush do { free(lb); free(slb); DGifCloseFile(gft, NULL); return(FH_ERROR_FORMAT); } while(false)
  //#define agflush return(FH_ERROR_FORMAT);
  //#define agrflush { DGifCloseFile(gft); return(FH_ERROR_FORMAT); }


int fh_gif_id(char* name)
{
  char id[4];
  int fd = open(name, O_RDONLY);
  if (fd == -1) return (0);
  read(fd, id, 4);
  close(fd);
  if (id[0] == 'G' && id[1] == 'I' && id[2] == 'F') return (1);
  return (0);
}

void
m_rend_gif_decodecolormap(const unsigned char* cmb, unsigned char* rgbb, ColorMapObject* cm, int s, int l, int transparency)
{
  (void)s;
  (void)transparency;
  for (int i = 0; i < l; i++)
  {
    GifColorType* cmentry = &cm->Colors[cmb[i]];
    *(rgbb++) = cmentry->Red;
    *(rgbb++) = cmentry->Green;
    *(rgbb++) = cmentry->Blue;
  }
}


/* Thanks goes here to Mauro Meneghin, who implemented interlaced GIF files support */

int fh_gif_load(char* name, unsigned char* buffer, unsigned char** alpha, int x, int y)
{
  (void)x;
  (void)y;
  int in_nextrow[4] = { 8, 8, 4, 2 };   //interlaced jump to the row current+in_nextrow
  int in_beginrow[4] = { 0, 4, 2, 1 };  //begin pass j from that row number
  int transparency = -1;  //-1 means not transparency present
  int ibxs = 0;
  GifByteType* extension = NULL;
  int extcode = 0;

  GifFileType* gft = DGifOpenFileName(name, NULL);
  if (gft == NULL)
  {
    printf("err5\n");
    gflush;
  } //////////
  GifRecordType rt = UNDEFINED_RECORD_TYPE;
  do
  {
    if (DGifGetRecordType(gft, &rt) == GIF_ERROR)
    grflush;
    switch (rt)
    {
      case IMAGE_DESC_RECORD_TYPE:
        if (DGifGetImageDesc(gft) == GIF_ERROR)
        grflush;
        int px = gft->Image.Width;
        int py = gft->Image.Height;
        char* lb = (char*) malloc(px * 3);
        char* slb = (char*) malloc(px);
        //  printf("reading...\n");
        if (lb != NULL && slb != NULL)
        {
          unsigned char* alphaptr = NULL;

          ColorMapObject* cmap = (gft->Image.ColorMap ? gft->Image.ColorMap : gft->SColorMap);
          int cmaps = cmap->ColorCount;

          ibxs = ibxs * 3;
          char* fbptr = (char*) buffer;

          if (transparency != -1)
          {
            alphaptr = malloc(px * py);
            *alpha = alphaptr;
          }

          if (!(gft->Image.Interlace))
          {
            for (int i = 0; i < py; i++, fbptr += px * 3)
            {
              if (DGifGetLine(gft, (GifPixelType*) slb, px) == GIF_ERROR)
              mgrflush;
              m_rend_gif_decodecolormap((unsigned char*) slb, (unsigned char*) lb, cmap, cmaps, px, transparency);
              memcpy(fbptr, lb, px * 3);
              if (alphaptr)
                for (int j = 0; j < px; j++) *(alphaptr++) = (((unsigned char*) slb)[j] == transparency) ? 0x00 : 0xff;
            }
          }
          else
          {
            unsigned char* aptr = NULL;

            for (int j = 0; j < 4; j++)
            {
              if (alphaptr)
                aptr = alphaptr + (in_beginrow[j] * px);

              fbptr = (char*) buffer + (in_beginrow[j] * px * 3);

              for (int i = in_beginrow[j];
                   i < py; i += in_nextrow[j], fbptr += px * 3 * in_nextrow[j], aptr += px * in_nextrow[j])
              {
                if (DGifGetLine(gft, (GifPixelType*) slb, px) == GIF_ERROR)
                mgrflush; /////////////
                m_rend_gif_decodecolormap((unsigned char*) slb, (unsigned char*) lb, cmap, cmaps, px, transparency);
                memcpy(fbptr, lb, px * 3);
                if (alphaptr)
                  for (int k = 0; k < px; k++) aptr[k] = (((unsigned char*) slb)[k] == transparency) ? 0x00 : 0xff;
              }
            }
          }
        }
        if (lb) free(lb);
        if (slb) free(slb);
        break;
      case EXTENSION_RECORD_TYPE:
        if (DGifGetExtension(gft, &extcode, &extension) == GIF_ERROR)
        grflush; //////////
        if (extcode == 0xf9) //look image transparency in graph ctr extension
        {
          if (extension[1] & 1)
          {
            transparency = extension[4];

          }
          //		    tran_off=(int)*extension;
          //		    transparency=(int)*(extension+tran_off);
          //			printf("transparency: %d\n", transparency);
        }
        while (extension != NULL)
        {
          if (DGifGetExtensionNext(gft, &extension) == GIF_ERROR)
          grflush;
        }
        break;
      default: break;
    }
  } while (rt != TERMINATE_RECORD_TYPE);
  DGifCloseFile(gft, NULL);
  return (FH_ERROR_OK);
}


int fh_gif_getsize(char* name, int* x, int* y)
{
  GifByteType* extension = NULL;
  int extcode = 0;

  GifFileType* gft = DGifOpenFileName(name, NULL);
  if (gft == NULL) gflush;
  GifRecordType rt = UNDEFINED_RECORD_TYPE;
  do
  {
    if (DGifGetRecordType(gft, &rt) == GIF_ERROR)
    grflush;
    switch (rt)
    {
      case IMAGE_DESC_RECORD_TYPE:

        if (DGifGetImageDesc(gft) == GIF_ERROR)
        grflush;
        int px = gft->Image.Width;
        int py = gft->Image.Height;
        *x = px;
        *y = py;
        DGifCloseFile(gft, NULL);
        return (FH_ERROR_OK);
      case EXTENSION_RECORD_TYPE:
        if (DGifGetExtension(gft, &extcode, &extension) == GIF_ERROR)
        grflush;
        while (extension != NULL)
        {
          if (DGifGetExtensionNext(gft, &extension) == GIF_ERROR)
          grflush;
        }
        break;
      default: break;
    }
  } while (rt != TERMINATE_RECORD_TYPE);
  DGifCloseFile(gft, NULL);
  return (FH_ERROR_FORMAT);
}

#endif
