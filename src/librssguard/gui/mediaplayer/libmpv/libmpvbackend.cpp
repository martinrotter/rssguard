// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvbackend.h"

#include "gui/mediaplayer/libmpv/libmpvwidget.h"

#include <QLayout>

#include <mpv/client.h>
#include <mpv/render_gl.h>

LibMpvBackend::LibMpvBackend(QWidget* parent) : PlayerBackend(parent), m_video(new MpvWidget(this)) {
  layout()->addWidget(m_video);
}

void LibMpvBackend::playUrl(const QUrl& url) {
  m_video->command(QStringList() << "loadfile" << url.toString());
}

void LibMpvBackend::playPause() {}

void LibMpvBackend::pause() {}

void LibMpvBackend::stop() {}

void LibMpvBackend::setPlaybackSpeed(int speed) {}

void LibMpvBackend::setVolume(int volume) {}

void LibMpvBackend::setPosition(int position) {}

QUrl LibMpvBackend::url() const {
  return {};
}

int LibMpvBackend::position() const {
  return 0;
}

int LibMpvBackend::duration() const {
  return 0;
}
