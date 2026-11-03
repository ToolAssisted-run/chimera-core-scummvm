#!/usr/bin/env bash
# Fetches the libraries' sources named in deps.lock: each tarball is
# downloaded once into build/deps/ (or $CHIMERA_DEPS_DIR, which may hold them
# already: nothing is downloaded then), checked against its SHA-256 - a
# tarball that is not the pinned one stops the build - and unpacked into
# build/src/<folder>. A folder already unpacked from the pinned tarball is
# left as it is. Writes build/src/deps.stamp, which deps.mk builds from.
#
# usage: fetch-deps.sh
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
cache="${CHIMERA_DEPS_DIR:-$root/build/deps}"
src="$root/build/src"
mkdir -p "$cache" "$src"

while read -r name version folder sha urls; do
	case "$name" in ''|'#'*) continue ;; esac
	first="${urls%% *}"
	case "$first" in
		*.tar.xz) ext=tar.xz ;;
		*.tar.gz) ext=tar.gz ;;
		*) echo "fetch-deps: $name: no tarball in $first" >&2; exit 1 ;;
	esac
	tarball="$cache/$name-$version.$ext"
	if [ ! -f "$tarball" ] || ! echo "$sha  $tarball" | sha256sum -c - >/dev/null 2>&1; then
		rm -f "$tarball"
		for url in $urls; do
			echo "fetch-deps: $name $version from $url"
			if curl -fsSL --retry 3 -m 600 -o "$tarball.part" "$url"; then
				mv "$tarball.part" "$tarball"
				break
			fi
			rm -f "$tarball.part"
		done
		[ -f "$tarball" ] || { echo "fetch-deps: $name $version could not be downloaded" >&2; exit 1; }
		echo "$sha  $tarball" | sha256sum -c - >/dev/null 2>&1 || {
			echo "fetch-deps: $tarball is not the pinned $name $version (SHA-256 $(sha256sum "$tarball" | cut -c1-64), want $sha)" >&2
			rm -f "$tarball"
			exit 1
		}
	fi
	if [ "$(cat "$src/$folder/.chimera-sha256" 2>/dev/null)" != "$sha" ]; then
		rm -rf "${src:?}/${folder:?}"
		tar -xf "$tarball" -C "$src"
		[ -d "$src/$folder" ] || { echo "fetch-deps: $tarball did not unpack into $folder" >&2; exit 1; }
		echo "$sha" > "$src/$folder/.chimera-sha256"
	fi
done < "$here/deps.lock"

sha256sum < "$here/deps.lock" > "$src/deps.stamp.new"
cmp -s "$src/deps.stamp.new" "$src/deps.stamp" 2>/dev/null && rm -f "$src/deps.stamp.new" || mv "$src/deps.stamp.new" "$src/deps.stamp"
echo "fetch-deps: $(grep -c '^[a-z]' "$here/deps.lock") libraries in $src"
