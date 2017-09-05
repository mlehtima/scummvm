
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

#include "backends/timer/qt/qt-timer-impl.h"

QtTimer::QtTimer(DefaultTimerManager *manager) : _manager(manager) {
	// Creates the timer callback
	_timer = new QTimer();
	connect(_timer, SIGNAL(timeout()), this, SLOT(timerCallback()), Qt::DirectConnection);
	_timer->start(10);
	_timer->moveToThread(QGuiApplication::instance()->thread());
}

QtTimer::~QtTimer() {
	// Removes the timer callback
	_timer->stop();
	delete _timer;
}

void QtTimer::timerCallback() {
	_manager->handler();
}

#endif
