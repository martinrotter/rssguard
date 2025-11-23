// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <optional>

#include <QMetaType>
#include <QStringList>

class ExternalTool {
  public:
    explicit ExternalTool() = default;
    ExternalTool(const ExternalTool& other);
    explicit ExternalTool(QString name, QString executable, QString parameters, QString domain);

    QString executable() const;
    QString parameters() const;
    QString name() const;

    bool run(const QString& target);

    QByteArray toString();

  public:
    static std::optional<ExternalTool> toolForDomain(const QList<ExternalTool>& tools, const QString& domain);
    static ExternalTool fromString(const QByteArray& str);
    static QList<ExternalTool> toolsFromSettings();
    static void setToolsToSettings(QVector<ExternalTool>& tools);

    QString domain() const;

  private:
    QString m_name;
    QString m_executable;
    QString m_parameters;
    QString m_domain;

    void sanitizeParameters();
};

Q_DECLARE_METATYPE(ExternalTool)

#endif // EXTERNALTOOL_H
