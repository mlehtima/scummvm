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

#include "backends/platform/qt/qt-window.h"

#include "common/textconsole.h"

#include "icons/scummvm.xpm"

QtWindow::QtWindow()
   : _window(nullptr),
     _inputGrabState(false),
     _windowCaption("ScummVM"),
     _lastFlags(0) {
}

QtWindow::~QtWindow() {
	delete _window;
	_window = nullptr;
}

void QtWindow::setWindowCaption(const Common::String &caption) {
	_windowCaption = caption;
	if (_window) {
		_window->setTitle(caption.c_str());
	}
}

void QtWindow::toggleMouseGrab() {
	if (_window) {
		_inputGrabState = !_inputGrabState;
		_window->setMouseGrabEnabled(_inputGrabState);
	}
}

bool QtWindow::hasMouseFocus() const {
/*
	if (_window) {
		return (SDL_GetWindowFlags(_window) & SDL_WINDOW_MOUSE_FOCUS);
	} else {
		return false;
	}
*/
	return true;
}

void QtWindow::warpMouseInWindow(uint x, uint y) {
	printf("QtWindow::warpMouseInWindow to %ix%i\n", x, y);
	QCursor::setPos(x, y);
/*
	if (_window && hasMouseFocus()) {

		QCursor c = _window->cursor();
		c.setPos(_window->mapToGlobal(QPoint(x, y)));
		c.setShape(Qt::BlankCursor);
		_window->setCursor(c);

	}
*/
}

void QtWindow::iconifyWindow() {
	if (_window) {
		_window->setMouseGrabEnabled(false);
		_window->showMinimized();
	}
}

bool QtWindow::createOrUpdateWindow(int width, int height, uint32 flags) {
	if (_inputGrabState) {
		_window->setMouseGrabEnabled(true);
	}

	QScreen *screen = QGuiApplication::primaryScreen();
	QSize screenSize;

	if (flags & Qt::WindowFullScreen) {
		screenSize = screen->size();
	} else {
		screenSize = screen->availableSize();
	}

	if (width > screenSize.width()) {
		width = screenSize.width();
	}

	if (height > screenSize.height()) {
		height = screenSize.height();
	}

	if (!_window) {
		_window = new QWindow();
		if (_window) {
			_window->moveToThread(QGuiApplication::instance()->thread());
			_window->setSurfaceType(QWindow::OpenGLSurface);
			_window->setGeometry(0, 0, width, height);
			_window->setIcon(QPixmap(scummvm_icon));
			_window->show();
			if (flags & Qt::WindowFullScreen) {
				_window->setWindowState(Qt::WindowFullScreen);
			}
		}
	} else {
		if (flags & Qt::WindowFullScreen) {
			_window->showFullScreen();
			_window->setKeyboardGrabEnabled(true);
			_window->setMouseGrabEnabled(true);
		} else {
			_window->resize(width, height);
			_window->showNormal();
			_window->setKeyboardGrabEnabled(false);
			_window->setMouseGrabEnabled(false);
		}
	}

	if (!_window) {
		return false;
	}

	_lastFlags = flags;

	return true;
}
