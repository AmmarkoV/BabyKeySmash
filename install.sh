#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

red=$(printf "\033[31m")
green=$(printf "\033[32m")
normal=$(printf "\033[m")

if [ $( id -u ) -eq 0 ]; then
 echo "Will begin installation now"
else
 echo $red
 echo "Installer must be run as root in order to copy files to system.."
 echo "Please re run using sudo ./install.sh , exiting now.."
 echo $normal
 exit 0
fi

cmake -S . -B build && cmake --build build -j$(nproc)

if [ -e build/babykeysmash ]
then
  echo $green
  echo "BabyKeySmash binary is OK :) , including it to system binaries .."
  echo $normal
  cp build/babykeysmash /usr/bin/babykeysmash
else
  echo $red
  echo "BabyKeySmash could not be built , you probably got a library missing"
  echo "See INSTALL for the dependency list"
  echo $normal
  exit 1
fi

if [ -d "/usr/share/babykeysmash" ]; then
  echo "BabyKeySmash installation detected , patching it up :)"
else
  echo "Installing BabyKeySmash in the system.. :)"
  mkdir /usr/share/babykeysmash
fi

cp -r shaders textures sounds scripts /usr/share/babykeysmash/
chmod 755 /usr/share/babykeysmash/scripts/*.sh

TIME_STAMP=`date`
touch babykeysmash_install.log
echo "$TIME_STAMP" >> babykeysmash_install.log

echo "Done , run babykeysmash ( or babykeysmash --greek ) to play"
exit 0
