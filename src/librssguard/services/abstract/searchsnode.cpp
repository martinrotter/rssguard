// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/searchsnode.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/gui/formaddeditprobe.h"
#include "services/abstract/serviceroot.h"

#include "3rd-party/boolinq/boolinq.h"

SearchsNode::SearchsNode(RootItem* parent_item) : RootItem(parent_item), m_actProbeNew(nullptr) {
  setKind(RootItem::Kind::Probes);
  setId(ID_PROBES);
  setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  setTitle(tr("Regex queries"));
  setDescription(tr("You can see all your permanent regex queries here."));
}

void SearchsNode::loadProbes(const QList<Search*>& probes) {
  for (auto* prb : probes) {
    appendChild(prb);
  }
}

QList<Message> SearchsNode::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return {};
  // return DatabaseQueries::getUndeletedLabelledMessages(database, getParentServiceRoot()->accountId());
}

Search* SearchsNode::probeById(const QString& custom_id) {
  auto chi = childItems();

  return qobject_cast<Search*>(boolinq::from(chi).firstOrDefault([custom_id](RootItem* it) {
    return it->customId() == custom_id;
  }));
}

QList<Search*> SearchsNode::probes() const {
  auto list = boolinq::from(childItems())
                .select([](RootItem* it) {
                  return static_cast<Search*>(it);
                })
                .toStdList();

  return FROM_STD_LIST(QList<Search*>, list);
}

QList<QAction*> SearchsNode::contextMenuFeedsList() {
  if (m_actProbeNew == nullptr) {
    m_actProbeNew = new QAction(qApp->icons()->fromTheme(QSL("system-search")), tr("New regex query"), this);

    connect(m_actProbeNew, &QAction::triggered, this, &SearchsNode::createProbe);
  }

  return QList<QAction*>{m_actProbeNew};
}

void SearchsNode::updateCounts(bool including_total_count) {
  Q_UNUSED(including_total_count)

  // NOTE: We do not update all counts here because it is simply taking too much time.
  // This is true when user has many regex queries added because SQLite (MariaDB) simply
  // takes too long to finish SQL queries with REGEXPs.
  //
  // We only update one by one.
  if (childCount() <= 10) {
    RootItem::updateCounts(including_total_count);
  }
  else {
  }
}

void SearchsNode::createProbe() {
  FormAddEditProbe frm(qApp->mainFormWidget());
  Search* new_prb = frm.execForAdd();

  if (new_prb != nullptr) {
    QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

    try {
      DatabaseQueries::createProbe(db, new_prb, getParentServiceRoot()->accountId());

      getParentServiceRoot()->requestItemReassignment(new_prb, this);
      getParentServiceRoot()->requestItemExpand({this}, true);

      new_prb->updateCounts(true);
    }
    catch (const ApplicationException&) {
      new_prb->deleteLater();
    }
  }
}
