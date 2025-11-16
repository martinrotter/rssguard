// For license of this file, see <project-root-folder>/LICENSE.md.

#pragma once

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <QList>

namespace qlinq {
  template <typename T>
  class Query {
    public:
      using value_type = T;

      Query() = default;
      explicit Query(const QList<T>& list) : _data(list) {}
      explicit Query(QList<T>&& list) : _data(std::move(list)) {}

      // Factory.
      static Query<T> from(const QList<T>& list) {
        return Query<T>(list);
      }

      static Query<T> from(QList<T>&& list) {
        return Query<T>(std::move(list));
      }

      // Basic iteration.
      auto begin() {
        return _data.begin();
      }

      auto end() {
        return _data.end();
      }

      auto begin() const {
        return _data.begin();
      }

      auto end() const {
        return _data.end();
      }

      // Materialization.
      QList<T> toList() const {
        return _data;
      }

      int count() const {
        return _data.size();
      }

      bool isEmpty() const {
        return _data.isEmpty();
      }

      // where: filter items.
      template <typename Pred>
      Query<T> where(Pred pred) const {
        QList<T> result;
        result.reserve(_data.size());
        for (const auto& item : _data) {
          if (pred(item)) {
            result.append(item);
          }
        }
        return Query<T>(std::move(result));
      }

      // select: project items.
      template <typename Func>
      auto select(Func func) const -> Query<typename std::decay<decltype(func(std::declval<T>()))>::type> {
        using ResultType = typename std::decay<decltype(func(std::declval<T>()))>::type;

        QList<ResultType> result;
        result.reserve(_data.size());
        for (const auto& item : _data) {
          result.append(func(item));
        }
        return Query<ResultType>(std::move(result));
      }

      // selectMany: project each item to a sequence and flatten.
      template <typename Func>
      auto selectMany(Func func) const -> Query<typename std::decay<decltype(*func(std::declval<T>()).begin())>::type> {
        using InnerList = decltype(func(std::declval<T>()));
        using InnerType = typename std::decay<decltype(*std::declval<InnerList>().begin())>::type;

        QList<InnerType> result;

        for (const auto& item : _data) {
          InnerList inner = func(item);
          for (const auto& x : inner) {
            result.append(x);
          }
        }

        return Query<InnerType>(std::move(result));
      }

      // take: first n items.
      Query<T> take(int n) const {
        if (n <= 0) {
          return Query<T>(QList<T>{});
        }

        if (n >= _data.size()) {
          return *this;
        }

        QList<T> result;
        result.reserve(n);
        for (int i = 0; i < n; ++i) {
          result.append(_data[i]);
        }
        return Query<T>(std::move(result));
      }

      // skip: skip first n items.
      Query<T> skip(int n) const {
        if (n <= 0) {
          return *this;
        }

        if (n >= _data.size()) {
          return Query<T>(QList<T>{});
        }

        QList<T> result;
        result.reserve(_data.size() - n);
        for (int i = n; i < _data.size(); ++i) {
          result.append(_data[i]);
        }
        return Query<T>(std::move(result));
      }

      // orderBy: sort by key selector (ascending).
      template <typename KeySelector>
      Query<T> orderBy(KeySelector keySelector) const {
        QList<T> result = _data;
        std::sort(result.begin(), result.end(), [&](const T& a, const T& b) {
          return keySelector(a) < keySelector(b);
        });
        return Query<T>(std::move(result));
      }

      // orderByDescending: sort by key selector (descending).
      template <typename KeySelector>
      Query<T> orderByDescending(KeySelector keySelector) const {
        QList<T> result = _data;
        std::sort(result.begin(), result.end(), [&](const T& a, const T& b) {
          return keySelector(a) > keySelector(b);
        });
        return Query<T>(std::move(result));
      }

      // any: does any element match?
      template <typename Pred>
      bool any(Pred pred) const {
        for (const auto& item : _data) {
          if (pred(item)) {
            return true;
          }
        }

        return false;
      }

      // all: do all elements match?
      template <typename Pred>
      bool all(Pred pred) const {
        for (const auto& item : _data) {
          if (!pred(item)) {
            return false;
          }
        }

        return true;
      }

      // min: return smallest element (requires operator< on T).
      std::optional<T> min() const {
        if (_data.isEmpty()) {
          return std::nullopt;
        }

        const T* best = &_data.first();

        for (const auto& item : _data) {
          if (item < *best) {
            best = &item;
          }
        }

        return *best;
      }

      // min with key selector (like LINQ's MinBy).
      template <typename KeySelector>
      std::optional<T> min(KeySelector keySelector) const {
        if (_data.isEmpty()) {
          return std::nullopt;
        }

        const T* best = &_data.first();
        auto bestKey = keySelector(*best);

        for (const auto& item : _data) {
          auto k = keySelector(item);

          if (k < bestKey) {
            best = &item;
            bestKey = k;
          }
        }

        return *best;
      }

      // max: return largest element (requires operator< on T).
      std::optional<T> max() const {
        if (_data.isEmpty()) {
          return std::nullopt;
        }

        const T* best = &_data.first();

        for (const auto& item : _data) {
          if (*best < item) {
            best = &item;
          }
        }

        return *best;
      }

      // max with key selector (like LINQ's MaxBy).
      template <typename KeySelector>
      std::optional<T> max(KeySelector keySelector) const {
        if (_data.isEmpty()) {
          return std::nullopt;
        }

        const T* best = &_data.first();
        auto bestKey = keySelector(*best);

        for (const auto& item : _data) {
          auto k = keySelector(item);

          if (bestKey < k) {
            best = &item;
            bestKey = k;
          }
        }

        return *best;
      }

      // count(value) – count occurrences of a specific value.
      int count(const T& value) const {
        int c = 0;
        for (const auto& item : _data) {
          if (item == value) {
            ++c;
          }
        }
        return c;
      }

      // for_each – apply a lambda to every element.
      template <typename Func>
      void for_each(Func func) const {
        for (const auto& item : _data) {
          func(item);
        }
      }

      // distinct – remove duplicate elements (requires operator==).
      Query<T> distinct() const {
        QList<T> result;
        for (const auto& item : _data) {
          if (!result.contains(item)) {
            result.append(item);
          }
        }
        return Query<T>(std::move(result));
      }

      // reverse – return elements in reversed order.
      Query<T> reverse() const {
        QList<T> result = _data;
        std::reverse(result.begin(), result.end());
        return Query<T>(std::move(result));
      }

      // sum(selector) – sum values produced by selector.
      template <typename Selector>
      auto sum(Selector selector) const -> typename std::decay<decltype(selector(std::declval<T>()))>::type {
        using R = typename std::decay<decltype(selector(std::declval<T>()))>::type;
        R total{};
        for (const auto& item : _data) {
          total += selector(item);
        }
        return total;
      }

      // first(): return first element, throw if empty.
      T first() const {
        if (_data.isEmpty()) {
          throw std::runtime_error("qlinq::first() called on an empty sequence");
        }

        return _data.first();
      }

      // first(predicate): return first matching element, throw if not found.
      template <typename Pred>
      T first(Pred pred) const {
        for (const auto& item : _data) {
          if (pred(item)) {
            return item;
          }
        }

        throw std::runtime_error("qlinq::first(predicate) found no matching element");
      }

      // firstOrDefault: optional first element.
      std::optional<T> firstOrDefault() const {
        if (_data.isEmpty()) {
          return std::nullopt;
        }

        return _data.first();
      }

      // firstOrDefault: optional first element matching predicate.
      template <typename Pred>
      std::optional<T> firstOrDefault(Pred pred) const {
        for (const auto& item : _data) {
          if (pred(item)) {
            return item;
          }
        }

        return std::nullopt;
      }

      // aggregate: fold with seed.
      template <typename Acc, typename Func>
      Acc aggregate(Acc seed, Func func) const {
        for (const auto& item : _data) {
          seed = func(seed, item);
        }
        return seed;
      }

    private:
      QList<T> _data;
  };

  // Helper free function so you can write qlinq::from(list).
  template <typename T>
  Query<T> from(const QList<T>& list) {
    return Query<T>::from(list);
  }

  template <typename T>
  Query<T> from(QList<T>&& list) {
    return Query<T>::from(std::move(list));
  }

} // namespace qlinq
