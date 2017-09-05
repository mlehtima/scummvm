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

#ifndef BACKENDS_MIXER_IMPL_QT_H
#define BACKENDS_MIXER_IMPL_QT_H

#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "backends/platform/qt/qt-sys.h"

#include "audio/mixer_intern.h"

/**
 * Qt mixer. It wraps the actual implementation
 * of the Audio:Mixer used by the engine, and setups
 * the Qt audio subsystem and the callback for the
 * audio mixer implementation.
 */
class QtMixer : public QObject {
	Q_OBJECT
public:
	QtMixer();
	virtual ~QtMixer();

	/**
	 * Initialize and setups the mixer
	 */
	void init();

	/**
	 * Get the audio mixer implementation
	 */
	Audio::Mixer *getMixer() { return (Audio::Mixer *)_mixer; }

	/**
	 * Pauses the audio system
	 */
	void suspendAudio();

	/**
	 * Resumes the audio system
	 */
	int resumeAudio();

protected:

	/** State of the audio system */
	bool _audioSuspended;

	/**
	 * Starts SDL audio
	 */
	virtual void startAudio();

public slots:
	void callbackHandler();

private:
	Audio::MixerImpl *_mixer;
	uint32 _outputRate;
	QAudioOutput *_output;
	QTimer *_pushTimer;
	QIODevice *_ioDevice;
};

#endif
