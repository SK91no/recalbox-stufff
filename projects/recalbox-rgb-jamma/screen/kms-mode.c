#include <fcntl.h>
#include <kms-mode.h>
#include <libdrm/drm.h>
#include <stdio.h>
#include <unistd.h>
#include <xf86drmMode.h>
#include <xf86drmMode.h>
#include "log.h"

char *cards[3] = {"/dev/dri/card1", "/dev/dri/card0", "/dev/dri/card2"};


bool checkConnector(int fd, drmModeConnectorPtr connector, video_mode *res_mode) {
  if (connector->connection == DRM_MODE_CONNECTED && connector->encoder_id != 0) {
    drmModeEncoderPtr encoder = drmModeGetEncoder(fd, connector->encoder_id);
    _syslog(LOG_DEBUG, "current crtc :%d", encoder->crtc_id);
    drmModeCrtcPtr crct = drmModeGetCrtc(fd, encoder->crtc_id);
    drmModeModeInfo mode = crct->mode;
    _syslog(LOG_DEBUG, "current mode : %s", crct->mode.name);
    _syslog(LOG_DEBUG, "current mode :\nvrefresh : %d\nvdisplay : %d\nvscan : %d\nvtotal : %d",
           crct->mode.vrefresh, crct->mode.vdisplay, crct->mode.vscan, crct->mode.vtotal);
    _syslog(LOG_DEBUG, "current mode :\nhdisplay : %d\nhskew : %d", crct->mode.hdisplay, crct->mode.hskew);
    _syslog(LOG_DEBUG, "current mode :\ntype : %d\nclock : %d", crct->mode.type, crct->mode.clock);
    _syslog(LOG_DEBUG, "current mode :\nvsync : %d\nclock : %d", crct->mode.type, crct->mode.clock);
    float vrefresh = (mode.clock * 1000.0) / (mode.htotal * mode.vtotal) *
                     ((mode.flags & DRM_MODE_FLAG_INTERLACE) ? 2 : 1);
    unsigned int hrefresh = mode.clock * 1000.0 / mode.htotal;
    _syslog(LOG_DEBUG, "current vsync : %f", vrefresh);
    _syslog(LOG_DEBUG, "current hsync : %d", hrefresh);
    res_mode->hfreq = hrefresh;
    res_mode->vfreq = vrefresh;
    res_mode->width = mode.hdisplay;
    res_mode->height = mode.vdisplay;
    res_mode->interlaced = ((mode.flags & DRM_MODE_FLAG_INTERLACE) ? 1 : 0);
    drmModeFreeCrtc(crct);
    drmModeFreeEncoder(encoder);
    return true;
  }
  return false;
}

video_mode get_current_mode() {
  video_mode res_mode = {
      .width = 0,
      .height = 0,
      .interlaced = 0,
      .vfreq = 0,
      .hfreq = 0,
  };

  bool found = false;

  for (int card = 0; card < 3; card++) {
    if (!found) {
      int fd = open(cards[card], O_RDWR);
      if(fd < 0) continue;
      drmModeResPtr resource = drmModeGetResources(fd);
      if (resource != NULL) {
        _syslog(LOG_DEBUG, "Open card with %d connectors", resource->count_connectors);
        for (int connectorIndex = 0; connectorIndex < resource->count_connectors; ++connectorIndex) {
          drmModeConnectorPtr connector = drmModeGetConnector(fd, resource->connectors[connectorIndex]);
          if (connector != NULL) {
            _syslog(LOG_DEBUG, "Opening connector %d (%s) (%d)\n", connectorIndex,
                   (connector->connection == DRM_MODE_CONNECTED ? "Connected" : "Disconnected"),
                   resource->connectors[connectorIndex]);
            if (checkConnector(fd, connector, &res_mode)) {
              drmModeFreeConnector(connector);
              found = true;
              break;
            }
          }
          drmModeFreeConnector(connector);
        }
      }
      drmModeFreeResources(resource);
      close(fd);
    }
  }
  return res_mode;
}
