/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef BACKENDS_GRAPHICS_QT_QTGRAPHICS_H
#define BACKENDS_GRAPHICS_QT_QTGRAPHICS_H

#include "backends/graphics/opengl/opengl-graphics.h"
#include "backends/platform/qt/qt-sys.h"
#include "backends/platform/qt/qt-window.h"

#include "common/array.h"
#include "common/events.h"

#define USE_OSD	1

class QtEventSource;

class OpenGLQtGraphicsManager : public OpenGL::OpenGLGraphicsManager, virtual public WindowedGraphicsManager, public Common::EventObserver {
public:
	OpenGLQtGraphicsManager(uint desktopWidth, uint desktopHeight, QtEventSource *source, QtWindow *window);
	virtual ~OpenGLQtGraphicsManager();

	// GraphicsManager API
	virtual void activateManager();
	virtual void deactivateManager();

	virtual bool hasFeature(OSystem::Feature f) const override;
	virtual void setFeatureState(OSystem::Feature f, bool enable) override;
	virtual bool getFeatureState(OSystem::Feature f) const override;

	virtual const OSystem::GraphicsMode *getSupportedStretchModes() const override;
	virtual int getDefaultStretchMode() const override;
	virtual bool setStretchMode(int mode) override;
	virtual int getStretchMode() const override;

#ifdef USE_RGB_COLOR
	virtual Common::List<Graphics::PixelFormat> getSupportedFormats() const override;
#endif

	virtual void updateScreen() override;

	// EventObserver API
	virtual bool notifyEvent(const Common::Event &event) override;

	virtual void notifyResize(const int width, const int height);

	/**
	 * Notifies the graphics manager about a mouse position change.
	 *
	 * The passed point *must* be converted from window coordinates to virtual
	 * coordinates in order for the event to be processed correctly by the game
	 * engine. Just use `convertWindowToVirtual` for this unless you need to do
	 * something special.
	 *
	 * @param mouse The mouse position in window coordinates, which must be
	 * converted synchronously to virtual coordinates.
	 * @returns true if the mouse was in a valid position for the game and
	 * should cause the event to be sent to the game.
	 */
	virtual bool notifyMousePosition(Common::Point &mouse);

	virtual bool showMouse(const bool visible) override;

protected:
	virtual bool loadVideoMode(uint requestedWidth, uint requestedHeight, const Graphics::PixelFormat &format) override;

	virtual void refreshScreen() override;

	virtual void *getProcAddress(const char *name) const override;

	virtual void setSystemMousePosition(const int x, const int y) {};

	virtual void handleResizeImpl(const int width, const int height) override;

	virtual int getGraphicsModeScale(int mode) const { return 1; }

private:
	bool setupMode(uint width, uint height);

	uint _graphicsScale;
	int _stretchMode;
	bool _ignoreLoadVideoMode;
	bool _gotResize;

	bool _wantsFullScreen;

	uint _desiredFullscreenWidth;
	uint _desiredFullscreenHeight;

	bool isHotkey(const Common::Event &event) const;

	uint32 _lastFlags;

	QtEventSource *_eventSource;

	QOpenGLContext *_glContext;
	QtWindow *_window;

protected:
	bool createOrUpdateWindow(const int width, const int height, const uint32 flags);
};

#endif
