// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GLOBALS_H
#define GLOBALS_H

class Globals {
  public:
    template <typename T, typename U>
    static bool hasFlag(T value, U flag);

  private:
    Globals();
};

template <typename T, typename U>
inline bool Globals::hasFlag(T value, U flag) {
  return (int(value) & int(flag)) == int(flag);
}

#endif // GLOBALS_H
