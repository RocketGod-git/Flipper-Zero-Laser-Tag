#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SoundGeneratorModePlay,
    SoundGeneratorModeStop,
} SoundGeneratorMode;

typedef enum {
    SoundGeneratorSoundBeep,
    SoundGeneratorSoundTone,
    SoundGeneratorSoundNoise,
} SoundGeneratorSound;

typedef struct SoundGenerator SoundGenerator;

SoundGenerator* sound_generator_alloc();
void sound_generator_free(SoundGenerator* sound_generator);

void sound_generator_set_mode(SoundGenerator* sound_generator, SoundGeneratorMode mode);
void sound_generator_start(SoundGenerator* sound_generator);
void sound_generator_stop(SoundGenerator* sound_generator);

void sound_generator_set_sound(SoundGenerator* sound_generator, SoundGeneratorSound sound);
void sound_generator_set_frequency(SoundGenerator* sound_generator, uint16_t frequency);
void sound_generator_set_volume(SoundGenerator* sound_generator, uint8_t volume);
