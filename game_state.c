
#include "game_state.h"
#include <furi.h>
#include <stdlib.h>

struct GameState {
    LaserTagTeam team;
    uint8_t health;
    uint16_t score;
    uint16_t ammo;
    uint32_t game_time;
    bool game_over;
};

GameState* game_state_alloc() {
    GameState* state = malloc(sizeof(GameState));
    if(!state) {
        return NULL;
    }
    state->team = TeamRed;
    state->health = INITIAL_HEALTH;
    state->score = 0;
    state->ammo = INITIAL_AMMO;
    state->game_time = 0;
    state->game_over = false;
    return state;
}

void game_state_reset(GameState* state) {
    furi_assert(state);
    state->health = INITIAL_HEALTH;
    state->score = 0;
    state->ammo = INITIAL_AMMO;
    state->game_time = 0;
    state->game_over = false;
}

void game_state_set_team(GameState* state, LaserTagTeam team) {
    furi_assert(state);
    state->team = team;
}

LaserTagTeam game_state_get_team(GameState* state) {
    furi_assert(state);
    return state->team;
}

void game_state_decrease_health(GameState* state, uint8_t amount) {
    furi_assert(state);
    if(state->health > amount) {
        state->health -= amount;
    } else {
        state->health = 0;
        state->game_over = true;
    }
}

void game_state_increase_health(GameState* state, uint8_t amount) {
    furi_assert(state);
    state->health = (state->health + amount > MAX_HEALTH) ? MAX_HEALTH : state->health + amount;
}

uint8_t game_state_get_health(GameState* state) {
    furi_assert(state);
    return state->health;
}

void game_state_increase_score(GameState* state, uint16_t points) {
    furi_assert(state);
    state->score += points;
}

uint16_t game_state_get_score(GameState* state) {
    furi_assert(state);
    return state->score;
}

void game_state_decrease_ammo(GameState* state, uint16_t amount) {
    furi_assert(state);
    if(state->ammo > amount) {
        state->ammo -= amount;
    } else {
        state->ammo = 0;
    }
}

void game_state_increase_ammo(GameState* state, uint16_t amount) {
    furi_assert(state);
    state->ammo += amount;
}

uint16_t game_state_get_ammo(GameState* state) {
    furi_assert(state);
    return state->ammo;
}

void game_state_update_time(GameState* state, uint32_t delta_time) {
    furi_assert(state);
    state->game_time += delta_time;
}

uint32_t game_state_get_time(GameState* state) {
    furi_assert(state);
    return state->game_time;
}

bool game_state_is_game_over(GameState* state) {
    furi_assert(state);
    return state->game_over;
}

void game_state_set_game_over(GameState* state, bool game_over) {
    furi_assert(state);
    state->game_over = game_over;
}
