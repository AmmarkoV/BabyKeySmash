#!/bin/sh
# Commands executed when BabyKeySmash exits , restore keyboard lighting .
# The brightness is put back to full because scripts/on_sleep.sh dims it
# when playtime ends .
(
  polychromatic-cli -s IO2230F48700869 -o reactive -p 4 -c "720095"
  polychromatic-cli -s IO2230F48700869 -o brightness -p 100
) &
