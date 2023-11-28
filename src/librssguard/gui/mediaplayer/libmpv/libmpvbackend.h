// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LIBMPVBACKEND_H
#define LIBMPVBACKEND_H

#include "gui/mediaplayer/playerbackend.h"

struct mpv_handle;
struct mpv_event;

class LibMpvBackend : public PlayerBackend {
    Q_OBJECT

  public:
    explicit LibMpvBackend(QWidget* parent = nullptr);
    virtual ~LibMpvBackend();

  public:
    virtual bool eventFilter(QObject* watched, QEvent* event);

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

  private slots:
    void onMpvEvents();

  signals:
    void launchMpvEvents();

  private:
    void appendLog(const QString& text);
    void createPlayer();
    void handleMpvEvent(mpv_event* event);

  private:
    QWidget* m_mpvContainer;
    mpv_handle* m_mpvHandle;
};

#endif // LIBMPVBACKEND_H
