/* chimera-machine.h - ScummVM as a Chimera machine (chimera-machine.cpp).
 *
 * The calls wbx-entry.c makes, and the two the graphics manager makes back. */
#ifndef CHIMERA_MACHINE_H
#define CHIMERA_MACHINE_H

#include <stdint.h>

/* the picture's capacity (waterbox.config), the sound, the frame */
#define SCVM_MAX_W 2560
#define SCVM_MAX_H 1600
#define SCVM_AUDIO_RATE 44100
#define SCVM_FPS 60
#define SCVM_SAMPLES_PER_FRAME (SCVM_AUDIO_RATE / SCVM_FPS)
/* starting a game takes at most this many frames of the machine's time
 * (ScummVM's detection and the engine's start come before the movie's first
 * frame) */
#define SCVM_START_FRAMES (SCVM_FPS * 120)
/* a loop that reads the clock or asks for events this many times without
 * the clock moving ends the frame there (see chimera-machine.cpp) */
#define SCVM_SPIN_LIMIT 4096

#ifdef __cplusplus
extern "C" {
#endif

int scvm_init(char *err, int errsz);
void scvm_set_button(int index, int on);
void scvm_set_axis(int index, int value);
void scvm_frame(int render);
const uint32_t *scvm_video(int *w, int *h);
const int16_t *scvm_audio(int *n);
void scvm_aspect(int *x, int *y);
int scvm_input_was_read(void);
uint64_t scvm_frames(void);
int32_t scvm_saves_count(void);
const char *scvm_saves_name(int32_t i);
int64_t scvm_saves_size(int32_t i);
const uint8_t *scvm_saves_data(int32_t i);

#ifdef __cplusplus
}

/* the graphics manager's: the engine moved the pointer; a picture was shown */
void chimeraMachineMouseWarped(int x, int y);
void chimeraMachineScreenUpdated();
#endif

#endif
