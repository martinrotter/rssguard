// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GLOBALS_H
#define GLOBALS_H

class Globals {
  public:
    template <typename T, typename U> static bool hasFlag(T lhs, U rhs);

  private:
    Globals();
};

template <typename T, typename U> inline bool Globals::hasFlag(T lhs, U rhs) {
  return (int(lhs) & int(rhs)) == int(rhs);
}

#endif // GLOBALS_H
