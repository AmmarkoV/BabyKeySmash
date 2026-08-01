#!/bin/sh
# Commands executed once when playtime is over ( --minutes N ) and the calm
# sleepy scene with the moon comes up . Put the keyboard in a slow dim
# breathing night colour so the room winds down together with the screen .
# The subshell keeps the application from blocking during the transition .
(
  polychromatic-cli -s IO2230F48700869 -o breath -p single -c "#101040"
  polychromatic-cli -s IO2230F48700869 -o brightness -p 30
) &
