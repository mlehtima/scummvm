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

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ARRAYSIZE // winnt.h defines ARRAYSIZE, but we want our own one...
#endif

#include "backends/platform/qt/qt.h"
#include "common/config-manager.h"
#include "gui/EventRecorder.h"
#include "common/taskbar.h"
#include "common/textconsole.h"

#include "backends/audiocd/default/default-audiocd.h"
#include "backends/events/default/default-events.h"
#include "backends/fs/qt/qt-fs-factory.h"
#include "backends/fs/qt/qt-fs.h"
#include "backends/saves/default/default-saves.h"
#if defined(POSIX)
#include "backends/saves/posix/posix-saves.h"
#elif defined(WIN32) && !defined(_WIN32_WCE)
#include "backends/saves/windows/windows-saves.h"
#endif

#ifdef USE_LINUXCD
#include "backends/audiocd/linux/linux-audiocd.h"
#endif

#include "backends/graphics/qt/qt-graphics.h"
#include "backends/mutex/qt/qt-mutex.h"
#include "backends/timer/qt/qt-timer.h"
#include "graphics/cursorman.h"

#include <time.h>	// for getTimeAndDate()

#ifdef USE_DETECTLANG
#ifndef WIN32
#include <locale.h>
#endif // !WIN32
#endif

OSystem_Qt::OSystem_Qt()
    : _desktopWidth(0),
      _desktopHeight(0),
      _inited(false),
      _logger(0),
      _mixerManager(0),
      _window(0) {
	_startTime.start();
}

OSystem_Qt::~OSystem_Qt() {
	QGuiApplication::restoreOverrideCursor();

	// Delete the various managers here. Note that the ModularBackend
	// destructor would also take care of this for us. However, various
	// of our managers must be deleted *before* we terminate Qt.
	// Hence, we perform the destruction on our own.
	delete _savefileManager;
	_savefileManager = 0;
	if (_graphicsManager) {
		dynamic_cast<OpenGLQtGraphicsManager *>(_graphicsManager)->deactivateManager();
	}
	delete _graphicsManager;
	_graphicsManager = 0;
	delete _window;
	_window = 0;
	delete _eventManager;
	_eventManager = 0;
	delete _eventSource;
	_eventSource = 0;
	delete _audiocdManager;
	_audiocdManager = 0;
	delete _mixerManager;
	_mixerManager = 0;

#ifdef ENABLE_EVENTRECORDER
	// HACK HACK HACK
	// This is nasty.
	delete g_eventRec.getTimerManager();
#else
	delete _timerManager;
#endif

	_timerManager = 0;
	delete _mutexManager;
	_mutexManager = 0;

	delete _logger;
	_logger = 0;
}

void OSystem_Qt::init() {
	// Disable OS cursor
	QCursor cursor(Qt::BlankCursor);
	QGuiApplication::setOverrideCursor(cursor);
	QGuiApplication::changeOverrideCursor(cursor);

	// Initialze File System Factory
	_fsFactory = new QtFilesystemFactory();

	if (!_logger)
		_logger = new Backends::Log::Log(this);

	if (_logger) {
		Common::WriteStream *logFile = createLogFile();
		if (logFile)
			_logger->open(logFile);
	}

	// Creates the early needed managers, if they don't exist yet
	// (we check for this to allow subclasses to provide their own).
	if (_mutexManager == 0)
		_mutexManager = new QtMutexManager();

	if (_window == 0)
		_window = new QtWindow();

#if defined(USE_TASKBAR)
	if (_taskbarManager == 0)
		_taskbarManager = new Common::TaskbarManager();
#endif

}

bool OSystem_Qt::hasFeature(Feature f) {
	if (f == kFeatureClipboardSupport) return true;
	if (f == kFeatureKbdMouseSpeed) return true;
	if (f == kFeatureOpenUrl)
		return true;

	return ModularBackend::hasFeature(f);
}

void OSystem_Qt::initBackend() {
	// Check if backend has not been initialized
	assert(!_inited);

	// Create the savefile manager
#if defined(POSIX)
	if (_savefileManager == 0)
		_savefileManager = new POSIXSaveFileManager();
#elif defined(WIN32) && !defined(_WIN32_WCE)
	if (_savefileManager == 0)
		_savefileManager = new WindowsSaveFileManager();
#endif

	QScreen *screen = QGuiApplication::primaryScreen();
	QRect  screenGeometry = screen->geometry();
	_desktopHeight = screenGeometry.height();
	_desktopWidth = screenGeometry.width();

	// Create the default event source, in case a custom backend
	// manager didn't provide one yet.
	if (_eventSource == 0)
		_eventSource = new QtEventSource();

	if (_eventManager == nullptr) {
		DefaultEventManager *eventManager = new DefaultEventManager(_eventSource);
		_eventManager = eventManager;
	}

	if (_graphicsManager == 0) {
		_graphicsManager = new OpenGLQtGraphicsManager(_desktopWidth, _desktopHeight, _eventSource, _window);
	}

	if (_savefileManager == 0)
		_savefileManager = new DefaultSaveFileManager();

	if (_mixerManager == 0) {
		_mixerManager = new QtMixerManager();
		// Setup and start mixer
		_mixerManager->init();
	}

#ifdef ENABLE_EVENTRECORDER
	g_eventRec.registerMixerManager(_mixerManager);

	g_eventRec.registerTimerManager(new QtTimerManager());
#else
	if (_timerManager == 0)
		_timerManager = new QtTimerManager();
#endif

	_audiocdManager = createAudioCDManager();

	_inited = true;

	if (!ConfMan.hasKey("kbdmouse_speed")) {
		ConfMan.registerDefault("kbdmouse_speed", 3);
		ConfMan.setInt("kbdmouse_speed", 3);
	}

	ModularBackend::initBackend();

	// We have to initialize the graphics manager before the event manager
	// so the virtual keyboard can be initialized, but we have to add the
	// graphics manager as an event observer after initializing the event
	// manager.
	dynamic_cast<OpenGLQtGraphicsManager *>(_graphicsManager)->activateManager();
}

#if defined(USE_TASKBAR)
void OSystem_Qt::engineInit() {
	// Add the started engine to the list of recent tasks
	_taskbarManager->addRecent(ConfMan.getActiveDomainName(), ConfMan.get("description"));

	// Set the overlay icon the current running engine
	_taskbarManager->setOverlayIcon(ConfMan.getActiveDomainName(), ConfMan.get("description"));
}

void OSystem_Qt::engineDone() {
	// Remove overlay icon
	_taskbarManager->setOverlayIcon("", "");
}
#endif

void OSystem_Qt::setWindowCaption(const char *caption) {
	// The string caption is supposed to be in LATIN-1 encoding.
	// Qt expects UTF-8. So we perform the conversion here.
	_window->setWindowCaption(QString::fromLatin1(caption).toStdString().c_str());
}

void OSystem_Qt::quit() {
	delete this;
}

void OSystem_Qt::fatalError() {
	delete this;
	exit(1);
}

void OSystem_Qt::logMessage(LogMessageType::Type type, const char *message) {
	// First log to stdout/stderr
	FILE *output = 0;

	if (type == LogMessageType::kInfo || type == LogMessageType::kDebug)
		output = stdout;
	else
		output = stderr;

	fputs(message, output);
	fflush(output);

	// Then log into file (via the logger)
	if (_logger)
		_logger->print(message);

	// Finally, some Windows / WinCE specific logging code.
#if defined( USE_WINDBG )
#if defined( _WIN32_WCE )
	TCHAR buf_unicode[1024];
	MultiByteToWideChar(CP_ACP, 0, message, strlen(message) + 1, buf_unicode, sizeof(buf_unicode));
	OutputDebugString(buf_unicode);

	if (type == LogMessageType::kError) {
#ifndef DEBUG
		drawError(message);
#else
		int cmon_break_into_the_debugger_if_you_please = *(int *)(message + 1);	// bus error
		printf("%d", cmon_break_into_the_debugger_if_you_please);			// don't optimize the int out
#endif
	}

#else
	OutputDebugString(message);
#endif
#endif
}

bool OSystem_Qt::openUrl(const Common::String &url) {
	return QDesktopServices::openUrl(QUrl(url.c_str(), QUrl::TolerantMode));
}

Common::String OSystem_Qt::getSystemLanguage() const {
#if defined(USE_DETECTLANG) && !defined(_WIN32_WCE)
#ifdef WIN32
	// We can not use "setlocale" (at least not for MSVC builds), since it
	// will return locales like: "English_USA.1252", thus we need a special
	// way to determine the locale string for Win32.
	char langName[9];
	char ctryName[9];

	const LCID languageIdentifier = GetUserDefaultUILanguage();

	if (GetLocaleInfo(languageIdentifier, LOCALE_SISO639LANGNAME, langName, sizeof(langName)) != 0 &&
		GetLocaleInfo(languageIdentifier, LOCALE_SISO3166CTRYNAME, ctryName, sizeof(ctryName)) != 0) {
		Common::String localeName = langName;
		localeName += "_";
		localeName += ctryName;

		return localeName;
	} else {
		return ModularBackend::getSystemLanguage();
	}
#else // WIN32
	// Activating current locale settings
	const Common::String locale = setlocale(LC_ALL, "");

	// Restore default C locale to prevent issues with
	// portability of sscanf(), atof(), etc.
	// See bug #3615148
	setlocale(LC_ALL, "C");

	// Detect the language from the locale
	if (locale.empty()) {
		return ModularBackend::getSystemLanguage();
	} else {
		int length = 0;

		// Strip out additional information, like
		// ".UTF-8" or the like. We do this, since
		// our translation languages are usually
		// specified without any charset information.
		for (int size = locale.size(); length < size; ++length) {
			// TODO: Check whether "@" should really be checked
			// here.
			if (locale[length] == '.' || locale[length] == ' ' || locale[length] == '@')
				break;
		}

		return Common::String(locale.c_str(), length);
	}
#endif // WIN32
#else // USE_DETECTLANG
	return ModularBackend::getSystemLanguage();
#endif // USE_DETECTLANG
}

uint32 OSystem_Qt::getMillis(bool skipRecord) {
	uint32 millis = _startTime.elapsed();

#ifdef ENABLE_EVENTRECORDER
	g_eventRec.processMillis(millis, skipRecord);
#endif

	return millis;
}

void OSystem_Qt::delayMillis(uint msecs) {
#ifdef ENABLE_EVENTRECORDER
	if (!g_eventRec.processDelayMillis())
#endif
		QThread::msleep(msecs);
}

void OSystem_Qt::getTimeAndDate(TimeDate &td) const {
	time_t curTime = time(0);
	struct tm t = *localtime(&curTime);
	td.tm_sec = t.tm_sec;
	td.tm_min = t.tm_min;
	td.tm_hour = t.tm_hour;
	td.tm_mday = t.tm_mday;
	td.tm_mon = t.tm_mon;
	td.tm_year = t.tm_year;
	td.tm_wday = t.tm_wday;
}

Audio::Mixer *OSystem_Qt::getMixer() {
	assert(_mixerManager);
	return getMixerManager()->getMixer();
}

QtMixerManager *OSystem_Qt::getMixerManager() {
	assert(_mixerManager);

#ifdef ENABLE_EVENTRECORDER
	return g_eventRec.getMixerManager();
#else
	return _mixerManager;
#endif
}

Common::TimerManager *OSystem_Qt::getTimerManager() {
#ifdef ENABLE_EVENTRECORDER
	return g_eventRec.getTimerManager();
#else
	return _timerManager;
#endif
}

AudioCDManager *OSystem_Qt::createAudioCDManager() {
#ifdef USE_LINUXCD
	return createLinuxAudioCDManager();
#else
	return new DefaultAudioCDManager();
#endif
}

Common::SaveFileManager *OSystem_Qt::getSavefileManager() {
#ifdef ENABLE_EVENTRECORDER
	return g_eventRec.getSaveManager(_savefileManager);
#else
	return _savefileManager;
#endif
}

//Not specified in base class
Common::String OSystem_Qt::getScreenshotsPath() {
	Common::String path = ConfMan.get("screenshotpath");
	if (!path.empty() && !path.hasSuffix("/"))
		path += "/";
	return path;
}

const OSystem::GraphicsMode *OSystem_Qt::getSupportedGraphicsModes() const {
	return _graphicsManager->getSupportedGraphicsModes();
}

int OSystem_Qt::getDefaultGraphicsMode() const {
	return _graphicsManager->getDefaultGraphicsMode();
}

bool OSystem_Qt::setGraphicsMode(int mode) {
	return _graphicsManager->setGraphicsMode(mode);
}

int OSystem_Qt::getGraphicsMode() const {
	return _graphicsManager->getGraphicsMode();
}

Common::String OSystem_Qt::getDefaultConfigFileName() {
	Common::String configFile;

	Common::String prefix;

	prefix = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).toStdString().c_str();

	if (!prefix.empty() && (prefix.size() + 1 + 11) < MAXPATHLEN) {
		configFile = prefix;
		configFile += "/scummvm.ini";
	} else {
		configFile = "scummvm.ini";
	}

	return configFile;
}

Common::WriteStream *OSystem_Qt::createLogFile() {
	// Start out by resetting _logFilePath, so that in case
	// of a failure, we know that no log file is open.
	_logFilePath.clear();

	const char *prefix = nullptr;
	Common::String path;
	Common::String logFile;

#ifdef MACOSX
	prefix = getenv("HOME");
	if (prefix == nullptr) {
		return 0;
	} else {
		path = prefix;
	}

	logFile = "Library/Logs";
#else
#ifdef POSIX
	// On POSIX systems we follow the XDG Base Directory Specification for
	// where to store files. The version we based our code upon can be found
	// over here: http://standards.freedesktop.org/basedir-spec/basedir-spec-0.8.html
	prefix = getenv("XDG_CACHE_HOME");
#endif
	if (prefix == nullptr || !*prefix) {
		path = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation).toStdString().c_str();
	} else {
		path = prefix;
	}

	logFile = "scummvm/logs";
#endif

	path += '/';
	path += logFile;
	QtFilesystemNode logPath(path);
	if (!logPath.create(true)) {
		warning("failed to create log %s", path.c_str());
		return 0;
	}
	logFile = path;
	logFile += "/scummvm.log";

	Common::FSNode file(logFile);
	Common::WriteStream *stream = file.createWriteStream();
	if (stream)
		_logFilePath = logFile;
	return stream;
}

void OSystem_Qt::addSysArchivesToSearchSet(Common::SearchSet &s, int priority) {
#ifdef POSIX
#ifdef DATA_PATH
	const char *snap = getenv("SNAP");
	if (snap) {
		Common::String dataPath = Common::String(snap) + DATA_PATH;
		Common::FSNode dataNode(dataPath);
		if (dataNode.exists() && dataNode.isDirectory()) {
			// This is the same priority which is used for the data path (below),
			// but we insert this one first, so it will be searched first.
			s.add(dataPath, new Common::FSDirectory(dataNode, 4), priority);
		}
	}
#endif
#endif

#ifdef DATA_PATH
	// Add the global DATA_PATH to the directory search list
	// FIXME: We use depth = 4 for now, to match the old code. May want to change that
	Common::FSNode dataNode(DATA_PATH);
	if (dataNode.exists() && dataNode.isDirectory()) {
		s.add(DATA_PATH, new Common::FSDirectory(dataNode, 4), priority);
	}
#endif

}
