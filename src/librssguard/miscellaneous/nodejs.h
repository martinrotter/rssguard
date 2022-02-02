// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NODEJS_H
#define NODEJS_H

#include <QObject>

class Settings;

class NodeJs : public QObject {
  Q_OBJECT

  public:
    explicit NodeJs(Settings* settings, QObject* parent = nullptr);

    QString nodeJsExecutable() const;
    void setNodeJsExecutable(const QString& exe) const;

    QString npmExecutable() const;
    void setNpmExecutable(const QString& exe) const;

    QString packageFolder() const;
    QString processedPackageFolder() const;
    void setPackageFolder(const QString& path);

    QString nodejsVersion(const QString& nodejs_exe) const;
    QString npmVersion(const QString& npm_exe) const;

  private:
    Settings* m_settings;
};

#endif // NODEJS_H
