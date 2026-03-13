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
} Audio;

static const char *sound_files[] = {
    "audio/sound_america.mp3",
    "audio/sound_bingchilling.mp3",
    "audio/sound_cardSelect.mp3",
    "audio/sound_erika.mp3",
    "audio/sound_explosionBlast.wav",
    "audio/sound_gun.wav",
    "audio/sound_hit.wav",
    "audio/sound_shield.wav",
};

static const char *music_files[] = {
    "audio/music_soundtrack.mp3",
};

