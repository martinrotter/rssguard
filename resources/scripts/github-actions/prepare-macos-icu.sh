#!/bin/bash

set -euo pipefail

architecture="$1"
minimum_macos="$2"
script_directory="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
icu_root="$(brew --prefix icu4c)"

check_icu() {
  python3 "$script_directory/validate-macos-package.py" --architecture "$architecture" \
    --minimum-macos "$minimum_macos" --check-binary "$1/lib/libicuuc.dylib" >&2 &&
  python3 "$script_directory/validate-macos-package.py" --architecture "$architecture" \
    --minimum-macos "$minimum_macos" --check-binary "$1/lib/libicudata.dylib" >&2
}

if ! check_icu "$icu_root"; then
  # Bottles can require the runner's OS rather than our documented minimum macOS.
  # Homebrew supplies and verifies the source; build it with our deployment target.
  dependency_directory="$(pwd)/macos-icu"
  mkdir -p "$dependency_directory/source"
  brew unpack --destdir="$dependency_directory/source" icu4c >&2
  configure_scripts=()
  while IFS= read -r -d '' configure_script; do
    configure_scripts+=("$configure_script")
  done < <(find "$dependency_directory/source" -path '*/source/configure' -type f -print0)

  if (( ${#configure_scripts[@]} != 1 )); then
    echo "Expected one ICU source/configure script." >&2
    exit 1
  fi

  icu_root="$dependency_directory/install"
  (
    cd "$(dirname -- "${configure_scripts[0]}")"
    export MACOSX_DEPLOYMENT_TARGET="$minimum_macos"
    export CFLAGS="-arch $architecture -mmacosx-version-min=$minimum_macos"
    export CXXFLAGS="$CFLAGS"
    export LDFLAGS="$CFLAGS"
    ./configure --prefix="$icu_root" --disable-tests --disable-samples --disable-static
    make -j "$(sysctl -n hw.ncpu)"
    make install
  ) >&2
  check_icu "$icu_root"
fi

# This script's stdout is the CMake ICU_ROOT value; diagnostics go to stderr.
printf '%s\n' "$icu_root"
