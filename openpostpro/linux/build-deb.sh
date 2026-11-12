#!/bin/sh
# Installs what the build needs, compiles OpenPostPro and makes the Debian package.
# Run it on the OLDEST system the package must work on (Ubuntu 24.04 or Linux Mint 22): a program compiled on a newer
# system does not run on an older one.
#
#   sh openpostpro/linux/build-deb.sh          (from anywhere in the repository)
#
# The program is in openpostpro/bin, the package in openpostpro/build-linux.
set -e

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)		# the folder with CMakeLists.txt
build="$root/build-linux"

if ! command -v apt-get >/dev/null 2>&1; then
	echo "This script is for Debian, Ubuntu and their derivatives (apt)." >&2
	exit 1
fi

echo "== Packages of the build (sudo asks for the password)"
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config git patchelf \
	libglfw3-dev libfreetype-dev libgl1-mesa-dev libx11-dev \
	python3-dev python3-tk zenity

if ! python3 -c 'import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)'; then
	echo "Python 3.12 or later is needed, this system has $(python3 --version 2>&1)." >&2
	exit 1
fi

echo "== System: $(. /etc/os-release && echo "$PRETTY_NAME"), glibc $(ldd --version | head -n 1 | awk '{print $NF}'), $(python3 --version)"

echo "== Build"
cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" -j "$(nproc)"

echo "== Package"
rm -f "$build"/openpostpro_*.deb
(cd "$build" && cpack -G DEB)

deb=$(ls "$build"/openpostpro_*.deb)
echo
echo "== Result: $deb"
dpkg -I "$deb" | grep -E 'Version|Depends|Recommends'
echo
echo "Install it with: sudo apt install $deb"
