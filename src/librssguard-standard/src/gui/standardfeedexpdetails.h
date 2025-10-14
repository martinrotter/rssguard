// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDFEEDEXPDETAILS_H
#define STANDARDFEEDEXPDETAILS_H

#include "ui_standardfeedexpdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class Category;
class RootItem;

class StandardFeedExpDetails : public QWidget {
    Q_OBJECT

    friend class FormStandardFeedDetails;

  public:
    explicit StandardFeedExpDetails(QWidget* parent = nullptr);

  private:
    Ui::StandardFeedExpDetails m_ui;
};

#endif // STANDARDFEEDEXPDETAILS_H
