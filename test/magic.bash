#!/bin/bash

set -eux

: ${awk:=awk}
: ${file:=file}
: ${diff:=diff}
: ${xargs:=xargs}
: ${default_magic:=/usr/share/misc/magic}
: ${out_dir:=out}
src_dir="$1"

out="$out_dir"/magic.out
ref="$src_dir"/test/ref/magic.out
list="$src_dir"/test/ref/magic.list
magic="$src_dir"/config/ber_magic


"$awk" '{printf("'"$src_dir"'/%s\n", $0);}' "$list" | "$xargs" -d '\n' "$file" -m "$default_magic":"$magic" -bz > "$out"


"$diff" -u "$ref" "$out"


