#include "game_state.h"
#include <furi.h>

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
    state->team = TeamRed;
    state->health = 100;
    state->score = 0;
    state->ammo = 100;
    state->game_time = 0;
    state->game_over = false;
    return state;
}

void game_state_free(GameState* state) {
    free(state);
}

void game_state_reset(GameState* state) {
    state->health = 100;
    state->score = 0;
    state->ammo = 100;
    state->game_time = 0;
    state->game_over = false;
}

void game_state_set_team(GameState* state, LaserTagTeam team) {
    state->team = team;
}

LaserTagTeam game_state_get_team(GameState* state) {
    return state->team;
}

void game_state_decrease_health(GameState* state, uint8_t amount) {
    if(state->health > amount) {
        state->health -= amount;
    } else {
        state->health = 0;
        state->game_over = true;
    }
}

void game_state_increase_health(GameState* state, uint8_t amount) {
    state->health = (state->health + amount > 100) ? 100 : state->health + amount;
}

uint8_t game_state_get_health(GameState* state) {
    return state->health;
}

void game_state_increase_score(GameState* state, uint16_t points) {
    state->score += points;
}

uint16_t game_state_get_score(GameState* state) {
    return state->score;
}

void game_state_decrease_ammo(GameState* state, uint16_t amount) {
    if(state->ammo > amount) {
        state->ammo -= amount;
    } else {
        state->ammo = 0;
    }
}

void game_state_increase_ammo(GameState* state, uint16_t amount) {
    state->ammo += amount;
}

uint16_t game_state_get_ammo(GameState* state) {
    return state->ammo;
}

void game_state_update_time(GameState* state, uint32_t delta_time) {
    state->game_time += delta_time;
}

uint32_t game_state_get_time(GameState* state) {
    return state->game_time;
}

bool game_state_is_game_over(GameState* state) {
    return state->game_over;
}

void game_state_set_game_over(GameState* state, bool game_over) {
    state->game_over = game_over;
}