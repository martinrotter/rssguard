// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

#include <QPointer>


class Settings : public QSettings {
    Q_OBJECT

  public:
    // Describes possible types of loaded settings.
    enum Type {
      Portable,
      NonPortable
    };

    // Destructor.
    virtual ~Settings();

    // Type of used settings.
    inline Type type() const {
      return m_initializationStatus;
    }

    // Getter/setter for settings values.
    inline QVariant value(const QString &section,
                   const QString &key,
                   const QVariant &default_value = QVariant()) {
      return QSettings::value(QString("%1/%2").arg(section, key), default_value);
    }

    inline void setValue(const QString &section,
                  const QString &key,
                  const QVariant &value) {
      QSettings::setValue(QString("%1/%2").arg(section, key), value);
    }

    // Synchronizes settings.
    QSettings::Status checkSettings();

    // Creates settings file in correct location.
    static Settings* setupSettings(QObject *parent);

  private:
    // Constructor.
    Settings(const QString & file_name, Format format,
             const Type &type, QObject * parent = 0);

    Type m_initializationStatus;
};

#endif // SETTINGS_H
