// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDFEEDSIMPORTEXPORTMODEL_H
#define STANDARDFEEDSIMPORTEXPORTMODEL_H

#include "services/abstract/accountcheckmodel.h"

class FeedsImportExportModel : public AccountCheckSortedModel {
  Q_OBJECT

  public:
    enum class Mode {
      Import,
      Export
    };

    explicit FeedsImportExportModel(QObject* parent = nullptr);
    virtual ~FeedsImportExportModel();

    // Exports to OPML 2.0
    // NOTE: http://dev.opml.org/spec2.html
    bool exportToOMPL20(QByteArray& result);
    void importAsOPML20(const QByteArray& data, bool fetch_metadata_online);

    // Exports to plain text format
    // where there is one feed URL per line.
    bool exportToTxtURLPerLine(QByteArray& result);
    void importAsTxtURLPerLine(const QByteArray& data, bool fetch_metadata_online);

    Mode mode() const;
    void setMode(const Mode& mode);

  signals:

    // These signals are emitted when user selects some data
    // to be imported/parsed into the model.
    void parsingStarted();
    void parsingProgress(int completed, int total);
    void parsingFinished(int count_failed, int count_succeeded, bool parsing_error);

  private:
    Mode m_mode;
};

#endif // STANDARDFEEDSIMPORTEXPORTMODEL_H
