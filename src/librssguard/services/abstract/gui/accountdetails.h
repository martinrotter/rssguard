// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ACCOUNTDETAILS_H
#define ACCOUNTDETAILS_H

#include <QWidget>

namespace Ui {
  class AccountDetails;
}

class RSSGUARD_DLLSPEC AccountDetails : public QWidget {
    Q_OBJECT

    friend class FormAccountDetails;

  public:
    explicit AccountDetails(QWidget* parent = nullptr);
    virtual ~AccountDetails();

  private:
    QScopedPointer<Ui::AccountDetails> m_ui;
};

#endif // ACCOUNTDETAILS_H
