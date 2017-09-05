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

#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "backends/fs/qt/qt-fs.h"
#include "backends/fs/stdiostream.h"
#include "common/algorithm.h"

#include "common/debug.h"

void QtFilesystemNode::setFlags() {
	QFileInfo info(_path.c_str());
	_isValid = (info.exists());
	_isDirectory = _isValid && info.isDir();
}

QtFilesystemNode::QtFilesystemNode(const Common::String &p) {
	assert(p.size() > 0);

	_path = QFileInfo(p.c_str()).absoluteFilePath().toStdString().c_str();

	// Normalize the path (that is, remove unneeded slashes etc.)
	_path = Common::normalizePath(_path, '/');
	_displayName = Common::lastPathComponent(_path, '/');

	setFlags();
}

AbstractFSNode *QtFilesystemNode::getChild(const Common::String &n) const {
	assert(!_path.empty());
	assert(_isDirectory);

	// Make sure the string contains no slashes
	assert(!n.contains('/'));

	// We assume here that _path is already normalized (hence don't bother to call
	//  Common::normalizePath on the final path).
	Common::String newPath(_path);
	if (_path.lastChar() != '/')
		newPath += '/';
	newPath += n;

	return makeNode(newPath);
}

bool QtFilesystemNode::getChildren(AbstractFSList &myList, ListMode mode, bool hidden) const {
	assert(_isDirectory);

	QDir dir(_path.c_str());

	QDirIterator it(dir, QDirIterator::FollowSymlinks);
	while (it.hasNext()) {
		it.next();

		QFileInfo info(it.fileInfo());

		// Skip 'invisible' files if necessary
		if (info.isHidden() && !hidden) {
			continue;
		}
		// Skip '.' and '..' to avoid cycles
		if (it.fileName() == "." || it.fileName() == "..") {
			continue;
		}

		// Start with a clone of this node, with the correct path set
		QtFilesystemNode entry(*this);
		entry._displayName = info.fileName().toStdString().c_str();
		if (_path.lastChar() != '/')
			entry._path += '/';
		entry._path += entry._displayName;

		entry._isDirectory = info.isDir();

		// Skip files that are invalid for some reason (e.g. because we couldn't
		// properly stat them).
		if (!entry._isValid)
			continue;

		// Honor the chosen mode
		if ((mode == Common::FSNode::kListFilesOnly && entry._isDirectory) ||
			(mode == Common::FSNode::kListDirectoriesOnly && !entry._isDirectory))
			continue;

		myList.push_back(new QtFilesystemNode(entry));
	}

	return true;
}

AbstractFSNode *QtFilesystemNode::getParent() const {
	if (_path == "/")
		return 0;	// The filesystem root has no parent

	const char *start = _path.c_str();
	const char *end = start + _path.size();

	// Strip of the last component. We make use of the fact that at this
	// point, _path is guaranteed to be normalized
	while (end > start && *(end-1) != '/')
		end--;

	if (end == start) {
		// This only happens if we were called with a relative path, for which
		// there simply is no parent.
		// TODO: We could also resolve this by assuming that the parent is the
		//       current working directory, and returning a node referring to that.
		return 0;
	}

	return makeNode(Common::String(start, end));
}

Common::SeekableReadStream *QtFilesystemNode::createReadStream() {
	return StdioStream::makeFromPath(getPath(), false);
}

Common::WriteStream *QtFilesystemNode::createWriteStream() {
	return StdioStream::makeFromPath(getPath(), true);
}

bool QtFilesystemNode::create(bool isDirectoryFlag) {
	bool success;

	if (isDirectoryFlag) {
		QDir dir(_path.c_str());
		if (!dir.exists())
			success = dir.mkpath(".");
		else
			success = true;
	} else {
		QFile file(_path.c_str());
		success = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
	}

	if (success) {
		setFlags();
		if (_isValid) {
			if (_isDirectory != isDirectoryFlag) warning("failed to create %s: got %s", isDirectoryFlag ? "directory" : "file", _isDirectory ? "directory" : "file");
			return _isDirectory == isDirectoryFlag;
		}

		warning("QtFilesystemNode: %s() was a success, but indicates there is no such %s",
			isDirectoryFlag ? "mkdir" : "create", isDirectoryFlag ? "directory" : "file");
		return false;
	}

	return false;
}

#endif
