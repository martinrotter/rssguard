// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/labelsnode.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/gui/formaddeditlabel.h"
#include "services/abstract/serviceroot.h"

LabelsNode::LabelsNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Labels);
  setId(ID_LABELS);
  setIcon(qApp->icons()->fromTheme(QSL("tag-folder"), QSL("emblem-favorite")));
  setTitle(tr("Labels"));
  setDescription(tr("You can see all your labels (tags) here."));
}

void LabelsNode::loadLabels(const QList<Label*>& labels) {
  for (Label* lbl : labels) {
    appendChild(lbl);
  }
}

QString LabelsNode::additionalTooltip() const {
  return tr("Number of labels: %1").arg(labels().size());
}

void LabelsNode::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);
  DatabaseQueries::markAllLabelledMessagesReadUnread(qApp->database()->driver()->connection(metaObject()->className()),
                                                     service->accountId(),
                                                     status);
  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}

void LabelsNode::updateCounts() {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = account()->accountId();
  auto acc = DatabaseQueries::getMessageCountsForAllLabels(database, account_id);

  for (Label* lbl : labels()) {
    if (!acc.contains(lbl->id())) {
      lbl->setCountOfAllMessages(0);
      lbl->setCountOfUnreadMessages(0);
    }
    else {
      auto ac = acc.value(lbl->id());

      lbl->setCountOfAllMessages(ac.m_total);
      lbl->setCountOfUnreadMessages(ac.m_unread);
    }
  }
}

Label* LabelsNode::labelByCustomId(const QString& custom_id) {
  auto chi = childItems();

  return qobject_cast<Label*>(qlinq::from(chi)
                                .firstOrDefault([custom_id](RootItem* it) {
                                  return it->customId() == custom_id;
                                })
                                .value_or(nullptr));
}

QHash<QString, Label*> LabelsNode::getHashedLabels() const {
  QHash<QString, Label*> res;

  for (Label* lbl : labels()) {
    res.insert(lbl->customId(), lbl);
  }

  return res;
}

QList<Label*> LabelsNode::labels() const {
  return qlinq::from(childItems())
    .select([](RootItem* it) {
      return static_cast<Label*>(it);
    })
    .toList();
}

void LabelsNode::createLabel() {
  if (Globals::hasFlag(account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Adding)) {
    FormAddEditLabel frm(qApp->mainFormWidget());
    Label* new_lbl = frm.execForAdd();

    if (new_lbl != nullptr) {
      QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

      try {
        DatabaseQueries::createLabel(db, new_lbl, account()->accountId());

        account()->requestItemReassignment(new_lbl, this);
        account()->requestItemExpand({this}, true);
      }
      catch (const ApplicationException& ex) {
        new_lbl->deleteLater();
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             {tr("Not allowed"),
                              tr("Cannot create label: %1.").arg(ex.message()),
                              QSystemTrayIcon::MessageIcon::Critical});
      }
    }
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Not allowed"),
                          tr("This account does not allow you to create labels."),
                          QSystemTrayIcon::MessageIcon::Critical});
  }
}
