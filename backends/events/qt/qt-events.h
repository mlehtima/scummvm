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

#ifndef BACKEND_EVENTS_QT_H
#define BACKEND_EVENTS_QT_H

#include "backends/platform/qt/qt-sys.h"
#include "backends/events/qt/qt-eventfilter.h"
#include "backends/graphics/qt/qt-graphics.h"

#include "common/events.h"

// multiplier used to increase resolution for keyboard/joystick mouse
#define MULTIPLIER 16

/**
 * The Qt event source.
 */
class QtEventSource : public Common::EventSource {
public:
	QtEventSource();
	virtual ~QtEventSource();

	void setGraphicsManager(OpenGLQtGraphicsManager *gMan);

	/**
	 * Gets and processes Qt events.
	 */
	virtual bool pollEvent(Common::Event &event);

	/**
	 * Resets keyboard emulation after a video screen change
	 */
//	virtual void resetKeyboardEmulation(int16 x_max, int16 y_max);

	/**
	 * Emulates a mouse movement that would normally be caused by a mouse warp
	 * of the system mouse.
	 */
	void fakeWarpMouse(const int x, const int y);

	void notifyResize(int width, int height);

protected:
	/** Last screen id for checking if it was modified */
	int _lastScreenID;

	/**
	 * The associated graphics manager.
	 */
	OpenGLQtGraphicsManager *_graphicsManager;

	/**
	 * Whether _fakeMouseMove contains an event we need to send.
	 */
	bool _queuedFakeMouseMove;

	/**
	 * A fake mouse motion event sent when the graphics manager is told to warp
	 * the mouse but the system mouse is unable to be warped (e.g. because the
	 * window is not focused).
	 */
	Common::Event _fakeMouseMove;

private:
	QtEventHandler *_eventFilter;

	/**
	 * Event queue for events from Qt
	 */
	Common::Queue<Common::Event> _eventQueue;

	/**
	 * Lock for Qt event queue
	 */
	OSystem::MutexRef _eventQueueLock;

	/**
	 * Whether size has changed
	 */
	bool _sizeChanged;

	/**
	 * New window width when size has changed
	 */
	uint _newWidth;

	/**
	 * New window height when size has changed
	 */
	uint _newHeight;
};

#endif
