// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "miscellaneous/iofactory.h"

#include "definitions/definitions.h"
#include "exceptions/ioexception.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QObject>
#include <QTemporaryFile>
#include <QTextStream>


IOFactory::IOFactory() {
}

bool IOFactory::isFolderWritable(const QString& folder) {
	QString real_file = folder;

	if (!real_file.endsWith(QDir::separator())) {
		real_file += QDir::separator();
	}

	real_file += "test-permissions-file";
	return QTemporaryFile(real_file).open();
}

QString IOFactory::getSystemFolder(QStandardPaths::StandardLocation location) {
	return QStandardPaths::writableLocation(location);
}

QString IOFactory::ensureUniqueFilename(const QString& name, const QString& append_format) {
	if (!QFile::exists(name)) {
		return name;
	}

	QString tmp_filename = name;
	int i = 1;

	while (QFile::exists(tmp_filename)) {
		tmp_filename = name;
		const int index = tmp_filename.lastIndexOf(QL1C('.'));
		const QString append_string = append_format.arg(i++);

		if (index < 0) {
			tmp_filename.append(append_string);
		}
		else {
			tmp_filename = tmp_filename.left(index) + append_string + tmp_filename.mid(index);
		}
	}

	return tmp_filename;
}

QString IOFactory::filterBadCharsFromFilename(const QString& name) {
	QString value = name;
	value.replace(QL1C('/'), QL1C('-'));
	value.remove(QL1C('\\'));
	value.remove(QL1C(':'));
	value.remove(QL1C('*'));
	value.remove(QL1C('?'));
	value.remove(QL1C('"'));
	value.remove(QL1C('<'));
	value.remove(QL1C('>'));
	value.remove(QL1C('|'));
	return value;
}

QByteArray IOFactory::readTextFile(const QString& file_path) {
	QFile input_file(file_path);
	QByteArray input_data;

	if (input_file.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::ReadOnly)) {
		input_data = input_file.readAll();
		input_file.close();
		return input_data;
	}
	else {
		throw IOException(tr("Cannot open file '%1' for reading.").arg(QDir::toNativeSeparators(file_path)));
	}
}

void IOFactory::writeTextFile(const QString& file_path, const QByteArray& data, const QString& encoding) {
	Q_UNUSED(encoding)
	QFile input_file(file_path);
	QTextStream stream(&input_file);

	if (input_file.open(QIODevice::Text | QIODevice::WriteOnly)) {
		stream << data;
		stream.flush();
		input_file.flush();
		input_file.close();
	}
	else {
		throw IOException(tr("Cannot open file '%1' for writting.").arg(QDir::toNativeSeparators(file_path)));
	}
}

bool IOFactory::copyFile(const QString& source, const QString& destination) {
	if (QFile::exists(destination)) {
		if (!QFile::remove(destination)) {
			return false;
		}
	}

	return QFile::copy(source, destination);
}
