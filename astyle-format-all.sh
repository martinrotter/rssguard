#!/bin/bash
#
# This file is part of RSS Guard.
#
# Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
# Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
#
# RSS Guard is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RSS Guard is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RSS Guard. If not, see <http:#www.gnu.org/licenses/>.

function usage {
  echo "Usage: $0 \"root-directory\"..."
  exit 1
}

if [ $# -eq 0 ]; then
  usage
fi

ASTYLE_CMD="astyle"
ASTYLE_RC=".astylerc"

# Check all args.
for dir in "$@"; do
    if [ ! -d "${dir}" ]; then
        echo "\"${dir}\" is not a directory..."
        usage
    fi
done

# Run the thing.
for dir in "$@"; do
  pushd "${dir}"
  
  if [ ! -r "$ASTYLE_RC" ]; then
    echo "No $ASTYLE_RC in pwd \"$PWD\"..."
    continue
  fi
  
  for f in $(find . \
                -name '*.c' \
                -o -name '*.cc' \
                -o -name '*.cpp' \
                -o -name '*.h' \
                -o -name '*.hh' \
                -o -name '*.hpp'); do
    "${ASTYLE_CMD}" --options="$ASTYLE_RC" "${f}"
  done
  
  # Remove backup files.
  find . -name "*.orig" | xargs --no-run-if-empty rm -v
  
  popd
done
