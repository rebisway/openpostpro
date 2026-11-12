#!/bin/sh
# Makes a Debian package independent of the version of Python of the system where it was built.
# The program is linked to libpython3.X.so, X being the version of the build system. Here it asks for
# libpython3.so instead, a link which the script postinst makes at the installation towards the
# libpython3.X.so.1.0 of the system (the program uses the stable ABI of Python 3.12, so any later version is fine).
#
# usage: portable-python.sh package.deb folder-of-the-linux-files
# needs: patchelf, dpkg-deb
set -e

deb="$1"
dir="$2"

if ! command -v patchelf >/dev/null 2>&1; then
	echo "portable-python: patchelf is not installed (sudo apt install patchelf): the package stays tied to the Python of this system" >&2
	exit 0
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

dpkg-deb -R "$deb" "$work/pkg"

exe="$work/pkg/usr/lib/openpostpro/openpostpro"
needed=$(patchelf --print-needed "$exe" | grep -E '^libpython3\.[0-9]+' | head -n 1 || true)
if [ -z "$needed" ]; then
	echo "portable-python: the program does not need libpython, nothing to do" >&2
	exit 0
fi

# the library is searched beside the program first, where postinst puts the link
patchelf --replace-needed "$needed" libpython3.so --set-rpath '$ORIGIN' "$exe"

# the dependency on the library of one version becomes a choice between the versions from 3.12
sed -i -E 's/libpython3\.[0-9]+[a-z0-9]*( \([^)]*\))?/libpython3.12t64 | libpython3.12 | libpython3.13 | libpython3.14 | libpython3.15/' "$work/pkg/DEBIAN/control"

install -m 755 "$dir/postinst" "$work/pkg/DEBIAN/postinst"
install -m 755 "$dir/postrm" "$work/pkg/DEBIAN/postrm"

(cd "$work/pkg" && find . -type f ! -path './DEBIAN/*' -printf '%P\0' | xargs -0 md5sum > DEBIAN/md5sums)

dpkg-deb --root-owner-group -b "$work/pkg" "$work/new.deb" >/dev/null
mv "$work/new.deb" "$deb"
echo "portable-python: $deb works with the Python 3.12 or later of the system (was $needed)"
