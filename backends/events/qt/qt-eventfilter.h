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

#ifndef BACKENDS_EVENTS_QT_EVENTFILTER_H
#define BACKENDS_EVENTS_QT_EVENTFILTER_H

#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "backends/platform/qt/qt-sys.h"
#include "backends/graphics/qt/qt-graphics.h"

class QtEventSource;

/**
 * Qt event filter to capture Qt events.
 */
class QtEventHandler : public QObject {
	Q_OBJECT
public:
	QtEventHandler(QtEventSource *eventSource,
                   Common::Queue<Common::Event> &eventQueue,
                   OSystem::MutexRef &eventQueueLock);
	virtual ~QtEventHandler();

	void setGraphicsManager(OpenGLQtGraphicsManager *gMan) { _graphicsManager = gMan; }

protected:
	/** @name Event Handlers
	 * Handlers for specific Qt events.
	 */
	//@{

	bool eventFilter(QObject *obj, QEvent *ev);
	void handleCloseEvent(QCloseEvent *ev);
	void handleKeyEvent(QKeyEvent *ev);
	void handleMouseEvent(QMouseEvent *ev);
	void handleResizeEvent(QResizeEvent *ev);
	void handleWheelEvent(QWheelEvent *ev);

	//@}

private:
	/**
	 * Configures the key modifiers flags status
	 */
	void QtModToOSystemKeyFlags(Qt::KeyboardModifiers mod, Common::Event &event);

	/**
	 * Translates Qt key codes to OSystem key codes
	 */
	Common::KeyCode QtToOSystemKeycode(int key);

	QtEventSource *_eventSource;
	Common::Queue<Common::Event> &_eventQueue;
	OSystem::MutexRef &_eventQueueLock;

	/**
	 * The associated graphics manager.
	 */
	OpenGLQtGraphicsManager *_graphicsManager;
};


#endif
