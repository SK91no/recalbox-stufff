#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// globals
int verbose_flag = 0;

// x11 and drm custom linked lists
struct _x11_xrandr {
  struct _x11_xrandr *next;
  char *screen_name;
  long unsigned drm_connector_id;
  char *panel_orientation;
};
typedef struct _x11_xrandr x11_xrandr_t;

struct _drm_connector {
  struct _drm_connector *next;
  int card_id;
  long unsigned connector_id;
  char *connector_name;
  long unsigned panel_orientation;
};
typedef struct _drm_connector drm_connector_t;

// misc utility functions
void error(const char * fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
}
void verbose(const char * fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  if (verbose_flag)
    vfprintf(stderr, fmt, argptr);
  va_end(argptr);
}

// output connector name from a type
const char *drm_get_connector_name(uint32_t type) {
  switch (type) {
    case DRM_MODE_CONNECTOR_VGA: return "VGA";
    case DRM_MODE_CONNECTOR_DVII: return "DVI-I";
    case DRM_MODE_CONNECTOR_DVID: return "DVI-D";
    case DRM_MODE_CONNECTOR_DVIA: return "DVI-A";
    case DRM_MODE_CONNECTOR_Composite: return "Composite";
    case DRM_MODE_CONNECTOR_SVIDEO: return "SVIDEO";
    case DRM_MODE_CONNECTOR_LVDS: return "LVDS";
    case DRM_MODE_CONNECTOR_Component: return "Component";
    case DRM_MODE_CONNECTOR_9PinDIN: return "9PinDIN";
    case DRM_MODE_CONNECTOR_DisplayPort: return "DP";
    case DRM_MODE_CONNECTOR_HDMIA: return "HDMI-A";
    case DRM_MODE_CONNECTOR_HDMIB: return "HDMI-B";
    case DRM_MODE_CONNECTOR_TV: return "TV";
    case DRM_MODE_CONNECTOR_eDP: return "eDP";
    case DRM_MODE_CONNECTOR_VIRTUAL: return "Virtual";
    case DRM_MODE_CONNECTOR_DSI: return "DSI";
    case DRM_MODE_CONNECTOR_DPI: return "DPI";
    default: return "Unknown";
  }
}

// translate drm rotation to text
const char* get_orientation(long unsigned rotation) {
  switch (rotation) {
    case DRM_MODE_ROTATE_0:   return "Normal";
    case DRM_MODE_ROTATE_90:  return "Left Side Up";
    case DRM_MODE_ROTATE_180: return "Upside Down";
    case DRM_MODE_ROTATE_270: return "Right Side Up";
    default:                  return "Unknown";
  }
}

char * get_xrandr_property_value(Display *dpy, const void *value_bytes) {
  const Atom *val = value_bytes;
  char *str = XGetAtomName (dpy, *val);
  char *output = "";
  if (str != NULL)
  {
    output = strdup(str);
    XFree (str);
  }
  return output;
}

// create the linked list of x11 screens
x11_xrandr_t * get_x11_screens() {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop;

  Display *dpy = XOpenDisplay(NULL);
  x11_xrandr_t *x11_xrandr;
  x11_xrandr_t *x11_xrandr_prev=calloc(1, sizeof(x11_xrandr_t));
  x11_xrandr_t *x11_xrandr_first = x11_xrandr_prev;;

  if (dpy == NULL) {
    error("Can't connect to x11 display\n");
    return NULL;
  }

  // get default screen of the display
  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  // get XRandR resources
  XRRScreenResources *screen_res = XRRGetScreenResourcesCurrent(dpy, root);
  if (screen_res == NULL) {
    error("Impossible to get x11 screen resources");
    XCloseDisplay(dpy);
    return NULL;
  }

  // get all video output
  for (int i = 0; i < screen_res->noutput; i++) {
    XRROutputInfo *output_info = XRRGetOutputInfo(dpy, screen_res, screen_res->outputs[i]);
    if (output_info) {
      x11_xrandr = calloc(1, sizeof(x11_xrandr_t));
      x11_xrandr->screen_name = strdup(output_info->name);

      int nprops;
      Atom *props = XRRListOutputProperties(dpy, screen_res->outputs[i], &nprops);
      for (int j = 0; j < nprops; j++) {
        char *prop_name = XGetAtomName(dpy, props[j]);
        if (strcmp(prop_name, "panel orientation") == 0) {
          XRRGetOutputProperty(dpy, screen_res->outputs[i], props[j], 0, 100, False, False,
              AnyPropertyType, &actual_type, &actual_format,
              &nitems, &bytes_after, &prop);
          if (nitems > 0) {
            x11_xrandr->panel_orientation = get_xrandr_property_value (dpy, prop);
          }

          XFree(prop);
        }
        if (strcmp(prop_name, "CONNECTOR_ID") == 0) {
          XRRGetOutputProperty(dpy, screen_res->outputs[i], props[j], 0, 4, False, False,
              AnyPropertyType, &actual_type, &actual_format,
              &nitems, &bytes_after, &prop);

          if (nitems > 0) {
            x11_xrandr->drm_connector_id = *((int*) prop);
          }

          XFree(prop);
        }
        XFree(prop_name);
      }

      XFree(props);
      XRRFreeOutputInfo(output_info);
      x11_xrandr_prev->next = x11_xrandr;
      x11_xrandr_prev = x11_xrandr;
    }
  }
  XRRFreeScreenResources(screen_res);
  XCloseDisplay(dpy);
  return x11_xrandr_first->next;
}

// create the linked lists of drm_connectors_t
drm_connector_t * get_drm_connectors() {
  int fd;
  drmModeRes *resources;
  drmModeConnector *connector;
  drmModePropertyPtr property;
  drmModeObjectPropertiesPtr props;
  int i;
  int card = 0;
  char *card_name;
  drm_connector_t * connector_list;
  drm_connector_t * connector_prev = calloc(1, sizeof(drm_connector_t));;
  drm_connector_t * connector_first = connector_prev;

  while(true) {
    // Ouvrir le périphérique DRM
    verbose("Testing card %d\n", card);
    int len = asprintf(&card_name, "/dev/dri/card%d", card);
    fd = open(card_name, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
      verbose("Can't open %s: %s", card_name);
      break;
    }

    // Récupérer les ressources DRM
    resources = drmModeGetResources(fd);
    if (!resources) {
      error("drmModeGetResources failed for fd %d (%s)\n", fd, card_name);
      close(fd);
      continue;
    }

    // Parcourir les connecteurs
    for (i = 0; i < resources->count_connectors; i++) {
      connector = drmModeGetConnector(fd, resources->connectors[i]);
      if (!connector) {
        error("drmModeGetConnector failed\n");
        continue;
      }

      // Récupérer les propriétés du connecteur
      props = drmModeObjectGetProperties(fd, connector->connector_id, DRM_MODE_OBJECT_CONNECTOR);
      if (!props) {
        error("drmModeObjectGetProperties failed\n");
        drmModeFreeConnector(connector);
        continue;
      }

      connector_list = calloc(1, sizeof(drm_connector_t));
      connector_list->card_id = card;
      connector_list->connector_id = connector->connector_id;
      int len = asprintf(&connector_list->connector_name, "%s-%d", drm_get_connector_name(connector->connector_type), connector->connector_type_id);
      connector_prev->next = connector_list;
      connector_prev = connector_list;
      for (int j = 0; j < props->count_props; j++) {
        property = drmModeGetProperty(fd, props->props[j]);
        if (!property) {
          error("drmModeGetProperty failed\n");
          continue;
        }
        if (strcmp(property->name, "panel orientation") == 0) {
          connector_list->panel_orientation = props->prop_values[j];
        }
        drmModeFreeProperty(property);
      }
      drmModeFreeObjectProperties(props);
      drmModeFreeConnector(connector);
    }
    card++;
    drmModeFreeResources(resources);
    close(fd);
  }
  return connector_first->next;
}

// Those 2 functions used to link x11 <-> drm information
//const x11_xrandr_t * get_x11_screen_by_drm_connector_id(const x11_xrandr_t * x11_screen, const int id) {
//  while (x11_screen) {
//    if (x11_screen->drm_connector_id == id)
//      return x11_screen;
//    x11_screen = x11_screen->next;
//  }
//  return NULL;
//}
//
//const drm_connector_t * get_connector_by_id(const drm_connector_t * connector, const int id) {
//  while (connector) {
//    if (connector->connector_id == id)
//      return connector;
//    connector = connector->next;
//  }
//  return NULL;
//}

// dump gathered data
void dump_drm_connectors(const drm_connector_t * drm_connector) {
  while(drm_connector) {
    printf("DRM:%d.%s:%u:%s\n", drm_connector->card_id, drm_connector->connector_name, drm_connector->connector_id, get_orientation(1<<drm_connector->panel_orientation));
    drm_connector = drm_connector->next;
  }
}
void dump_x11_screens(const x11_xrandr_t * x11_screen) {
  while(x11_screen) {
    printf("X11:%s:%u:%s\n", x11_screen->screen_name, x11_screen->drm_connector_id, x11_screen->panel_orientation);
    x11_screen = x11_screen->next;
  }
}

static void show_usage() {
  printf("Usage: drm_info [ -hdxv ]\n");
  printf("\noutput format:\n");
  printf("-d (show drm connectors and panel orientation)\n");
  printf("DRM:0.eDP-1:1:Right Side Up\n\n");
  printf("-x (show x11 screens and panel orientation)\n");
  printf("X11:eDP:1:Right Side Up\n");
}

int main(int argc, char *argv[argc]) {
  drm_connector_t *drm_connectors;
  x11_xrandr_t *x11_screens;
  int drm_flag = 0;
  int x11_flag = 0;
  int c;
  // parse args
  while ((c = getopt (argc, argv, "hdxv")) != -1)
    switch (c) {
      case 'd':
        drm_flag = 1;
        break;
      case 'x':
        x11_flag = 1;
        break;
      case 'v':
        verbose_flag = 1;
        break;
      case 'h':
      default:
        show_usage();
        exit(1);
    }

  x11_screens = get_x11_screens();
  drm_connectors = get_drm_connectors();
  if (x11_flag)
    dump_x11_screens(x11_screens);
  if (drm_flag)
    dump_drm_connectors(drm_connectors);
  return EXIT_SUCCESS;
}
