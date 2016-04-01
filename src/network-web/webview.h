// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebEngineView>


class WebView : public QWebEngineView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit WebView(QWidget *parent = 0);
    virtual ~WebView();

    // Page accessor.
    inline QWebEnginePage *page() const {
      return m_page;
    }

  signals:
    // Emitted if user changes zoom factor via CTRL + mouse wheel combo.
    void zoomFactorChanged();

  public slots:
    // Page zoom modifiers.
    bool increaseWebPageZoom();
    bool decreaseWebPageZoom();
    bool resetWebPageZoom();

  protected:
    // Customize mouse wheeling.
    void wheelEvent(QWheelEvent *event);

  private:
    QWebEnginePage *m_page;
};

#endif // BASEWEBVIEW_H
