// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef READABILITY_H
#define READABILITY_H

#include <QObject>

#include <QProcess>

class Readability : public QObject {
  Q_OBJECT

  public:
    explicit Readability(QObject* parent = nullptr);

    void makeHtmlReadable(const QString& html, const QString& base_url = {});

  private slots:
    void onReadabilityFinished(int exit_code, QProcess::ExitStatus exit_status);

  signals:
    void htmlReadabled(const QString& better_html);
    void errorOnHtmlReadabiliting(const QString& error);
};

#endif // READABILITY_H
