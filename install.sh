#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

red=$(printf "\033[31m")
green=$(printf "\033[32m")
normal=$(printf "\033[m")

if [ $( id -u ) -eq 0 ]; then
 echo $red
 echo "Do not run the installer as root , it would make build/ root owned .."
 echo "Please re run using ./install.sh , it will ask for your password when"
 echo "it needs to copy files to the system , exiting now.."
 echo $normal
 exit 1
fi

if command -v apt-get >/dev/null 2>&1; then
  echo "Making sure the dependencies are installed , your password is required.."
  sudo apt-get install -y cmake g++ pkg-config libx11-dev libglew-dev \
                          libopencv-dev libasound2-dev libvorbis-dev espeak-ng
  if [ $? -ne 0 ]; then
    echo $red
    echo "Could not install the dependencies , see INSTALL for the list"
    echo $normal
    exit 1
  fi
else
  echo "This is not an apt based system , see INSTALL for the dependency list"
fi

cmake -S . -B build && cmake --build build -j$(nproc)

if [ -e build/babykeysmash ]
then
  echo $green
  echo "BabyKeySmash binary is OK :) , including it to system binaries .."
  echo $normal
  sudo cp build/babykeysmash /usr/bin/babykeysmash
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
  sudo mkdir /usr/share/babykeysmash
fi

sudo cp -r shaders textures sounds scripts /usr/share/babykeysmash/
sudo chmod 755 /usr/share/babykeysmash/scripts/*.sh

sudo cp babykeysmash.desktop /usr/share/applications/babykeysmash.desktop
sudo cp textures/emoji_teddy.png /usr/share/icons/babykeysmash.png

TIME_STAMP=`date`
touch babykeysmash_install.log
echo "$TIME_STAMP" >> babykeysmash_install.log

echo "Done , run babykeysmash ( or babykeysmash --greek ) to play"
exit 0
