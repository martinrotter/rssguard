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

#include "network-web/webview.h"

#include "definitions/definitions.h"

#include <QWebEnginePage>
#include <QWheelEvent>


WebView::WebView(QWidget *parent)
  : QWebEngineView(parent), m_page(new QWebEnginePage(this)) {
  setPage(m_page);
}

WebView::~WebView() {
  qDebug("Destroying WebView.");
}

bool WebView::increaseWebPageZoom() {
  const qreal new_factor = zoomFactor() + 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool WebView::decreaseWebPageZoom() {
  const qreal new_factor = zoomFactor() - 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool WebView::resetWebPageZoom() {
  const qreal new_factor = 1.0;

  if (new_factor != zoomFactor()) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}
