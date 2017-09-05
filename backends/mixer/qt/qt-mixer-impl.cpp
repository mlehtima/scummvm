
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

#include "backends/mixer/qt/qt-mixer-impl.h"

#define SAMPLES_PER_SEC 44100

QtMixer::QtMixer()
	:
	_pushTimer(new QTimer()),
	_mixer(0),
	_outputRate(SAMPLES_PER_SEC),
	_audioSuspended(false) {
}

QtMixer::~QtMixer() {
	_mixer->setReady(false);
	_output->stop();
	delete _mixer;
}

void QtMixer::init() {
	QAudioFormat format;
	format.setSampleRate(SAMPLES_PER_SEC);
	format.setChannelCount(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo deviceInfo(QAudioDeviceInfo::defaultOutputDevice());
    if (!deviceInfo.isFormatSupported(format)) {
        warning("Default format not supported - trying to use nearest\n");
        format = deviceInfo.nearestFormat(format);
    }
	_output = new QAudioOutput(deviceInfo, format);
	_output->setNotifyInterval(20);

	_mixer = new Audio::MixerImpl(g_system, _outputRate);
	assert(_mixer);
	_mixer->setReady(true);

	startAudio();
}

void QtMixer::startAudio() {
	_pushTimer->disconnect();
	_ioDevice = _output->start();
	connect(_pushTimer, SIGNAL(timeout()), this, SLOT(callbackHandler()), Qt::DirectConnection);
	_pushTimer->start(20);
	_pushTimer->moveToThread(QGuiApplication::instance()->thread());

}

void QtMixer::callbackHandler() {
	assert(_mixer);
	if (_output->state() == QAudio::StoppedState) {
		return;
	}

	byte samples[132768];
	memset(&samples, 0, 132768*sizeof(byte));

	int len = _mixer->mixCallback(samples, _output->periodSize());
	if (len) {
		_ioDevice->write((char*)(samples), len*4);
	}
}

void QtMixer::suspendAudio() {
	printf("QtMixer::suspendAudio\n");
	_output->suspend();
	_audioSuspended = true;
}

int QtMixer::resumeAudio() {
	printf("QtMixer::resumeAudio\n");
	if (!_audioSuspended)
		return -2;
	_output->resume();
	_audioSuspended = false;
	return 0;
}

#endif
