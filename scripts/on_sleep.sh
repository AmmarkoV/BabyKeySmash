#!/bin/bash
# Commands executed once when playtime is over ( --minutes N ) and the calm
# sleepy scene with the moon comes up . Put the keyboard in a slow dim
# breathing night colour so the room winds down together with the screen .
# The subshell keeps the application from blocking during the transition .
# The keyboard serial is looked up at runtime so the script works on any
# machine , and does nothing at all when polychromatic is not installed .
command -v polychromatic-cli >/dev/null 2>&1 || exit 0

serials=$(polychromatic-cli -l --no-pretty-column 2>/dev/null | awk -F'  +' '$2 == "keyboard" { print $3 }')
[ -z "$serials" ] && exit 0

(
  for serial in $serials; do
    polychromatic-cli -s "$serial" -o breath -p single -c "#101040"
    polychromatic-cli -s "$serial" -o brightness -p 30
  done
) &
