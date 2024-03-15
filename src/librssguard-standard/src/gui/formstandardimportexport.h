// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEXPORT_H
#define FORMEXPORT_H

#include <QDialog>

#include "src/standardfeedsimportexportmodel.h"
#include "ui_formstandardimportexport.h"

namespace Ui {
  class FormStandardImportExport;
}

class Category;
class StandardServiceRoot;

class FormStandardImportExport : public QDialog {
    Q_OBJECT

  public:
    enum class ConversionType { OPML20 = 0, TxtUrlPerLine = 1 };

    explicit FormStandardImportExport(StandardServiceRoot* service_root, QWidget* parent = nullptr);
    virtual ~FormStandardImportExport();

    void setMode(FeedsImportExportModel::Mode mode);

  private slots:
    void performAction();
    void selectFile();

    void onParsingStarted();
    void onParsingFinished(int count_failed, int count_succeeded);
    void onParsingProgress(int completed, int total);

    void onPostProcessScriptChanged(const QString& new_pp);

  private:
    void selectExportFile(bool without_dialog);
    void selectImportFile();
    void parseImportFile(const QString& file_name,
                         bool fetch_metadata_online,
                         bool do_not_fetch_titles,
                         bool do_not_fetch_icons,
                         const QString& post_process_script);

    void exportFeeds();
    void importFeeds();

    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

    QScopedPointer<Ui::FormStandardImportExport> m_ui;
    ConversionType m_conversionType;
    FeedsImportExportModel* m_model;
    StandardServiceRoot* m_serviceRoot;
};

#endif // FORMEXPORT_H
