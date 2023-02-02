// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GLOBALS_H
#define GLOBALS_H

class Globals {
  public:
    template <typename T> static bool hasFlag(T lhs, T rhs);

  private:
    Globals();
};

template <typename T> inline bool Globals::hasFlag(T lhs, T rhs) {
  return (int(lhs) & int(rhs)) == int(rhs);
}

#endif // GLOBALS_H
