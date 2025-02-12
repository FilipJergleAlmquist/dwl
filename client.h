/*
 * Attempt to consolidate unavoidable suck into one file, away from dwl.c.  This
 * file is not meant to be pretty.  We use a .h file with static inline
 * functions instead of a separate .c module, or function pointers like sway, so
 * that they will simply compile out if the chosen #defines leave them unused.
 */

/* Leave these functions first; they're used in the others */
static inline int client_is_x11(Client *c) {
#ifdef XWAYLAND
  return c->type == X11Managed || c->type == X11Unmanaged;
#else
  return 0;
#endif
}

static inline struct wlr_surface *client_surface(Client *c) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    return c->surface.xwayland->surface;
#endif
  return c->surface.xdg->surface;
}

static inline int toplevel_from_wlr_surface(struct wlr_surface *s, Client **pc,
                                            LayerSurface **pl) {
  struct wlr_xdg_surface *xdg_surface, *tmp_xdg_surface;
  struct wlr_surface *root_surface;
  struct wlr_layer_surface_v1 *layer_surface;
  Client *c = NULL;
  LayerSurface *l = NULL;
  int type = -1;
#ifdef XWAYLAND
  struct wlr_xwayland_surface *xsurface;
#endif

  if (!s)
    return -1;
  root_surface = wlr_surface_get_root_surface(s);

#ifdef XWAYLAND
  if ((xsurface = wlr_xwayland_surface_try_from_wlr_surface(root_surface))) {
    c = xsurface->data;
    type = c->type;
    goto end;
  }
#endif

  if ((layer_surface =
           wlr_layer_surface_v1_try_from_wlr_surface(root_surface))) {
    l = layer_surface->data;
    type = LayerShell;
    goto end;
  }

  xdg_surface = wlr_xdg_surface_try_from_wlr_surface(root_surface);
  while (xdg_surface) {
    tmp_xdg_surface = NULL;
    switch (xdg_surface->role) {
    case WLR_XDG_SURFACE_ROLE_POPUP:
      if (!xdg_surface->popup || !xdg_surface->popup->parent)
        return -1;

      tmp_xdg_surface =
          wlr_xdg_surface_try_from_wlr_surface(xdg_surface->popup->parent);

      if (!tmp_xdg_surface)
        return toplevel_from_wlr_surface(xdg_surface->popup->parent, pc, pl);

      xdg_surface = tmp_xdg_surface;
      break;
    case WLR_XDG_SURFACE_ROLE_TOPLEVEL:
      c = xdg_surface->data;
      type = c->type;
      goto end;
    case WLR_XDG_SURFACE_ROLE_NONE:
      return -1;
    }
  }

end:
  if (pl)
    *pl = l;
  if (pc)
    *pc = c;
  return type;
}

/* The others */
static inline void client_activate_surface(struct wlr_surface *s,
                                           int activated) {
  struct wlr_xdg_toplevel *toplevel;
#ifdef XWAYLAND
  struct wlr_xwayland_surface *xsurface;
  if ((xsurface = wlr_xwayland_surface_try_from_wlr_surface(s))) {
    wlr_xwayland_surface_activate(xsurface, activated);
    return;
  }
#endif
  if ((toplevel = wlr_xdg_toplevel_try_from_wlr_surface(s)))
    wlr_xdg_toplevel_set_activated(toplevel, activated);
}

static inline uint32_t client_set_bounds(Client *c, int32_t width,
                                         int32_t height) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    return 0;
#endif
  if (wl_resource_get_version(c->surface.xdg->toplevel->resource) >=
          XDG_TOPLEVEL_CONFIGURE_BOUNDS_SINCE_VERSION &&
      width >= 0 && height >= 0 &&
      (c->bounds.width != width || c->bounds.height != height)) {
    c->bounds.width = width;
    c->bounds.height = height;
    return wlr_xdg_toplevel_set_bounds(c->surface.xdg->toplevel, width, height);
  }
  return 0;
}

static inline const char *client_get_appid(Client *c) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    return c->surface.xwayland->class;
#endif
  return c->surface.xdg->toplevel->app_id;
}

static inline void client_get_clip(Client *c, struct wlr_box *clip) {
  struct wlr_box xdg_geom = {0};
  *clip = (struct wlr_box){
      .x = 0,
      .y = 0,
      .width = c->geom.width,
      .height = c->geom.height,
  };

#ifdef XWAYLAND
  if (client_is_x11(c))
    return;
#endif

  wlr_xdg_surface_get_geometry(c->surface.xdg, &xdg_geom);
  clip->x = xdg_geom.x;
  clip->y = xdg_geom.y;
}

static inline void client_get_geometry(Client *c, struct wlr_box *geom) {
#ifdef XWAYLAND
  if (client_is_x11(c)) {
    geom->x = c->surface.xwayland->x;
    geom->y = c->surface.xwayland->y;
    geom->width = c->surface.xwayland->width;
    geom->height = c->surface.xwayland->height;
    return;
  }
#endif
  wlr_xdg_surface_get_geometry(c->surface.xdg, geom);
}

static inline Client *client_get_parent(Client *c) {
  Client *p = NULL;
#ifdef XWAYLAND
  if (client_is_x11(c) && c->surface.xwayland->parent)
    toplevel_from_wlr_surface(c->surface.xwayland->parent->surface, &p, NULL);
#endif
  if (c->surface.xdg->toplevel->parent)
    toplevel_from_wlr_surface(c->surface.xdg->toplevel->parent->base->surface,
                              &p, NULL);

  return p;
}

static inline const char *client_get_title(Client *c) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    return c->surface.xwayland->title;
#endif
  return c->surface.xdg->toplevel->title;
}

static inline pid_t client_get_pid(Client *c) {
  pid_t pid;
  struct wlr_xwayland_surface *xwayland_surf;
  struct wlr_xdg_surface *xdg_surf;
  struct wl_client *client;
#ifdef XWAYLAND
  xcb_connection_t *xc;
  xcb_get_property_cookie_t cookie;
  xcb_get_property_reply_t *reply;
  int err;
  if (client_is_x11(c)) {
    xwayland_surf = c->surface.xwayland;
    xc = xcb_connect(c->display_name, NULL);
    err = xcb_connection_has_error(xc);
    if (err) {
      wlr_log(WLR_ERROR,
              "xcb_connect to X server failed with code %d\n. Return old PID\n",
              err);
      return xwayland_surf->pid;
    }
    cookie = xcb_get_property(xc, 0, xwayland_surf->window_id,
                              netatom[NetWMPid], XCB_ATOM_CARDINAL, 0, 1);
    reply = xcb_get_property_reply(xc, cookie, NULL);

    if (reply != NULL) {
      if (xcb_get_property_value_length(reply) == sizeof(uint32_t)) {
        uint32_t *upid = (uint32_t *)xcb_get_property_value(reply);
        xwayland_surf->pid = *upid;
      }
      free(reply);
    }
    xcb_disconnect(xc);
    return xwayland_surf->pid;
  }
#endif
  xdg_surf = c->surface.xdg;
  client = wl_resource_get_client(xdg_surf->resource);
  wl_client_get_credentials(client, &pid, NULL, NULL);
  return pid;
}

static inline int client_is_float_type(Client *c) {
  struct wlr_xdg_toplevel *toplevel;
  struct wlr_xdg_toplevel_state state;

#ifdef XWAYLAND
  if (client_is_x11(c)) {
    struct wlr_xwayland_surface *surface = c->surface.xwayland;
    xcb_size_hints_t *size_hints = surface->size_hints;
    size_t i;
    if (surface->modal)
      return 1;

    for (i = 0; i < surface->window_type_len; i++)
      if (surface->window_type[i] == netatom[NetWMWindowTypeDialog] ||
          surface->window_type[i] == netatom[NetWMWindowTypeSplash] ||
          surface->window_type[i] == netatom[NetWMWindowTypeToolbar] ||
          surface->window_type[i] == netatom[NetWMWindowTypeUtility])
        return 1;

    return size_hints && size_hints->min_width > 0 &&
           size_hints->min_height > 0 &&
           (size_hints->max_width == size_hints->min_width ||
            size_hints->max_height == size_hints->min_height);
  }
#endif

  toplevel = c->surface.xdg->toplevel;
  state = toplevel->current;
  return toplevel->parent || (state.min_width != 0 && state.min_height != 0 &&
                              (state.min_width == state.max_width ||
                               state.min_height == state.max_height));
}

static inline int client_is_rendered_on_mon(Client *c, Monitor *m) {
  /* This is needed for when you don't want to check formal assignment,
   * but rather actual displaying of the pixels.
   * Usually VISIBLEON suffices and is also faster. */
  struct wlr_surface_output *s;
  int unused_lx, unused_ly;
  if (!wlr_scene_node_coords(&c->scene->node, &unused_lx, &unused_ly))
    return 0;
  wl_list_for_each(s, &client_surface(c)->current_outputs,
                   link) if (s->output == m->wlr_output) return 1;
  return 0;
}

static inline int client_is_stopped(Client *c) {
  int pid;
  siginfo_t in = {0};
#ifdef XWAYLAND
  if (client_is_x11(c))
    return 0;
#endif

  wl_client_get_credentials(c->surface.xdg->client->client, &pid, NULL, NULL);
  if (waitid(P_PID, pid, &in, WNOHANG | WCONTINUED | WSTOPPED | WNOWAIT) < 0) {
    /* This process is not our child process, while is very unluckely that
     * it is stopped, in order to do not skip frames assume that it is. */
    if (errno == ECHILD)
      return 1;
  } else if (in.si_pid) {
    if (in.si_code == CLD_STOPPED || in.si_code == CLD_TRAPPED)
      return 1;
    if (in.si_code == CLD_CONTINUED)
      return 0;
  }

  return 0;
}

static inline int client_is_unmanaged(Client *c) {
#ifdef XWAYLAND
  return c->type == X11Unmanaged;
#endif
  return 0;
}

static inline void client_notify_enter(struct wlr_surface *s,
                                       struct wlr_seat *seat) {
  struct wlr_keyboard *kb = wlr_seat_get_keyboard(seat);
  wlr_log(WLR_DEBUG, "Clientnotifyenter, surface: %p, kb: %p, seat: %p\n", s,
          kb, seat);
  if (kb)
    wlr_seat_keyboard_notify_enter(seat, s, kb->keycodes, kb->num_keycodes,
                                   &kb->modifiers);
  else
    wlr_seat_keyboard_notify_enter(seat, s, NULL, 0, NULL);
}

static inline void client_restack_surface(Client *c) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    wlr_xwayland_surface_restack(c->surface.xwayland, NULL,
                                 XCB_STACK_MODE_ABOVE);
#endif
  return;
}

static inline void client_send_close(Client *c) {
#ifdef XWAYLAND
  if (client_is_x11(c)) {
    wlr_xwayland_surface_close(c->surface.xwayland);
    return;
  }
#endif
  wlr_xdg_toplevel_send_close(c->surface.xdg->toplevel);
}

static inline void client_set_fullscreen(Client *c, int fullscreen) {
#ifdef XWAYLAND
  if (client_is_x11(c)) {
    wlr_xwayland_surface_set_fullscreen(c->surface.xwayland, fullscreen);
    return;
  }
#endif
  wlr_xdg_toplevel_set_fullscreen(c->surface.xdg->toplevel, fullscreen);
}

static inline uint32_t client_set_size(Client *c, uint32_t width,
                                       uint32_t height) {
#ifdef XWAYLAND
  if (client_is_x11(c)) {
    wlr_xwayland_surface_configure(c->surface.xwayland, c->geom.x, c->geom.y,
                                   width, height);
    return 0;
  }
#endif
  if (width == c->surface.xdg->toplevel->current.width &&
      height == c->surface.xdg->toplevel->current.height)
    return 0;
  return wlr_xdg_toplevel_set_size(c->surface.xdg->toplevel, width, height);
}

static inline void client_set_tiled(Client *c, uint32_t edges) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    return;
#endif
  if (wl_resource_get_version(c->surface.xdg->resource) >=
      XDG_TOPLEVEL_STATE_TILED_RIGHT_SINCE_VERSION) {
    wlr_xdg_toplevel_set_tiled(c->surface.xdg->toplevel, edges);
  } else {
    wlr_xdg_toplevel_set_maximized(c->surface.xdg->toplevel,
                                   edges != WLR_EDGE_NONE);
  }
}

static inline void client_set_suspended(Client *c, int suspended) {
#ifdef XWAYLAND
  if (client_is_x11(c)) {
    wlr_xwayland_surface_set_withdrawn(c->surface.xwayland, suspended);
    return;
  }
#endif

  wlr_xdg_toplevel_set_suspended(c->surface.xdg->toplevel, suspended);
}

static inline int client_wants_focus(Client *c) {
#ifdef XWAYLAND
  return client_is_unmanaged(c) &&
         wlr_xwayland_or_surface_wants_focus(c->surface.xwayland) &&
         wlr_xwayland_icccm_input_model(c->surface.xwayland) !=
             WLR_ICCCM_INPUT_MODEL_NONE;
#endif
  return 0;
}

static inline int client_wants_fullscreen(Client *c) {
#ifdef XWAYLAND
  if (client_is_x11(c))
    return c->surface.xwayland->fullscreen;
#endif
  return c->surface.xdg->toplevel->requested.fullscreen;
}
