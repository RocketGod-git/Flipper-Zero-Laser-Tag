#include "laser_tag_view.h"
#include "laser_tag_icons.h"
#include <furi.h>
#include <gui/elements.h>

struct LaserTagView {
    View* view;
};

typedef struct {
    LaserTagTeam team;
    uint8_t health;
    uint16_t ammo;
    uint16_t score;
    uint32_t game_time;
    bool game_over;
} LaserTagViewModel;

static void laser_tag_view_draw_callback(Canvas* canvas, void* model) {
    LaserTagViewModel* m = model;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_draw_icon(canvas, 0, 0, m->team == TeamRed ? TEAM_RED_ICON : TEAM_BLUE_ICON);
    canvas_draw_icon(canvas, 0, 10, HEALTH_ICON);
    canvas_draw_str_aligned(canvas, 20, 14, AlignLeft, AlignBottom, furi_string_get_cstr(furi_string_alloc_printf("%d", m->health)));
    canvas_draw_icon(canvas, 0, 20, AMMO_ICON);
    canvas_draw_str_aligned(canvas, 20, 24, AlignLeft, AlignBottom, furi_string_get_cstr(furi_string_alloc_printf("%d", m->ammo)));
    canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignBottom, "Score:");
    canvas_draw_str_aligned(canvas, 64, 20, AlignCenter, AlignBottom, furi_string_get_cstr(furi_string_alloc_printf("%d", m->score)));
    uint32_t minutes = m->game_time / 60;
    uint32_t seconds = m->game_time % 60;
    canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignBottom, furi_string_get_cstr(furi_string_alloc_printf("%02ld:%02ld", minutes, seconds)));
    canvas_draw_icon(canvas, 112, 0, LASER_GUN_ICON);

    if(m->game_over) {
        canvas_draw_icon(canvas, 56, 28, GAME_OVER_ICON);
    }
}

static bool laser_tag_view_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false;
}

LaserTagView* laser_tag_view_alloc() {
    LaserTagView* laser_tag_view = malloc(sizeof(LaserTagView));
    laser_tag_view->view = view_alloc();
    view_set_context(laser_tag_view->view, laser_tag_view);
    view_allocate_model(laser_tag_view->view, ViewModelTypeLocking, sizeof(LaserTagViewModel));
    view_set_draw_callback(laser_tag_view->view, laser_tag_view_draw_callback);
    view_set_input_callback(laser_tag_view->view, laser_tag_view_input_callback);
    return laser_tag_view;
}

void laser_tag_view_draw(View* view, Canvas* canvas) {
    LaserTagViewModel* model = view_get_model(view);
    laser_tag_view_draw_callback(canvas, model);
    view_commit_model(view, false);
}

void laser_tag_view_free(LaserTagView* laser_tag_view) {
    furi_assert(laser_tag_view);
    view_free(laser_tag_view->view);
    free(laser_tag_view);
}

View* laser_tag_view_get_view(LaserTagView* laser_tag_view) {
    furi_assert(laser_tag_view);
    return laser_tag_view->view;
}

void laser_tag_view_update(LaserTagView* laser_tag_view, GameState* game_state) {
    furi_assert(laser_tag_view);
    furi_assert(game_state);

    with_view_model(
        laser_tag_view->view,
        LaserTagViewModel * model,
        {
            model->team = game_state_get_team(game_state);
            model->health = game_state_get_health(game_state);
            model->ammo = game_state_get_ammo(game_state);
            model->score = game_state_get_score(game_state);
            model->game_time = game_state_get_time(game_state);
            model->game_over = game_state_is_game_over(game_state);
        },
        true);
}
