#!/bin/bash
# Commands executed when BabyKeySmash starts
# The keyboard serial is looked up at runtime so the script works on any
# machine , and does nothing at all when polychromatic is not installed .
command -v polychromatic-cli >/dev/null 2>&1 || exit 0

serials=$(polychromatic-cli -l --no-pretty-column 2>/dev/null | awk -F'  +' '$2 == "keyboard" { print $3 }')
[ -z "$serials" ] && exit 0

(
  for serial in $serials; do
    polychromatic-cli -s "$serial" -o ripple -p random
  done
) &
