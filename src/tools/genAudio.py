#!/usr/bin/env python3
import os
from pathlib import Path

AUDIO_DIR = f"{os.getcwd()}/audio"
OUTPUT = f"{os.getcwd()}/../audio.h"

def normalize(name: str) -> str:
    name = os.path.splitext(name)[0]
    name = name.upper()
    return name


def main():
    audio_path = Path(AUDIO_DIR)

    sounds = []
    music = []

    for file in sorted(audio_path.iterdir()):
        if not file.is_file():
            continue

        name = file.name

        if name.startswith("sound_"):
            enum_name = normalize(name)
            sounds.append((enum_name, name))

        elif name.startswith("music_"):
            enum_name = normalize(name)
            music.append((enum_name, name))

    with open(OUTPUT, "w") as f:

        f.write("#pragma once\n\n")
        f.write('#include "raylib.h"\n\n')

        # ---- Sound enum ----
        f.write("typedef enum SoundId {\n")
        for i, (enum_name, _) in enumerate(sounds):
            f.write(f"    {enum_name},\n")
        f.write(f"    SOUND_COUNT\n")
        f.write("} SoundId;\n\n")

        # ---- Music enum ----
        f.write("typedef enum MusicId {\n")
        for i, (enum_name, _) in enumerate(music):
            f.write(f"    {enum_name},\n")
        f.write(f"    MUSIC_COUNT\n")
        f.write("} MusicId;\n\n")

        # ---- Struct ----
        f.write("typedef struct Audio {\n")
        f.write("    Sound sounds[SOUND_COUNT];\n")
        f.write("    Music music[MUSIC_COUNT];\n")
        f.write("    int currentSongtrackID\n")
        f.write("} Audio;\n\n")

        # ---- file path tables ----
        f.write("static const char *sound_files[] = {\n")
        for _, name in sounds:
            f.write(f'    "audio/{name}",\n')
        f.write("};\n\n")

        f.write("static const char *music_files[] = {\n")
        for _, name in music:
            f.write(f'    "audio/{name}",\n')
        f.write("};\n\n")

    print(f"Generated {OUTPUT}")


if __name__ == "__main__":
    main()
