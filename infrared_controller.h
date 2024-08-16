#pragma once

#include <stdbool.h>
#include "game_state.h"  // For LaserTagTeam enum

typedef struct InfraredController InfraredController;

/**
 * Allocate and initialize an InfraredController.
 * @return Pointer to the newly allocated InfraredController.
 */
InfraredController* infrared_controller_alloc();

/**
 * Free an InfraredController and its resources.
 * @param controller Pointer to the InfraredController to free.
 */
void infrared_controller_free(InfraredController* controller);

/**
 * Set the team for the InfraredController.
 * @param controller Pointer to the InfraredController.
 * @param team The team to set (TeamRed or TeamBlue).
 */
void infrared_controller_set_team(InfraredController* controller, LaserTagTeam team);

/**
 * Send an infrared signal corresponding to the controller's team.
 * @param controller Pointer to the InfraredController.
 */
void infrared_controller_send(InfraredController* controller);

/**
 * Check if a hit has been received from the opposite team.
 * @param controller Pointer to the InfraredController.
 * @return true if a hit was received, false otherwise.
 */
bool infrared_controller_receive(InfraredController* controller);

// IR command definitions
#define IR_COMMAND_RED_TEAM 0xA1
#define IR_COMMAND_BLUE_TEAM 0xB2