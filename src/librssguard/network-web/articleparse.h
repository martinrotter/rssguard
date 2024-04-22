// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLEPARSE_H
#define ARTICLEPARSE_H

#include "miscellaneous/nodejs.h"

#include <QObject>
#include <QProcess>

class ArticleParse : public QObject {
    Q_OBJECT

  public:
    explicit ArticleParse(QObject* parent = nullptr);

    void parseArticle(QObject* sndr, const QString& url);

  private slots:
    void onParsingFinished(QObject* sndr, int exit_code, QProcess::ExitStatus exit_status);
    void onPackageReady(const QList<NodeJs::PackageMetadata>& pkgs, bool already_up_to_date);
    void onPackageError(const QList<NodeJs::PackageMetadata>& pkgs, const QString& error);

  signals:
    void articleParsed(QObject* sndr, const QString& better_html);
    void errorOnArticlePArsing(QObject* sndr, const QString& error);

  private:
    bool m_modulesInstalling;
    bool m_modulesInstalled;
    QString m_scriptFilename;
};

#endif // ARTICLEPARSE_H
