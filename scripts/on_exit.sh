#!/bin/sh
# Commands executed when BabyKeySmash exits , restore keyboard lighting
polychromatic-cli -s IO2230F48700869 -o reactive -p 4 -c "720095" &
