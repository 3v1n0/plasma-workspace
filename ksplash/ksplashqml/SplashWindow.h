/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SPLASH_WINDOW_H_
#define SPLASH_WINDOW_H_

#include <QQuickView>

#include "config-ksplashqml.h"

class QResizeEvent;
class QMouseEvent;
class QKeyEvent;

#if HAVE_WAYLAND
class WaylandSurface;
#endif

class SplashWindow: public QQuickView
{
public:
    SplashWindow(bool testing, bool window);
    ~SplashWindow();

    void setStage(int stage);
    virtual void setGeometry(const QRect &rect);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);

private:
    int m_stage;
    bool m_testing;
    bool m_window;

#if HAVE_WAYLAND
    WaylandSurface *m_plasmaSurface;
#endif
};

#endif // SPLASH_WINDOW_H_
