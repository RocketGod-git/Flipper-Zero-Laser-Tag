#pragma once

#include <stdbool.h>
#include "game_state.h"

typedef struct InfraredController InfraredController;

InfraredController* infrared_controller_alloc();
void infrared_controller_free(InfraredController* controller);
void infrared_controller_set_team(InfraredController* controller, LaserTagTeam team);
void infrared_controller_send(InfraredController* controller);
bool infrared_controller_receive(InfraredController* controller);
void infrared_controller_pause(InfraredController* controller);
void infrared_controller_resume(InfraredController* controller);

#define IR_COMMAND_RED_TEAM  0xA1
#define IR_COMMAND_BLUE_TEAM 0xB2
