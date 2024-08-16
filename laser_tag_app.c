#include "laser_tag_app.h"
#include "laser_tag_view.h"
#include "infrared_controller.h"
#include "game_state.h"
#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#define TAG "LaserTag"

struct LaserTagApp {
    Gui* gui;
    ViewPort* view_port;
    LaserTagView* view;
    FuriMessageQueue* event_queue;
    FuriTimer* timer;
    NotificationApp* notifications;
    InfraredController* ir_controller;
    GameState* game_state;
    LaserTagState state;
};

const NotificationSequence sequence_vibro_1 = {
    &message_vibro_on,
    &message_vibro_off,
    NULL
};

static void laser_tag_app_timer_callback(void* context) {
    furi_assert(context);
    LaserTagApp* app = context;
    game_state_update_time(app->game_state, 1);
    laser_tag_view_update(app->view, app->game_state);
}

static void laser_tag_app_input_callback(InputEvent* input_event, void* context) {
    furi_assert(context);
    LaserTagApp* app = context;
    furi_message_queue_put(app->event_queue, input_event, FuriWaitForever);
}

static void laser_tag_app_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    LaserTagApp* app = context;
    if(app->state == LaserTagStateTeamSelect) {
        canvas_clear(canvas);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 32, 32, "Select Team:");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 32, 48, "LEFT: Red  RIGHT: Blue");
    } else {
        laser_tag_view_draw(laser_tag_view_get_view(app->view), canvas);
    }
}

LaserTagApp* laser_tag_app_alloc() {
    LaserTagApp* app = malloc(sizeof(LaserTagApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    app->view = laser_tag_view_alloc();
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->ir_controller = infrared_controller_alloc();
    app->game_state = game_state_alloc();
    app->state = LaserTagStateTeamSelect;

    view_port_draw_callback_set(app->view_port, laser_tag_app_draw_callback, app);
    view_port_input_callback_set(app->view_port, laser_tag_app_input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->timer = furi_timer_alloc(laser_tag_app_timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(app->timer, furi_kernel_get_tick_frequency());

    return app;
}

void laser_tag_app_free(LaserTagApp* app) {
    furi_assert(app);

    furi_timer_free(app->timer);
    view_port_enabled_set(app->view_port, false);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    laser_tag_view_free(app->view);
    furi_message_queue_free(app->event_queue);
    infrared_controller_free(app->ir_controller);
    game_state_free(app->game_state);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    free(app);
}

void laser_tag_app_fire(LaserTagApp* app) {
    infrared_controller_send(app->ir_controller);
    game_state_decrease_ammo(app->game_state, 1);
    notification_message(app->notifications, &sequence_blink_blue_100);
}

void laser_tag_app_handle_hit(LaserTagApp* app) {
    game_state_decrease_health(app->game_state, 10);
    notification_message(app->notifications, &sequence_vibro_1);
}

void laser_tag_app_enter_game_state(LaserTagApp* app) {
    app->state = LaserTagStateGame;
    game_state_reset(app->game_state);
    laser_tag_view_update(app->view, app->game_state);
}

int32_t laser_tag_app(void* p) {
    UNUSED(p);
    LaserTagApp* app = laser_tag_app_alloc();

    InputEvent event;
    bool running = true;
    while(running) {
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, 100);
        if(status == FuriStatusOk) {
            if(event.type == InputTypePress) {
                if(app->state == LaserTagStateTeamSelect) {
                    switch(event.key) {
                    case InputKeyLeft:
                        infrared_controller_set_team(app->ir_controller, TeamRed);
                        game_state_set_team(app->game_state, TeamRed);
                        laser_tag_app_enter_game_state(app);
                        break;
                    case InputKeyRight:
                        infrared_controller_set_team(app->ir_controller, TeamBlue);
                        game_state_set_team(app->game_state, TeamBlue);
                        laser_tag_app_enter_game_state(app);
                        break;
                    default:
                        break;
                    }
                } else {
                    switch(event.key) {
                    case InputKeyBack:
                        running = false;
                        break;
                    case InputKeyOk:
                        laser_tag_app_fire(app);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        if(app->state == LaserTagStateGame) {
            if(infrared_controller_receive(app->ir_controller)) {
                laser_tag_app_handle_hit(app);
            }

            if(game_state_is_game_over(app->game_state)) {
                notification_message(app->notifications, &sequence_error);
                running = false;
            }

            laser_tag_view_update(app->view, app->game_state);
        }

        view_port_update(app->view_port);
    }

    laser_tag_app_free(app);
    return 0;
}
