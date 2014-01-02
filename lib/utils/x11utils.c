/*****************************************************************************
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

// TODO multi screen?

#include "x11utils.h"
#include "x11shadow_p.h"
#include "log.h"

#include <X11/Xlib-xcb.h>

static Display *qtc_disp = NULL;
static xcb_connection_t *qtc_xcb_conn = NULL;
static int qtc_default_screen_no = -1;
static xcb_window_t qtc_root_window = {0};
static xcb_screen_t *qtc_default_screen = NULL;
static char wm_cm_s_atom_name[100] = "_NET_WM_CM_S";

QTC_EXPORT xcb_atom_t qtc_x11_net_wm_moveresize;
QTC_EXPORT xcb_atom_t qtc_x11_net_wm_cm_s_default;
QTC_EXPORT xcb_atom_t qtc_x11_kde_net_wm_skip_shadow;
QTC_EXPORT xcb_atom_t qtc_x11_kde_net_wm_force_shadow;
QTC_EXPORT xcb_atom_t qtc_x11_kde_net_wm_shadow;
QTC_EXPORT xcb_atom_t qtc_x11_kde_net_wm_blur_behind_region;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_menubar_size;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_statusbar;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_titlebar_size;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_active_window;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_toggle_menubar;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_toggle_statusbar;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_opacity;
QTC_EXPORT xcb_atom_t qtc_x11_qtc_bgnd;
static xcb_atom_t qtc_x11_xembed_info;

static const struct {
    xcb_atom_t *atom;
    const char *name;
} qtc_x11_atoms[] = {
    {&qtc_x11_net_wm_moveresize, "_NET_WM_MOVERESIZE"},
    {&qtc_x11_net_wm_cm_s_default, wm_cm_s_atom_name},
    {&qtc_x11_kde_net_wm_skip_shadow, "_KDE_NET_WM_SKIP_SHADOW"},
    {&qtc_x11_kde_net_wm_force_shadow, "_KDE_NET_WM_FORCE_SHADOW"},
    {&qtc_x11_kde_net_wm_shadow, "_KDE_NET_WM_SHADOW"},
    {&qtc_x11_kde_net_wm_blur_behind_region, "_KDE_NET_WM_BLUR_BEHIND_REGION"},
    {&qtc_x11_qtc_menubar_size, "_QTCURVE_MENUBAR_SIZE_"},
    {&qtc_x11_qtc_statusbar, "_QTCURVE_STATUSBAR_"},
    {&qtc_x11_qtc_titlebar_size, "_QTCURVE_TITLEBAR_SIZE_"},
    {&qtc_x11_qtc_active_window, "_QTCURVE_ACTIVE_WINDOW_"},
    {&qtc_x11_qtc_toggle_menubar, "_QTCURVE_TOGGLE_MENUBAR_"},
    {&qtc_x11_qtc_toggle_statusbar, "_QTCURVE_TOGGLE_STATUSBAR_"},
    {&qtc_x11_qtc_opacity, "_QTCURVE_OPACITY_"},
    {&qtc_x11_qtc_bgnd, "_QTCURVE_BGND_"},
    {&qtc_x11_xembed_info, "_XEMBED_INFO"}
};
#define QTC_X11_ATOM_N (sizeof(qtc_x11_atoms) / sizeof(qtc_x11_atoms[0]))

QTC_EXPORT xcb_window_t
(qtcX11RootWindow)(int scrn_no)
{
    if (scrn_no < 0 || scrn_no == qtc_default_screen_no) {
        return qtc_root_window;
    }
    return qtcX11GetScreen()->root;
}

QTC_EXPORT void
qtcX11FlushXlib()
{
    if (qtc_disp) {
        XFlush(qtc_disp);
    }
}

QTC_EXPORT int
qtcX11DefaultScreenNo()
{
    return qtc_default_screen_no;
}

QTC_EXPORT xcb_screen_t*
qtcX11DefaultScreen()
{
    return qtc_default_screen;
}

static xcb_screen_t*
screen_of_display(xcb_connection_t *c, int screen)
{
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator(xcb_get_setup(c));
    for (;iter.rem;--screen, xcb_screen_next(&iter)) {
        if (screen == 0) {
            return iter.data;
        }
    }
    return NULL;
}

static void
qtcX11AtomsInit()
{
    xcb_connection_t *conn = qtc_xcb_conn;
    xcb_intern_atom_cookie_t cookies[QTC_X11_ATOM_N];
    for (size_t i = 0;i < QTC_X11_ATOM_N;i++) {
        cookies[i] = xcb_intern_atom(conn, 0, strlen(qtc_x11_atoms[i].name),
                                     qtc_x11_atoms[i].name);
    }
    for (size_t i = 0;i < QTC_X11_ATOM_N;i++) {
        xcb_intern_atom_reply_t *r =
            xcb_intern_atom_reply(conn, cookies[i], NULL);
        if (qtcLikely(r)) {
            *qtc_x11_atoms[i].atom = r->atom;
            free(r);
        } else {
            *qtc_x11_atoms[i].atom = 0;
        }
    }
}

QTC_EXPORT xcb_screen_t*
(qtcX11GetScreen)(int screen_no)
{
    if (screen_no == -1 || screen_no == qtc_default_screen_no) {
        return qtc_default_screen;
    }
    if (qtcUnlikely(!qtc_xcb_conn)) {
        return NULL;
    }
    return screen_of_display(qtc_xcb_conn, screen_no);
}

QTC_EXPORT void
qtcX11InitXcb(xcb_connection_t *conn, int screen_no)
{
    if (qtcUnlikely(qtc_xcb_conn) || !conn) {
        return;
    }
    if (screen_no < 0) {
        screen_no = 0;
    }
    qtc_xcb_conn = conn;
    qtc_default_screen_no = screen_no;
    qtc_default_screen = screen_of_display(conn, screen_no);
    if (qtc_default_screen) {
        qtc_root_window = qtc_default_screen->root;
    }
    const size_t base_len = strlen("_NET_WM_CM_S");
    sprintf(wm_cm_s_atom_name + base_len, "%d", screen_no);
    qtcX11AtomsInit();
    qtcX11ShadowInit();
}

QTC_EXPORT void
qtcX11InitXlib(Display *disp)
{
    if (qtcUnlikely(qtc_xcb_conn) || !disp) {
        return;
    }
    qtc_disp = disp;
    qtcX11InitXcb(XGetXCBConnection(disp), DefaultScreen(disp));
}

QTC_EXPORT xcb_connection_t*
qtcX11GetConn()
{
    return qtc_xcb_conn;
}

QTC_EXPORT Display*
qtcX11GetDisp()
{
    return qtc_disp;
}

QTC_EXPORT void
qtcX11Flush()
{
    if (qtcLikely(qtc_xcb_conn)) {
        xcb_flush(qtc_xcb_conn);
    }
}

QTC_EXPORT uint32_t
qtcX11GenerateId()
{
    if (qtcLikely(qtc_xcb_conn)) {
        return xcb_generate_id(qtc_xcb_conn);
    }
    return 0;
}

QTC_EXPORT int32_t
qtcX11GetShortProp(xcb_window_t win, xcb_atom_t atom)
{
    if (qtcUnlikely(!win)) {
        return -1;
    }
    int32_t res = -1;
    xcb_get_property_reply_t *reply =
        qtcX11Call(get_property, 0, win, atom, XCB_ATOM_CARDINAL, 0, 1);
    if (!reply) {
        return -1;
    }
    if (xcb_get_property_value_length(reply) > 0) {
        uint32_t val = *(int32_t*)xcb_get_property_value(reply);
        if (val < 512) {
            res = val;
        }
    }
    free(reply);
    return res;
}

QTC_EXPORT void
qtcX11MapRaised(xcb_window_t win)
{
    if (qtcUnlikely(!win)) {
        return;
    }
    static const uint32_t val = XCB_STACK_MODE_ABOVE;
    qtcX11CallVoid(configure_window, win, XCB_CONFIG_WINDOW_STACK_MODE, &val);
    qtcX11CallVoid(map_window, win);
}

QTC_EXPORT bool
qtcX11CompositingActive()
{
    xcb_get_selection_owner_reply_t *reply =
        qtcX11Call(get_selection_owner, qtc_x11_net_wm_cm_s_default);
    if (!reply) {
        return false;
    }
    bool res = (reply->owner != 0);
    free(reply);
    return res;
}

QTC_EXPORT bool
qtcX11HasAlpha(xcb_window_t win)
{
    if (qtcUnlikely(!win)) {
        return false;
    }
    if (!qtcX11CompositingActive()) {
        return false;
    }
    if (!win) {
        return true;
    }
    xcb_get_geometry_reply_t *reply = qtcX11Call(get_geometry, win);
    if (!reply) {
        return false;
    }
    bool res = (reply->depth == 32);
    free(reply);
    return res;
}

QTC_EXPORT bool
qtcX11IsEmbed(xcb_window_t win)
{
    if (qtcUnlikely(!win)) {
        return false;
    }
    xcb_get_property_reply_t *reply =
        qtcX11Call(get_property, 0, win, qtc_x11_xembed_info,
                   qtc_x11_xembed_info, 0, 1);
    if (!reply) {
        return false;
    }
    bool res = xcb_get_property_value_length(reply) > 0;
    free(reply);
    return res;
}