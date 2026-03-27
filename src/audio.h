#pragma once

#include "raylib.h"

typedef enum SoundId {
    SOUND_AMERICA,
    SOUND_BINGCHILLING,
    SOUND_CARDSELECT,
    SOUND_ERIKA,
    SOUND_EXPLOSIONBLAST,
    SOUND_GUN,
    SOUND_HIT,
    SOUND_SHIELD,
    SOUND_COUNT
} SoundId;

typedef enum MusicId {
    MUSIC_SOUNDTRACK,
    MUSIC_COUNT
} MusicId;

typedef struct Audio {
    Sound sounds[SOUND_COUNT];
    Music music[MUSIC_COUNT];
    int currentSongtrackID;
} Audio;

static const char *sound_files[] = {
    "assets/audio/sound_america.mp3",
    "assets/audio/sound_bingchilling.mp3",
    "assets/audio/sound_cardSelect.mp3",
    "assets/audio/sound_erika.mp3",
    "assets/audio/sound_explosionBlast.wav",
    "assets/audio/sound_gun.wav",
    "assets/audio/sound_hit.wav",
    "assets/audio/sound_shield.wav",
};

static const char *music_files[] = {
    "assets/audio/music_soundtrack.mp3",
};

