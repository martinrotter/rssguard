#!/bin/sh

set -eux

ARCH="$(uname -m)"
EXTRA_PACKAGES="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/get-debloated-pkgs.sh"

pacman -Syu --noconfirm \
	base-devel       \
	curl             \
	cmake			 \
	git              \
	gtk3             \
	libxcb           \
	libxcursor       \
	libxi            \
	libxkbcommon     \
	libxkbcommon-x11 \
	libxrandr        \
	libxtst          \
	mariadb-clients  \
	mpv				 \
	pipewire-audio   \
	pulseaudio       \
	pulseaudio-alsa  \
	qt6ct            \
	qt6-wayland      \
	gcc-libs		 \
	glibc			 \
	libglvnd		 \
	qt6-5compat	     \
	qt6-tools        \
	qt6-base	     \
	qt6-declarative	 \
	qt6-multimedia   \
	sqlite 			 \
	wget             \
	xorg-server-xvfb \
	zsync

echo "Installing debloated packages..."
echo "---------------------------------------------------------------"
wget --retry-connrefused --tries=30 "$EXTRA_PACKAGES" -O ./get-debloated-pkgs.sh
chmod +x ./get-debloated-pkgs.sh
./get-debloated-pkgs.sh --add-mesa --prefer-nano opus-mini

#pacman -Q rssguard | awk '{print $2; exit}' > ~/version