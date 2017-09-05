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

#ifndef BACKENDS_PLATFORM_QT_WINDOW_H
#define BACKENDS_PLATFORM_QT_WINDOW_H

#include "backends/platform/qt/qt-sys.h"

#include "common/events.h"
#include "common/str.h"

class OpenGLQtGraphicsManager;

class QtWindow {
public:
	QtWindow();
	virtual ~QtWindow();

	/**
	 * Change the caption of the window.
	 *
	 * @param caption New window caption in UTF-8 encoding.
	 */
	void setWindowCaption(const Common::String &caption);

	/**
	 * Toggle mouse grab state. This decides whether the cursor can leave the
	 * window or not.
	 */
	void toggleMouseGrab();

	/**
	 * Check whether the application has mouse focus.
	 */
	bool hasMouseFocus() const;

	/**
	 * Warp the mouse to the specified position in window coordinates.
	 */
	void warpMouseInWindow(uint x, uint y);

	/**
	 * Iconifies the window.
	 */
	void iconifyWindow();

public:
	/**
	 * @return The window ScummVM has setup with Qt.
	 */
	QWindow *getQWindow() const { return _window; }

	/**
	 * Creates or updates the Qt window.
	 *
	 * @param width   Width of the window.
	 * @param height  Height of the window.
	 * @param flags   Qt flags used after window creation
	 * @return true on success, false otherwise
	 */
	bool createOrUpdateWindow(int width, int height, uint32 flags);

protected:
	/**
	 * QWindow
	 */
	QWindow *_window;

private:
	uint32 _lastFlags;
	bool _inputGrabState;
	Common::String _windowCaption;
};

class QtIconlessWindow : public QtWindow {
public:
	virtual void setupIcon() {}
};

#endif
