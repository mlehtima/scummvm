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

#ifndef PLATFORM_QT_H
#define PLATFORM_QT_H

#include "backends/platform/qt/qt-sys.h"

#include "common/events.h"
#include "backends/log/log.h"
#include "backends/modular-backend.h"
#include "backends/events/qt/qt-events.h"
#include "backends/mixer/qt/qt-mixer.h"
#include "backends/platform/qt/qt-window.h"

#include "common/array.h"

/**
 * Base OSystem class for all Qt ports.
 */
class OSystem_Qt : public ModularBackend {
public:
	OSystem_Qt();
	virtual ~OSystem_Qt();

	/**
	 * Pre-initialize backend. It should be called after
	 * instantiating the backend. Early needed managers are
	 * created here.
	 */
	virtual void init();

	/**
	 * Get the Mixer Manager instance. Not to confuse with getMixer(),
	 * that returns Audio::Mixer. The Mixer Manager is a Qt wrapper class
	 * for the Audio::Mixer. Used by other managers.
	 */
	virtual QtMixerManager *getMixerManager();

	virtual bool hasFeature(Feature f);

	// Override functions from ModularBackend and OSystem
	virtual void initBackend();
#if defined(USE_TASKBAR)
	virtual void engineInit();
	virtual void engineDone();
#endif
	virtual void quit() override;
	virtual void fatalError() override;

	// Logging
	virtual void logMessage(LogMessageType::Type type, const char *message);

	virtual bool openUrl(const Common::String &url);

	virtual Common::String getSystemLanguage() const;

	virtual void setWindowCaption(const char *caption);
	virtual void addSysArchivesToSearchSet(Common::SearchSet &s, int priority = 0);
	virtual uint32 getMillis(bool skipRecord = false);
	virtual void delayMillis(uint msecs);
	virtual void getTimeAndDate(TimeDate &td) const;
	virtual Audio::Mixer *getMixer();
	virtual Common::TimerManager *getTimerManager();
	virtual Common::SaveFileManager *getSavefileManager();

	//Screenshots
	virtual Common::String getScreenshotsPath();

protected:
	bool _inited;

	/**
	 * The event source we use for obtaining Qt events.
	 */
	QtEventSource *_eventSource;

	/**
	 * Mixer manager that configures and setups Qt for
	 * the wrapped Audio::Mixer, the true mixer.
	 */
	QtMixerManager *_mixerManager;

	/**
	 * The Qt output window.
	 */
	QtWindow *_window;

	virtual Common::EventSource *getDefaultEventSource() { return _eventSource; }

	/**
	 * Create the audio CD manager
	 */
	virtual AudioCDManager *createAudioCDManager();

	// Logging
	virtual Common::WriteStream *createLogFile();
	Backends::Log::Log *_logger;

	int _desktopWidth, _desktopHeight;

	virtual const OSystem::GraphicsMode *getSupportedGraphicsModes() const;
	virtual int getDefaultGraphicsMode() const;
	virtual bool setGraphicsMode(int mode);

	virtual int getGraphicsMode() const;

	/**
	 * The path of the currently open log file, if any.
	 *
	 * @note This is currently a string and not an FSNode for simplicity;
	 * e.g. we don't need to include fs.h here, and currently the
	 * only use of this value is to use it to open the log file in an
	 * editor; for that, we need it only as a string anyway.
	 */
	Common::String _logFilePath;

	virtual Common::String getDefaultConfigFileName();

private:
	QTime _startTime;
};

#endif
