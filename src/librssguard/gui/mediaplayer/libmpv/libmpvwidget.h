// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LIBMPVWIDGET_H
#define LIBMPVWIDGET_H

#include <QOpenGLWidget>

struct mpv_handle;
struct mpv_event;
struct mpv_render_context;

class MpvWidget : public QOpenGLWidget {
    Q_OBJECT

  public:
    explicit MpvWidget(QWidget* parent = nullptr, Qt::WindowFlags f = {});
    virtual ~MpvWidget();

    void command(const QVariant& params);
    void setProperty(const QString& name, const QVariant& value);
    QVariant getProperty(const QString& name) const;
    QSize sizeHint() const {
      return QSize(480, 270);
    }

  signals:
    void durationChanged(int value);
    void positionChanged(int value);

  protected:
    virtual void initializeGL();
    virtual void paintGL();

  private slots:
    void on_mpv_events();
    void maybeUpdate();

  private:
    void handle_mpv_event(mpv_event* event);
    static void on_update(void* ctx);

  private:
    mpv_handle* mpv;
    mpv_render_context* mpv_gl;

    // QWidget interface
  protected:
    virtual void keyPressEvent(QKeyEvent* event);

    // QObject interface
  public:
    virtual bool eventFilter(QObject* watched, QEvent* event);
};

#endif // LIBMPVWIDGET_H
