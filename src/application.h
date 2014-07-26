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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "qtsingleapplication/qtsingleapplication.h"

#include "miscellaneous/settings.h"

#if defined(qApp)
#undef qApp
#endif

// Define new qApp macro. Yeaaaaah.
#define qApp (Application::instance())


// TODO: presunout nektery veci sem, settings atp
class Application : public QtSingleApplication {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Application(const QString &id, int &argc, char **argv);
    virtual ~Application();

    inline Settings *settings() {
      if (m_settings == NULL) {
        m_settings = Settings::setupSettings(this);
      }

      return m_settings;
    }

    // Returns pointer to "GOD" application singleton.
    inline static Application *instance() {
      return static_cast<Application*>(QCoreApplication::instance());
    }

  private:
    Settings *m_settings;
};

#endif // APPLICATION_H
