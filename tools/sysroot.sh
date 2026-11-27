#!/bin/sh
# Q2 runtime from a pinned upstream firmware; headers from Debian buster.
set -eu
out=${1:-"$HOME/.cache/q2/sysroot"}
mkdir -p "$out/lib" "$out/usr/include"
out=$(cd "$out" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
curl -fL --retry 3 --connect-timeout 20 https://github.com/DiamondBond/q2-pod/releases/download/1.9.5/update.tar -o "$tmp/update.tar"
echo "c62c565bc193c3555b46499f17d965f8c54ae86e4907c118cb06ab679379bf01  $tmp/update.tar" | sha256sum -c -
tar -xOf "$tmp/update.tar" recovery-update/rootfs.squashfs > "$tmp/rootfs.squashfs"
while read -r checksum relative; do
    curl -fsSL --retry 3 "https://archive.debian.org/debian/pool/main/$relative" -o "$tmp/p.deb"
    printf '%s  %s\n' "$checksum" "$tmp/p.deb" | sha256sum -c -
    dpkg-deb --fsys-tarfile "$tmp/p.deb" | tar -xf - -C "$out" ./usr/include
done <<'PACKAGES'
e28bac8637e98d78045f0fd05613ed2ab298fb1aa6387633b6d609119774d590 g/glibc/libc6-dev_2.28-10+deb10u1_mipsel.deb
5f6ca1c34bb326b38fa44838b1ed051c33a2591bf7bfa26276e54fc85c00427c l/linux/linux-libc-dev_4.19.249-2_mipsel.deb
PACKAGES
# Obtain ALSA headers from the archive, verifying against its signed package index.
curl -fsSL --retry 3 https://archive.debian.org/debian/dists/buster/Release -o "$tmp/Release"
curl -fsSL --retry 3 https://archive.debian.org/debian/dists/buster/Release.gpg -o "$tmp/Release.gpg"
gpgv --keyring /usr/share/keyrings/debian-archive-keyring.gpg --keyring /usr/share/keyrings/debian-archive-removed-keys.gpg "$tmp/Release.gpg" "$tmp/Release"
checksum=$(awk '/^SHA256:/{f=1;next} /^[^ ]/{f=0} f && $3=="main/binary-mipsel/Packages.xz"{print $1}' "$tmp/Release")
test -n "$checksum"
curl -fsSL --retry 3 https://archive.debian.org/debian/dists/buster/main/binary-mipsel/Packages.xz -o "$tmp/Packages.xz"
printf '%s  %s\n' "$checksum" "$tmp/Packages.xz" | sha256sum -c -
xz -dc "$tmp/Packages.xz" | awk -v RS='' -F '\n' '$1=="Package: libasound2-dev" {for(i=1;i<=NF;i++){if($i ~ /^Filename: /) file=substr($i,11); if($i ~ /^SHA256: /) sha=substr($i,9)} print sha, file}' > "$tmp/alsa"
read -r checksum relative < "$tmp/alsa"
test -n "$checksum" && test -n "$relative"
curl -fsSL --retry 3 "https://archive.debian.org/debian/$relative" -o "$tmp/alsa.deb"
printf '%s  %s\n' "$checksum" "$tmp/alsa.deb" | sha256sum -c -
dpkg-deb --fsys-tarfile "$tmp/alsa.deb" | tar -xf - -C "$out" ./usr/include
cp "$out/usr/include/mipsel-linux-gnu/gnu/stubs-o32_hard.h" "$out/usr/include/mipsel-linux-gnu/gnu/stubs-o32_hard_2008.h"
unsquashfs -q -n -d "$tmp/root" "$tmp/rootfs.squashfs" lib usr/lib >/dev/null
for lib in ld-linux-mipsn8.so.1 libc.so.6 libpthread.so.0 libasound.so.2 libgcc_s.so.1 libm.so.6; do
    if [ -f "$tmp/root/lib/$lib" ]; then
        cp -L "$tmp/root/lib/$lib" "$out/lib/$lib"
    elif [ -f "$tmp/root/usr/lib/$lib" ]; then
        cp -L "$tmp/root/usr/lib/$lib" "$out/lib/$lib"
    else
        echo "Missing Q2 runtime library: $lib" >&2
        exit 1
    fi
done
test -f "$out/usr/include/alsa/asoundlib.h"
