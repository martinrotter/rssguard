#!/bin/sh

set -eux

ARCH="$(uname -m)"
EXTRA_PACKAGES="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/get-debloated-pkgs.sh"

pacman-key --init
pacman -Syy --noconfirm
pacman -S --noconfirm archlinux-keyring

pacman -Syu --noconfirm  \
    appstream            \
    base-devel           \
    cmake                \
    curl                 \
    gcc-libs             \
    git                  \
    glibc                \
    go                   \
    gtk3                 \
    icu                  \
    libheif              \
    libglvnd             \
    libxcb               \
    libxcursor           \
    libxi                \
    libxkbcommon         \
    libxkbcommon-x11     \
    libxrandr            \
    libxtst              \
    make                 \
    mariadb-clients      \
    mpv                  \
    ninja                \
    pipewire-audio       \
    pulseaudio           \
    pulseaudio-alsa      \
    qt5-base             \
    qt5-declarative      \
    qt5-imageformats     \
    qt5-multimedia       \
    qt5-tools            \
    qt5-wayland          \
    qt5-svg              \
    qt6-base             \
    qt6-declarative      \
    qt6-imageformats     \
    qt6-multimedia       \
    qt6-tools            \
    qt6-wayland          \
    qt6-svg              \
    qt6ct                \
    sqlite               \
    wget                 \
    xorg-server-xvfb     \
    zsync

echo "Installing debloated packages..."
echo "---------------------------------------------------------------"
wget --retry-connrefused --tries=30 "$EXTRA_PACKAGES" -O ./get-debloated-pkgs.sh
chmod +x ./get-debloated-pkgs.sh
./get-debloated-pkgs.sh --add-mesa --prefer-nano opus-mini librsvg-mini gdk-pixbuf2-mini

#pacman -Q rssguard | awk '{print $2; exit}' > ~/version