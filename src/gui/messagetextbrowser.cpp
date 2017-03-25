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

#include "gui/messagetextbrowser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"


MessageTextBrowser::MessageTextBrowser(QWidget *parent) : QTextBrowser(parent) {
}

MessageTextBrowser::~MessageTextBrowser() {
}

QVariant MessageTextBrowser::loadResource(int type, const QUrl &name) {
  Q_UNUSED(name)

  switch (type) {
    case QTextDocument::ImageResource: {
      if (m_imagePlaceholder.isNull()) {
        m_imagePlaceholder = qApp->icons()->miscPixmap(QSL("image-placeholder")).scaledToWidth(20, Qt::FastTransformation);
      }

      return m_imagePlaceholder;
    }

    default:
      return QTextBrowser::loadResource(type, name);
  }
}

void MessageTextBrowser::wheelEvent(QWheelEvent *e) {
  QTextBrowser::wheelEvent(e);
  qApp->settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, font().toString());
}
