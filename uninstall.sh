#!/bin/bash

if [ -d "/usr/share/babykeysmash" ]; then
  echo "BabyKeySmash installation detected , uninstalling it "
  sudo rm -rf /usr/share/babykeysmash
else
  echo "No installed program resources found"
fi

if [ -e /usr/bin/babykeysmash ]
then
  sudo rm /usr/bin/babykeysmash
else
  echo "No BabyKeySmash binary detected"
fi

echo "Done"
exit 0
