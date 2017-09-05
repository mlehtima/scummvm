
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

#if defined(QT_BACKEND)

#include "backends/events/qt/qt-eventfilter.h"
#include "backends/events/qt/qt-events.h"

QtEventHandler::QtEventHandler(QtEventSource *eventSource,
                               Common::Queue<Common::Event> &eventQueue,
                               OSystem::MutexRef &eventQueueLock)
    : _eventSource(eventSource),
      _eventQueue(eventQueue),
      _eventQueueLock(eventQueueLock) {
	this->moveToThread(QGuiApplication::instance()->thread());
	QGuiApplication::instance()->installEventFilter(this);
}

QtEventHandler::~QtEventHandler() {
}

bool QtEventHandler::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease) {
		handleKeyEvent(static_cast<QKeyEvent *>(ev));
		return true;
	} else if (ev->type() == QEvent::MouseMove || ev->type() == QEvent::MouseButtonPress || ev->type() == QEvent::MouseButtonRelease) {
		handleMouseEvent(static_cast<QMouseEvent *>(ev));
		return true;
	} else if (ev->type() == QEvent::Wheel) {
		handleWheelEvent(static_cast<QWheelEvent *>(ev));
		return true;
	} else if (ev->type() == QEvent::Resize) {
		handleResizeEvent(static_cast<QResizeEvent *>(ev));
		return true;
	} else if (ev->type() == QEvent::Close) {
		handleCloseEvent(static_cast<QCloseEvent *>(ev));
		return true;
	} else {
		// standard event processing
		return QObject::eventFilter(obj, ev);
	}
}

void QtEventHandler::handleCloseEvent(QCloseEvent *ev) {
	Common::Event e;
	e.type = Common::EVENT_QUIT;

	g_system->lockMutex(_eventQueueLock);
	_eventQueue.push(e);
	g_system->unlockMutex(_eventQueueLock);
}

void QtEventHandler::handleKeyEvent(QKeyEvent *ev) {
	Common::Event e;
	if (ev->type() == QEvent::KeyPress) {
//		printf("key down %c %s\n", ev->key(), ev->text().toStdString().c_str());
		e.type = Common::EVENT_KEYDOWN;
	} else {
//		printf("key up %c %s\n", ev->key(), ev->text().toStdString().c_str());
		e.type = Common::EVENT_KEYUP;
	}
	e.kbd.keycode = QtToOSystemKeycode(ev->key());
	e.kbd.ascii = !ev->text().isEmpty() ? ev->text().at(0).unicode() : 0;
	e.kbdRepeat = ev->isAutoRepeat();
	QtModToOSystemKeyFlags(ev->modifiers(), e);
	g_system->lockMutex(_eventQueueLock);
	_eventQueue.push(e);
	g_system->unlockMutex(_eventQueueLock);
}

void QtEventHandler::handleMouseEvent(QMouseEvent *ev) {
	Common::Event e;
	if (ev->type() == QEvent::MouseMove) {
		e.type = Common::EVENT_MOUSEMOVE;
	} else if (ev->type() == QEvent::MouseButtonPress) {
		switch (ev->button()) {
		case Qt::LeftButton:
			e.type = Common::EVENT_LBUTTONDOWN;
			break;
		case Qt::RightButton:
			e.type = Common::EVENT_RBUTTONDOWN;
			break;
		case Qt::MiddleButton:
			e.type = Common::EVENT_MBUTTONDOWN;
			break;
		default:
			return;
		}
	} else if (ev->type() == QEvent::MouseButtonRelease) {
		switch (ev->button()) {
		case Qt::LeftButton:
			e.type = Common::EVENT_LBUTTONUP;
			break;
		case Qt::RightButton:
			e.type = Common::EVENT_RBUTTONUP;
			break;
		case Qt::MiddleButton:
			e.type = Common::EVENT_MBUTTONUP;
			break;
		default:
			return;
		}
	}
	e.mouse = Common::Point(ev->x(), ev->y());

	if (_graphicsManager) {
		_graphicsManager->notifyMousePosition(e.mouse);
	}

	g_system->lockMutex(_eventQueueLock);
	_eventQueue.push(e);
	g_system->unlockMutex(_eventQueueLock);
}

void QtEventHandler::handleResizeEvent(QResizeEvent *ev) {
	if (_graphicsManager) {
		QSize newSize = ev->size();
		_eventSource->notifyResize(newSize.width(), newSize.height());
	}
}

void QtEventHandler::handleWheelEvent(QWheelEvent *ev) {
	Common::Event e;
	int32 yDir = ev->angleDelta().y();

	e.mouse = Common::Point(ev->x(), ev->y());

	if (_graphicsManager) {
		_graphicsManager->notifyMousePosition(e.mouse);
	}

	if (yDir < 0) {
		e.type = Common::EVENT_WHEELDOWN;
		g_system->lockMutex(_eventQueueLock);
		_eventQueue.push(e);
		g_system->unlockMutex(_eventQueueLock);
	} else if (yDir > 0) {
		e.type = Common::EVENT_WHEELUP;
		g_system->lockMutex(_eventQueueLock);
		_eventQueue.push(e);
		g_system->unlockMutex(_eventQueueLock);
	}
}

void QtEventHandler::QtModToOSystemKeyFlags(Qt::KeyboardModifiers mod, Common::Event &ev) {
	ev.kbd.flags = 0;

	if (mod & Qt::ShiftModifier)
		ev.kbd.flags |= Common::KBD_SHIFT;
	if (mod & Qt::AltModifier)
		ev.kbd.flags |= Common::KBD_ALT;
	if (mod & Qt::ControlModifier)
		ev.kbd.flags |= Common::KBD_CTRL;
	if (mod & Qt::MetaModifier)
		ev.kbd.flags |= Common::KBD_META;
}

Common::KeyCode QtEventHandler::QtToOSystemKeycode(int key) {
	switch (key) {
	case Qt::Key_Backspace: return Common::KEYCODE_BACKSPACE;
	case Qt::Key_Tab: return Common::KEYCODE_TAB;
	case Qt::Key_Clear: return Common::KEYCODE_CLEAR;
	case Qt::Key_Return: return Common::KEYCODE_RETURN;
	case Qt::Key_Pause: return Common::KEYCODE_PAUSE;
	case Qt::Key_Escape: return Common::KEYCODE_ESCAPE;
	case Qt::Key_Space: return Common::KEYCODE_SPACE;
	case Qt::Key_Exclam: return Common::KEYCODE_EXCLAIM;
	case Qt::Key_QuoteDbl: return Common::KEYCODE_QUOTEDBL;
	case Qt::Key_NumberSign: return Common::KEYCODE_HASH;
	case Qt::Key_Dollar: return Common::KEYCODE_DOLLAR;
	case Qt::Key_Ampersand: return Common::KEYCODE_AMPERSAND;
	case Qt::Key_Apostrophe: return Common::KEYCODE_QUOTE;
	case Qt::Key_ParenLeft: return Common::KEYCODE_LEFTPAREN;
	case Qt::Key_ParenRight: return Common::KEYCODE_RIGHTPAREN;
	case Qt::Key_Asterisk: return Common::KEYCODE_ASTERISK;
	case Qt::Key_Plus: return Common::KEYCODE_PLUS;
	case Qt::Key_Comma: return Common::KEYCODE_COMMA;
	case Qt::Key_Minus: return Common::KEYCODE_MINUS;
	case Qt::Key_Period: return Common::KEYCODE_PERIOD;
	case Qt::Key_Slash: return Common::KEYCODE_SLASH;
	case Qt::Key_0: return Common::KEYCODE_0;
	case Qt::Key_1: return Common::KEYCODE_1;
	case Qt::Key_2: return Common::KEYCODE_2;
	case Qt::Key_3: return Common::KEYCODE_3;
	case Qt::Key_4: return Common::KEYCODE_4;
	case Qt::Key_5: return Common::KEYCODE_5;
	case Qt::Key_6: return Common::KEYCODE_6;
	case Qt::Key_7: return Common::KEYCODE_7;
	case Qt::Key_8: return Common::KEYCODE_8;
	case Qt::Key_9: return Common::KEYCODE_9;
	case Qt::Key_Colon: return Common::KEYCODE_COLON;
	case Qt::Key_Semicolon: return Common::KEYCODE_SEMICOLON;
	case Qt::Key_Less: return Common::KEYCODE_LESS;
	case Qt::Key_Equal: return Common::KEYCODE_EQUALS;
	case Qt::Key_Greater: return Common::KEYCODE_GREATER;
	case Qt::Key_Question: return Common::KEYCODE_QUESTION;
	case Qt::Key_At: return Common::KEYCODE_AT;
	case Qt::Key_BracketLeft: return Common::KEYCODE_LEFTBRACKET;
	case Qt::Key_Backslash: return Common::KEYCODE_BACKSLASH;
	case Qt::Key_BracketRight: return Common::KEYCODE_RIGHTBRACKET;
	case Qt::Key_AsciiCircum: return Common::KEYCODE_CARET;
	case Qt::Key_Underscore: return Common::KEYCODE_UNDERSCORE;
	case Qt::Key_QuoteLeft: return Common::KEYCODE_BACKQUOTE;
	case Qt::Key_A: return Common::KEYCODE_a;
	case Qt::Key_B: return Common::KEYCODE_b;
	case Qt::Key_C: return Common::KEYCODE_c;
	case Qt::Key_D: return Common::KEYCODE_d;
	case Qt::Key_E: return Common::KEYCODE_e;
	case Qt::Key_F: return Common::KEYCODE_f;
	case Qt::Key_G: return Common::KEYCODE_g;
	case Qt::Key_H: return Common::KEYCODE_h;
	case Qt::Key_I: return Common::KEYCODE_i;
	case Qt::Key_J: return Common::KEYCODE_j;
	case Qt::Key_K: return Common::KEYCODE_k;
	case Qt::Key_L: return Common::KEYCODE_l;
	case Qt::Key_M: return Common::KEYCODE_m;
	case Qt::Key_N: return Common::KEYCODE_n;
	case Qt::Key_O: return Common::KEYCODE_o;
	case Qt::Key_P: return Common::KEYCODE_p;
	case Qt::Key_Q: return Common::KEYCODE_q;
	case Qt::Key_R: return Common::KEYCODE_r;
	case Qt::Key_S: return Common::KEYCODE_s;
	case Qt::Key_T: return Common::KEYCODE_t;
	case Qt::Key_U: return Common::KEYCODE_u;
	case Qt::Key_V: return Common::KEYCODE_v;
	case Qt::Key_W: return Common::KEYCODE_w;
	case Qt::Key_X: return Common::KEYCODE_x;
	case Qt::Key_Y: return Common::KEYCODE_y;
	case Qt::Key_Z: return Common::KEYCODE_z;
	case Qt::Key_Delete: return Common::KEYCODE_DELETE;
	case Qt::Key_AsciiTilde: return Common::KEYCODE_TILDE;
	case Qt::Key_Up: return Common::KEYCODE_UP;
	case Qt::Key_Down: return Common::KEYCODE_DOWN;
	case Qt::Key_Right: return Common::KEYCODE_RIGHT;
	case Qt::Key_Left: return Common::KEYCODE_LEFT;
	case Qt::Key_Insert: return Common::KEYCODE_INSERT;
	case Qt::Key_Home: return Common::KEYCODE_HOME;
	case Qt::Key_End: return Common::KEYCODE_END;
	case Qt::Key_PageUp: return Common::KEYCODE_PAGEUP;
	case Qt::Key_PageDown: return Common::KEYCODE_PAGEDOWN;
	case Qt::Key_F1: return Common::KEYCODE_F1;
	case Qt::Key_F2: return Common::KEYCODE_F2;
	case Qt::Key_F3: return Common::KEYCODE_F3;
	case Qt::Key_F4: return Common::KEYCODE_F4;
	case Qt::Key_F5: return Common::KEYCODE_F5;
	case Qt::Key_F6: return Common::KEYCODE_F6;
	case Qt::Key_F7: return Common::KEYCODE_F7;
	case Qt::Key_F8: return Common::KEYCODE_F8;
	case Qt::Key_F9: return Common::KEYCODE_F9;
	case Qt::Key_F10: return Common::KEYCODE_F10;
	case Qt::Key_F11: return Common::KEYCODE_F11;
	case Qt::Key_F12: return Common::KEYCODE_F12;
	case Qt::Key_F13: return Common::KEYCODE_F13;
	case Qt::Key_F14: return Common::KEYCODE_F14;
	case Qt::Key_F15: return Common::KEYCODE_F15;
	case Qt::Key_NumLock: return Common::KEYCODE_NUMLOCK;
	case Qt::Key_CapsLock: return Common::KEYCODE_CAPSLOCK;
	case Qt::Key_ScrollLock: return Common::KEYCODE_SCROLLOCK;
	case Qt::Key_Shift: return Common::KEYCODE_RSHIFT;
	case Qt::Key_Control: return Common::KEYCODE_RCTRL;
	case Qt::Key_Alt: return Common::KEYCODE_RALT;
	case Qt::Key_Super_L: return Common::KEYCODE_LSUPER;
	case Qt::Key_Super_R: return Common::KEYCODE_RSUPER;
	case Qt::Key_Mode_switch: return Common::KEYCODE_MODE;
	case Qt::Key_Help: return Common::KEYCODE_HELP;
	case Qt::Key_Print: return Common::KEYCODE_PRINT;
	case Qt::Key_SysReq: return Common::KEYCODE_SYSREQ;
	case Qt::Key_Menu: return Common::KEYCODE_MENU;
	case Qt::Key_PowerDown: return Common::KEYCODE_POWER;
	case Qt::Key_PowerOff: return Common::KEYCODE_POWER;
	case Qt::Key_Undo: return Common::KEYCODE_UNDO;
	default: return Common::KEYCODE_INVALID;
	}
}

#endif
