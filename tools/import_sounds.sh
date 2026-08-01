#!/bin/sh
# Convert sound effects into the fixed ogg/vorbis format BabyKeySmash plays
# ( 44100 Hz , stereo , trimmed to 4 seconds , quality 4 ) . Ogg keeps the
# repository small ; decoding happens at startup through libvorbisfile .
# Usage : tools/import_sounds.sh [sourceDirectory]
# Every sounds/*.ogg is picked up at startup , delete the ones you dislike .
SRC="${1:-$HOME/Music/Sounds/Simple}"
DST="$(dirname "$0")/../sounds"
mkdir -p "$DST"
for f in "$SRC"/*.mp3 ; do
  [ -f "$f" ] || continue
  base=$(basename "$f" .mp3)
  ffmpeg -y -loglevel error -i "$f" -t 4 -ar 44100 -ac 2 -c:a libvorbis -q:a 4 "$DST/$base.ogg" \
    && echo "wrote $DST/$base.ogg"
done
