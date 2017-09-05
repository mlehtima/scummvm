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

#ifndef BACKENDS_MIXER_QT_H
#define BACKENDS_MIXER_QT_H

#include "backends/platform/qt/qt-sys.h"
#include "backends/mixer/qt/qt-mixer-impl.h"

#include "audio/mixer_intern.h"

/**
 * Qt mixer manager. Wrapper for the Qt mixer implementation.
 */
class QtMixerManager {
public:
	QtMixerManager();
	virtual ~QtMixerManager();

	/**
	 * Initialize and setups the mixer
	 */
	virtual void init();

	/**
	 * Get the audio mixer implementation
	 */
	Audio::Mixer *getMixer() { return _audioOutput->getMixer(); }

	/**
	 * Pauses the audio system
	 */
	virtual void suspendAudio();

	/**
	 * Resumes the audio system
	 */
	virtual int resumeAudio();

private:
	QtMixer *_audioOutput;
};

#endif
