/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#include "utils_p.h"
#include <qtcurve-utils/number.h>

QTC_EXPORT void
qtcCairoRegion(cairo_t *cr, const cairo_region_t *region)
{
    int n_boxes = cairo_region_num_rectangles(region);
    QtcRect box;
    for (int i = 0;i < n_boxes;i++) {
        cairo_region_get_rectangle(region, i, &box);
        cairo_rectangle(cr, box.x, box.y, box.width, box.height);
    }
}

QTC_EXPORT void
qtcCairoClipRegion(cairo_t *cr, const cairo_region_t *region)
{
    cairo_new_path(cr);
    if (qtcLikely(region)) {
        qtcCairoRegion(cr, region);
        cairo_clip(cr);
    }
}

QTC_EXPORT void
qtcCairoClipRectangle(cairo_t *cr, const QtcRect *rect)
{
    cairo_new_path(cr);
    cairo_rectangle(cr, rect->x, rect->y, rect->width, rect->height);
    cairo_clip(cr);
}

QTC_EXPORT void
qtcCairoSetColor(cairo_t *cr, const GdkColor *col, double a)
{
    cairo_set_source_rgba(cr, col->red / 65535.0, col->green / 65535.0,
                          col->blue / 65535.0, a);
}

QTC_EXPORT void
qtcCairoPatternAddColorStop(cairo_pattern_t *pt, double offset,
                            const GdkColor *col, double a)
{
    cairo_pattern_add_color_stop_rgba(pt, offset, col->red / 65535.0,
                                      col->green / 65535.0,
                                      col->blue / 65535.0, a);
}

QTC_EXPORT void
qtcRectConstrain(QtcRect *rect, const QtcRect *con)
{
    if (rect && con) {
        if (rect->x < con->x) {
            rect->width -= con->x - rect->x;
            rect->x = con->x;
        }
        if(rect->y < con->y) {
            rect->height -= rect->y - con->y;
            rect->y = con->y;
        }
        if ((rect->x + rect->width) > (con->x + con->width)) {
            rect->width -= (rect->x + rect->width) - (con->x + con->width);
        }
        if ((rect->y + rect->height) > (con->y + con->height)) {
            rect->height -= (rect->y + rect->height) - (con->y + con->height);
        }
    }
}

QTC_EXPORT void
qtcRectUnion(const QtcRect *src1, const QtcRect *src2, QtcRect *dest)
{
    int dest_x;
    int dest_y;
    dest_x = qtcMin(src1->x, src2->x);
    dest_y = qtcMin(src1->y, src2->y);
    dest->width = qtcMax(src1->x + src1->width,
                         src2->x + src2->width) - dest_x;
    dest->height = qtcMax(src1->y + src1->height,
                          src2->y + src2->height) - dest_y;
    dest->x = dest_x;
    dest->y = dest_y;
}