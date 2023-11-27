// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LIBMPVBACKEND_H
#define LIBMPVBACKEND_H

#include "gui/mediaplayer/playerbackend.h"

class MpvWidget;

class LibMpvBackend : public PlayerBackend {
    Q_OBJECT

  public:
    explicit LibMpvBackend(QWidget* parent = nullptr);

    virtual QUrl url() const;
    virtual int position() const;
    virtual int duration() const;

  public slots:
    virtual void playUrl(const QUrl& url);
    virtual void playPause();
    virtual void pause();
    virtual void stop();
    virtual void setPlaybackSpeed(int speed);
    virtual void setVolume(int volume);
    virtual void setPosition(int position);

  private:
    MpvWidget* m_video;
};

#endif // LIBMPVBACKEND_H
