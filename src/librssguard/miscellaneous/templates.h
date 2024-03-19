// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TEMPLATES_H
#define TEMPLATES_H

#include <QVariantList>

template <class T>
static QVariant toVariant(const QList<T>& list) {
  QVariantList variant_list;
  variant_list.reserve(list.size());

  for (const auto& v : list) {
    variant_list.append(v);
  }

  return variant_list;
}

template <class T>
static QList<T> toList(const QVariant& qv) {
  QList<T> data_list;

  foreach (QVariant v, qv.value<QVariantList>()) {
    data_list << v.value<T>();
  }

  return data_list;
}

#endif // TEMPLATES_H
