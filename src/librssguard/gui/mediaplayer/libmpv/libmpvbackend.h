// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LIBMPVBACKEND_H
#define LIBMPVBACKEND_H

#include "gui/mediaplayer/playerbackend.h"

#include <mpv/client.h>

class LibMpvWidget;

class LibMpvBackend : public PlayerBackend {
    Q_OBJECT

  public:
    explicit LibMpvBackend(Application* app, QWidget* parent = nullptr);
    virtual ~LibMpvBackend();

  public:
    virtual bool eventFilter(QObject* watched, QEvent* event);

    virtual QUrl url() const;
    virtual int position() const;
    virtual int duration() const;

    static void installCustomConfig(const QString& directory);

  public slots:
    virtual void setMuted(bool muted);
    virtual void playUrl(const QUrl& url);
    virtual void playPause();
    virtual void pause();
    virtual void stop();
    virtual void setPlaybackSpeed(int speed);
    virtual void setVolume(int volume);
    virtual void setPosition(int position);
    virtual void setFullscreen(bool fullscreen);

  private slots:
    void onMpvEvents();

  signals:
    void launchMpvEvents();

  private:
    void processEndFile(mpv_event_end_file* end_file);
    void processTracks(const QJsonDocument& json);
    void processPropertyChange(mpv_event_property* prop, uint64_t property_code);
    void processLogMessage(mpv_event_log_message* msg);
    void appendLog(const QString& text);
    void createPlayer();
    void handleMpvEvent(mpv_event* event);

    const char* mpvDecodeString(void* data) const;
    bool mpvDecodeBool(void* data) const;
    int mpvDecodeInt(void* data) const;
    double mpvDecodeDouble(void* data) const;
    QString mpvEncodeKeyboardButton(int btn) const;
    QString errorToString(mpv_error error) const;

    void destroyHandle();
    void loadSettings();

  private:
    QString m_customConfigFolder;
    LibMpvWidget* m_mpvContainer;
    mpv_handle* m_mpvHandle;
    QUrl m_url;
};

#endif // LIBMPVBACKEND_H
