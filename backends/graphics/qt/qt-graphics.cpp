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

#if defined(QT_BACKEND)

#include "backends/graphics/qt/qt-graphics.h"

#include "backends/events/qt/qt-events.h"
#include "backends/platform/qt/qt.h"
#include "backends/platform/qt/qt-sys.h"
#include "common/config-manager.h"
#include "common/mutex.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/util.h"
#include "common/frac.h"
#ifdef USE_RGB_COLOR
#include "common/list.h"
#endif
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/scaler.h"
#include "graphics/scaler/aspect.h"
#include "graphics/surface.h"
#include "gui/EventRecorder.h"
#ifdef USE_PNG
#include "common/file.h"
#include "image/png.h"
#endif

OpenGLQtGraphicsManager::OpenGLQtGraphicsManager(uint desktopWidth, uint desktopHeight, QtEventSource *source, QtWindow *window)
    : _glContext(0), _eventSource(source), _window(window),  _lastFlags(0),
      _graphicsScale(2), _stretchMode(STRETCH_FIT), _ignoreLoadVideoMode(false),
      _gotResize(false), _wantsFullScreen(false),
      _desiredFullscreenWidth(0), _desiredFullscreenHeight(0) {
	_desiredFullscreenWidth  = desktopWidth;
	_desiredFullscreenHeight = desktopHeight;
}

OpenGLQtGraphicsManager::~OpenGLQtGraphicsManager() {
	notifyContextDestroy();
	delete _glContext;
}

void OpenGLQtGraphicsManager::activateManager() {
	_eventSource->setGraphicsManager(this);
	// Register the graphics manager as a event observer
	g_system->getEventManager()->getEventDispatcher()->registerObserver(this, 10, false);
}

void OpenGLQtGraphicsManager::deactivateManager() {
	_eventSource->setGraphicsManager(0);
	// Unregister the event observer
	if (g_system->getEventManager()->getEventDispatcher()) {
		g_system->getEventManager()->getEventDispatcher()->unregisterObserver(this);
	}
}

bool OpenGLQtGraphicsManager::hasFeature(OSystem::Feature f) const {
	switch (f) {
	case OSystem::kFeatureFullscreenMode:
	case OSystem::kFeatureStretchMode:
	case OSystem::kFeatureIconifyWindow:
		return true;

	default:
		return OpenGLGraphicsManager::hasFeature(f);
	}
}

void OpenGLQtGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	switch (f) {
	case OSystem::kFeatureFullscreenMode:
		assert(getTransactionMode() != kTransactionNone);
		_wantsFullScreen = enable;
		break;

	case OSystem::kFeatureIconifyWindow:
		if (enable) {
			_window->iconifyWindow();
		}
		break;

	default:
		OpenGLGraphicsManager::setFeatureState(f, enable);
	}
}

bool OpenGLQtGraphicsManager::getFeatureState(OSystem::Feature f) const {
	switch (f) {
	case OSystem::kFeatureFullscreenMode:
		if (_window && _window->getQWindow()) {
			return _window->getQWindow()->windowState() == Qt::WindowFullScreen;
		} else {
			return _wantsFullScreen;
		}
	default:
		return OpenGLGraphicsManager::getFeatureState(f);
	}
}

namespace {
const OSystem::GraphicsMode qtGlStretchModes[] = {
	{"center", _s("Center"), STRETCH_CENTER},
	{"pixel-perfect", _s("Pixel-perfect scaling"), STRETCH_INTEGRAL},
	{"fit", _s("Fit to window"), STRETCH_FIT},
	{"stretch", _s("Stretch to window"), STRETCH_STRETCH},
	{nullptr, nullptr, 0}
};

} // End of anonymous namespace

const OSystem::GraphicsMode *OpenGLQtGraphicsManager::getSupportedStretchModes() const {
	return qtGlStretchModes;
}

int OpenGLQtGraphicsManager::getDefaultStretchMode() const {
	return STRETCH_FIT;
}

bool OpenGLQtGraphicsManager::setStretchMode(int mode) {
	assert(getTransactionMode() != kTransactionNone);

	if (mode == _stretchMode)
		return true;

	// Check this is a valid mode
	const OSystem::GraphicsMode *sm = qtGlStretchModes;
	bool found = false;
	while (sm->name) {
		if (sm->id == mode) {
			found = true;
			break;
		}
		sm++;
	}
	if (!found) {
		warning("unknown stretch mode %d", mode);
		return false;
	}

	_stretchMode = mode;
	return true;
}

int OpenGLQtGraphicsManager::getStretchMode() const {
	return _stretchMode;
}

#ifdef USE_RGB_COLOR
Common::List<Graphics::PixelFormat> OpenGLQtGraphicsManager::getSupportedFormats() const {
	Common::List<Graphics::PixelFormat> formats;

	// Our default mode is (memory layout wise) RGBA8888 which is a different
	// logical layout depending on the endianness. We chose this mode because
	// it is the only 32bit color mode we can safely assume to be present in
	// OpenGL and OpenGL ES implementations. Thus, we need to supply different
	// logical formats based on endianness.
#ifdef SCUMM_LITTLE_ENDIAN
	// ABGR8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#else
	// RGBA8888
	formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#endif
	// RGB565
	formats.push_back(Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
	// RGBA5551
	formats.push_back(Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0));
	// RGBA4444
	formats.push_back(Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0));

#if !USE_FORCED_GLES && !USE_FORCED_GLES2
#if !USE_FORCED_GL
	if (!isGLESContext()) {
#endif
#ifdef SCUMM_LITTLE_ENDIAN
		// RGBA8888
		formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#else
		// ABGR8888
		formats.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#endif
#if !USE_FORCED_GL
	}
#endif
#endif

	// RGB555, this is used by SCUMM HE 16 bit games.
	// This is not natively supported by OpenGL ES implementations, we convert
	// the pixel format internally.
	formats.push_back(Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0));

	formats.push_back(Graphics::PixelFormat::createFormatCLUT8());

	return formats;
}
#endif

void OpenGLQtGraphicsManager::updateScreen() {
	_glContext->makeCurrent(_window->getQWindow());
	OpenGLGraphicsManager::updateScreen();
}

void OpenGLQtGraphicsManager::notifyResize(const int width, const int height) {
	int currentWidth, currentHeight;
	QSize size = _window->getQWindow()->size();
	currentHeight = size.height();
	currentWidth = size.width();
	if (width != currentWidth || height != currentHeight) {
		return;
	}
	_gotResize = true;
	handleResize(width, height);
}

bool OpenGLQtGraphicsManager::notifyMousePosition(Common::Point &mouse) {
	bool valid = true;
	if (_activeArea.drawRect.contains(mouse)) {
		_cursorLastInActiveArea = true;
	} else {
		mouse.x = CLIP<int>(mouse.x, _activeArea.drawRect.left, _activeArea.drawRect.right - 1);
		mouse.y = CLIP<int>(mouse.y, _activeArea.drawRect.top, _activeArea.drawRect.bottom - 1);

/*
		if (_window->mouseIsGrabbed() ||
			// Keep the mouse inside the game area during dragging to prevent an
			// event mismatch where the mouseup event gets lost because it is
			// performed outside of the game area
			//(_cursorLastInActiveArea && SDL_GetMouseState(nullptr, nullptr) != 0))
			{
			setSystemMousePosition(mouse.x, mouse.y);
		} else {
			// Allow the in-game mouse to get a final movement event to the edge
			// of the window if the mouse was moved out of the game area
			if (_cursorLastInActiveArea) {
				_cursorLastInActiveArea = false;
			} else if (_cursorVisible) {
				// Keep sending events to the game if the cursor is invisible,
				// since otherwise if a game lets you skip a cutscene by
				// clicking and the user moved the mouse outside the active
				// area, the clicks wouldn't do anything, which would be
				// confusing
				valid = false;
			}
			if (_cursorVisible) {
				showCursor = SDL_ENABLE;
			}

		}
*/
	}

	if (valid) {
		setMousePosition(mouse.x, mouse.y);
		mouse = convertWindowToVirtual(mouse.x, mouse.y);
	}
	return valid;
}

bool OpenGLQtGraphicsManager::showMouse(const bool visible) {
	if (visible == _cursorVisible) {
		return visible;
	}

	return WindowedGraphicsManager::showMouse(visible);
}

bool OpenGLQtGraphicsManager::loadVideoMode(uint requestedWidth, uint requestedHeight, const Graphics::PixelFormat &format) {
	// In some cases we might not want to load the requested video mode. This
	// will assure that the window size is not altered.
	if (_ignoreLoadVideoMode) {
		_ignoreLoadVideoMode = false;
		return true;
	}

	// This function should never be called from notifyResize thus we know
	// that the requested size came from somewhere else.
	_gotResize = false;

	// Apply the currently saved scale setting.
	requestedWidth  *= _graphicsScale;
	requestedHeight *= _graphicsScale;

	// Set up the mode.
	return setupMode(requestedWidth, requestedHeight);
}

void OpenGLQtGraphicsManager::refreshScreen() {
	// Swap OpenGL buffers
	QWindow *win = _window->getQWindow();
	if (win->isExposed()) {
		_glContext->swapBuffers(win);
	}
}

void *OpenGLQtGraphicsManager::getProcAddress(const char *name) const {
	QByteArray bytename(name);
	return (void *)_glContext->getProcAddress(bytename);
}

void OpenGLQtGraphicsManager::handleResizeImpl(const int width, const int height) {
	OpenGLGraphicsManager::handleResizeImpl(width, height);
	_forceRedraw = true;
}

bool OpenGLQtGraphicsManager::setupMode(uint width, uint height) {
	// This is pretty confusing since RGBA8888 talks about the memory
	// layout here. This is a different logical layout depending on
	// whether we run on little endian or big endian. However, we can
	// only safely assume that RGBA8888 in memory layout is supported.
	// Thus, we chose this one.
	const Graphics::PixelFormat rgba8888 =
#ifdef SCUMM_LITTLE_ENDIAN
	                                       Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
#else
	                                       Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0);
#endif

	if (_glContext) {
		notifyContextDestroy();

		delete _glContext;
		_glContext = nullptr;
	}

	uint32 flags = 0;
	if (_wantsFullScreen) {
		width  = _desiredFullscreenWidth;
		height = _desiredFullscreenHeight;
		flags |= Qt::WindowFullScreen;
	}

	if (!createOrUpdateWindow(width, height, flags)) {
		warning("Failed to create window\n");
		return false;
	}

	_glContext = new QOpenGLContext();
	_glContext->setFormat(_window->getQWindow()->requestedFormat());
	_glContext->create();

	if (_glContext->isOpenGLES()) {
		setContextType(OpenGL::kContextGLES2);
	} else {
		setContextType(OpenGL::kContextGL);
	}

	if (!_glContext) {
		warning("Failed to create context\n");
		return false;
	}
	if (!_glContext->makeCurrent(_window->getQWindow())) {
		warning("Failed to make context current\n");
		return false;
	}

	notifyContextCreate(rgba8888, rgba8888);

	int actualWidth, actualHeight;
	QSize size = _window->getQWindow()->size();
	actualHeight = size.height();
	actualWidth = size.width();
	handleResize(actualWidth, actualHeight);

	return true;
}

bool OpenGLQtGraphicsManager::notifyEvent(const Common::Event &event) {
	switch (event.type) {
	case Common::EVENT_KEYUP:
		return isHotkey(event);

	case Common::EVENT_KEYDOWN:
		if (event.kbd.hasFlags(Common::KBD_ALT)) {
			if (   event.kbd.keycode == Common::KEYCODE_RETURN
				) {
				// Alt-Return and Alt-Enter toggle full screen mode
				beginGFXTransaction();
					setFeatureState(OSystem::kFeatureFullscreenMode, !getFeatureState(OSystem::kFeatureFullscreenMode));
				endGFXTransaction();

#ifdef USE_OSD
				if (getFeatureState(OSystem::kFeatureFullscreenMode)) {
					displayMessageOnOSD(_("Fullscreen mode"));
				} else {
					displayMessageOnOSD(_("Windowed mode"));
				}
#endif
				return true;
			}

			// Alt-s creates a screenshot
			if (event.kbd.keycode == Common::KEYCODE_s) {
				Common::String filename;

				Common::String screenshotsPath;
				OSystem_Qt *qt_g_system = dynamic_cast<OSystem_Qt*>(g_system);
				if (qt_g_system)
					screenshotsPath = qt_g_system->getScreenshotsPath();

				for (int n = 0;; n++) {
#ifdef USE_PNG
					filename = Common::String::format("scummvm%05d.png", n);
#else
					filename = Common::String::format("scummvm%05d.bmp", n);
#endif

					QFile file((screenshotsPath + filename).c_str());

					if (!file.open(QIODevice::ReadOnly))
						break;
					file.close();
				}

				if (saveScreenshot(screenshotsPath + filename)) {
					if (screenshotsPath.empty())
						debug("Saved screenshot '%s' in current directory", filename.c_str());
					else
						debug("Saved screenshot '%s' in directory '%s'", filename.c_str(), screenshotsPath.c_str());
				} else {
					if (screenshotsPath.empty())
						warning("Could not save screenshot in current directory");
					else
						warning("Could not save screenshot in directory '%s'", screenshotsPath.c_str());
				}

				return true;
			}

		} else if (event.kbd.hasFlags(Common::KBD_CTRL | Common::KBD_ALT)) {
			if (event.kbd.keycode == Common::KEYCODE_a) {
				// In case the user changed the window size manually we will
				// not change the window size again here.
				_ignoreLoadVideoMode = _gotResize;

				// Ctrl+Alt+a toggles the aspect ratio correction state.
				beginGFXTransaction();
					setFeatureState(OSystem::kFeatureAspectRatioCorrection, !getFeatureState(OSystem::kFeatureAspectRatioCorrection));
				endGFXTransaction();

				// Make sure we do not ignore the next resize. This
				// effectively checks whether loadVideoMode has been called.
				assert(!_ignoreLoadVideoMode);

#ifdef USE_OSD
				if (getFeatureState(OSystem::kFeatureAspectRatioCorrection))
					displayMessageOnOSD(_("Enabled aspect ratio correction"));
				else
					displayMessageOnOSD(_("Disabled aspect ratio correction"));
#endif

				return true;
			} else if (event.kbd.keycode == Common::KEYCODE_f) {
				// Never ever try to resize the window when we simply want to enable or disable filtering.
				// This assures that the window size does not change.
				_ignoreLoadVideoMode = true;

				// Ctrl+Alt+f toggles filtering on/off
				beginGFXTransaction();
					setFeatureState(OSystem::kFeatureFilteringMode, !getFeatureState(OSystem::kFeatureFilteringMode));
				endGFXTransaction();

				// Make sure we do not ignore the next resize. This
				// effectively checks whether loadVideoMode has been called.
				assert(!_ignoreLoadVideoMode);

#ifdef USE_OSD
				if (getFeatureState(OSystem::kFeatureFilteringMode)) {
					displayMessageOnOSD(_("Filtering enabled"));
				} else {
					displayMessageOnOSD(_("Filtering disabled"));
				}
#endif

				return true;
			} else if (event.kbd.keycode == Common::KEYCODE_s) {
				// Never try to resize the window when changing the scaling mode.
				_ignoreLoadVideoMode = true;
				// Ctrl+Alt+s cycles through stretch mode
				int index = 0;
				const OSystem::GraphicsMode *sm = qtGlStretchModes;
				while (sm->name) {
					if (sm->id == _stretchMode)
						break;
					sm++;
					index++;
				}
				index++;
				if (!qtGlStretchModes[index].name)
					index = 0;
				beginGFXTransaction();
				setStretchMode(qtGlStretchModes[index].id);
				endGFXTransaction();
#ifdef USE_OSD
				Common::String message = Common::String::format("%s: %s",
					_("Stretch mode"),
					_(qtGlStretchModes[index].description)
					);
				displayMessageOnOSD(message.c_str());
#endif
				return true;
			}
		}
		// Fall through

	default:
		return false;
	}

	return false;
}

bool OpenGLQtGraphicsManager::isHotkey(const Common::Event &event) const {
	if (event.kbd.hasFlags(Common::KBD_ALT)) {
		return    event.kbd.keycode == Common::KEYCODE_RETURN
		       || event.kbd.keycode == Common::KEYCODE_s;
	} else if (event.kbd.hasFlags(Common::KBD_CTRL | Common::KBD_ALT)) {
		return    event.kbd.keycode == Common::KEYCODE_a
		       || event.kbd.keycode == Common::KEYCODE_f
		       || event.kbd.keycode == Common::KEYCODE_s;
	}
	return false;
}

bool OpenGLQtGraphicsManager::createOrUpdateWindow(int width, int height, const uint32 flags) {
	if (!_window) {
		return false;
	}

	// We only update the actual window when flags change (which usually means
	// fullscreen mode is entered/exited), when updates are forced so that we
	// do not reset the window size whenever a game makes a call to change the
	// size or pixel format of the internal game surface (since a user may have
	// resized the game window), or when the launcher is visible (since a user
	// may change the scaler, which should reset the window size)
	if (!_window->getQWindow() || _lastFlags != flags || _overlayVisible) {
		if (!_window->createOrUpdateWindow(width, height, flags)) {
			return false;
		}
		_lastFlags = flags;
	}

	return true;
}

#endif
