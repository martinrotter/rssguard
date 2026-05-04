#!/bin/sh

set -eux

use_qt5="$1"

ARCH="$(uname -m)"
EXTRA_PACKAGES="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/get-debloated-pkgs.sh"

pacman-key --init
pacman -Syy --noconfirm
pacman -S --noconfirm archlinux-keyring
pacman-key --recv-keys bd2ac8c5e989490c --keyserver keyserver.ubuntu.com
pacman-key --lsign-key bd2ac8c5e989490c

cat >> /etc/pacman.conf <<'EOF'

[arcanisrepo]
Server = https://repo.arcanis.me/arcanisrepo/x86_64
SigLevel = DatabaseRequired PackageNever TrustedOnly
EOF

pacman -Syy --noconfirm

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
    sndio                \
    sqlite               \
    wget                 \
    xorg-server-xvfb     \
    zsync

if [[ "$use_qt5" == "ON" ]]; then
pacman -Syu --noconfirm  \
    qt5-base             \
    qt5-declarative      \
    qt5-imageformats     \
    qt5-multimedia       \
    qt5-tools            \
    qt5-wayland          \
    qt5-webengine        \
    qt5-svg
else
pacman -Syu --noconfirm  \
    qxmpp                \
    qt6-base             \
    qt6-declarative      \
    qt6-imageformats     \
    qt6-multimedia       \
    qt6-tools            \
    qt6-svg              \
    qt6-webengine        \
    qt6ct
fi

echo "Installing debloated packages..."
echo "---------------------------------------------------------------"
wget --retry-connrefused --tries=30 "$EXTRA_PACKAGES" -O ./get-debloated-pkgs.sh

chmod +x ./get-debloated-pkgs.sh
COMMON_PACKAGES=1 ./get-debloated-pkgs.sh --add-mesa --prefer-nano

#pacman -Q rssguard | awk '{print $2; exit}' > ~/version
