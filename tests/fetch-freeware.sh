#!/bin/sh
# Downloads the freeware games the tests play into a folder, each checked by
# its SHA-1. They are scummvm.org's own downloads, each free to redistribute
# by the licence inside its zip (a readme, LICENSE.txt or license.txt); none
# of them is ever put in this repository. CI runs this; locally, give the
# tests a folder that already has them (run-gate.sh -g <folder>).
#
# usage: tests/fetch-freeware.sh <folder>
set -eu
dest="${1:?usage: fetch-freeware.sh <folder>}"
mkdir -p "$dest"
base="https://downloads.scummvm.org/frs/extras"
while read -r sha1 name path; do
	[ -n "$sha1" ] || continue
	out="$dest/$name"
	if [ ! -f "$out" ] || [ "$(sha1sum "$out" | cut -c1-40)" != "$sha1" ]; then
		curl -sfL --retry 3 -o "$out.part" "$base/$path"
		mv "$out.part" "$out"
	fi
	got="$(sha1sum "$out" | cut -c1-40)"
	[ "$got" = "$sha1" ] || { echo "fetch-freeware: $name is $got, not $sha1" >&2; exit 1; }
	echo "fetch-freeware: $name"
done <<'EOF'
acd7a578879c73fb3994b9c72b6571dc62635f6e BASS-Floppy-1.3.zip Beneath%20a%20Steel%20Sky/BASS-Floppy-1.3.zip
ff6bdc0b6a0daeea1a62d68118c7e994483425ce FOTAQ_Floppy.zip Flight%20of%20the%20Amazon%20Queen/FOTAQ_Floppy.zip
b7d430b17c361396ccf81734148cb0abb4bb4abb lure-1.1.zip Lure%20of%20the%20Temptress/lure-1.1.zip
0cb91c9e56bc963d17a3c6afe15b76430dd2c6fe drascula-1.0.zip Drascula_%20The%20Vampire%20Strikes%20Back/drascula-1.0.zip
2d4a9a438f529c455fc34350380958d75916c077 dreamweb-uk-1.1.zip Dreamweb/dreamweb-uk-1.1.zip
EOF
