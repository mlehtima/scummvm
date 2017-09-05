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

#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "common/scummsys.h"

#if defined(QT_BACKEND)

#include "backends/events/qt/qt-events.h"
#include "backends/platform/qt/qt.h"
#include "backends/graphics/graphics.h"
#include "common/config-manager.h"
#include "common/textconsole.h"
#include "common/fs.h"

QtEventSource::QtEventSource()
    : EventSource(),
      _lastScreenID(0),
      _graphicsManager(0),
      _eventQueueLock(g_system->createMutex()),
      _sizeChanged(false),
      _newWidth(0),
      _newHeight(0)
      {
	_eventFilter = new QtEventHandler(this, _eventQueue, _eventQueueLock);
}

QtEventSource::~QtEventSource() {
	delete _eventFilter;
}

void QtEventSource::setGraphicsManager(OpenGLQtGraphicsManager *gMan) {
	_graphicsManager = gMan;
	_eventFilter->setGraphicsManager(gMan);
}

bool QtEventSource::pollEvent(Common::Event &event) {
	g_system->lockMutex(_eventQueueLock);

	if (_sizeChanged) {
		if (_graphicsManager) {
			_sizeChanged = false;
			_graphicsManager->notifyResize(_newWidth, _newHeight);
			int screenID = _graphicsManager->getScreenChangeID();
			if (screenID != _lastScreenID) {
				_lastScreenID = screenID;
				event.type = Common::EVENT_SCREEN_CHANGED;
				g_system->unlockMutex(_eventQueueLock);
				return true;
			}
		}
	}

	if (_eventQueue.empty()) {
		g_system->unlockMutex(_eventQueueLock);
		return false;
	}

	event = _eventQueue.pop();

	g_system->unlockMutex(_eventQueueLock);
	return true;
}

void QtEventSource::notifyResize(int width, int height) {
	_newWidth = width;
	_newHeight = height;
	_sizeChanged = true;
}

#endif
