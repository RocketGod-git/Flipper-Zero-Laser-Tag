#include "infrared_controller.h"
#include <furi.h>
#include <infrared_worker.h>
#include <infrared_signal.h>
#include <notification/notification_messages.h>

#define TAG "InfraredController"

const NotificationSequence sequence_hit = {
    &message_vibro_on,
    &message_note_d4,
    &message_delay_1000,
    &message_vibro_off,
    &message_sound_off,
    NULL,
};

struct InfraredController {
    LaserTagTeam team;
    InfraredWorker* worker;
    InfraredSignal* signal;
    NotificationApp* notification;
    bool hit_received;
    bool processing_signal;
};

static void infrared_rx_callback(void* context, InfraredWorkerSignal* received_signal) {
    FURI_LOG_I(TAG, "RX callback triggered");

    InfraredController* controller = (InfraredController*)context;
    if(controller->processing_signal) {
        FURI_LOG_W(TAG, "Already processing a signal, skipping callback");
        return;
    }

    controller->processing_signal = true;

    if(!received_signal) {
        FURI_LOG_E(TAG, "Received signal is NULL");
        controller->processing_signal = false;
        return;
    }

    const InfraredMessage* message = infrared_worker_get_decoded_signal(received_signal);
    FURI_LOG_I(TAG, "Received signal - signal address: %p", (void*)received_signal);

    if(message) {
        FURI_LOG_I(
            TAG,
            "Received message: protocol=%d, address=0x%lx, command=0x%lx",
            message->protocol,
            (unsigned long)message->address,
            (unsigned long)message->command);

        if((controller->team == TeamRed && message->command == IR_COMMAND_BLUE_TEAM) ||
           (controller->team == TeamBlue && message->command == IR_COMMAND_RED_TEAM)) {
            controller->hit_received = true;
            FURI_LOG_I(
                TAG, "Hit detected for team: %s", controller->team == TeamRed ? "Red" : "Blue");
            notification_message_block(controller->notification, &sequence_hit);
        }
    } else {
        FURI_LOG_W(TAG, "RX callback received NULL message");
    }

    FURI_LOG_I(TAG, "RX callback completed");
    controller->processing_signal = false;
}

InfraredController* infrared_controller_alloc() {
    FURI_LOG_I(TAG, "Allocating InfraredController");

    InfraredController* controller = malloc(sizeof(InfraredController));
    if(!controller) {
        FURI_LOG_E(TAG, "Failed to allocate InfraredController");
        return NULL;
    }

    controller->team = TeamRed;
    controller->worker = infrared_worker_alloc();
    controller->signal = infrared_signal_alloc();
    controller->notification = furi_record_open(RECORD_NOTIFICATION);
    controller->hit_received = false;
    controller->processing_signal = false;

    if(controller->worker && controller->signal && controller->notification) {
        FURI_LOG_I(
            TAG, "InfraredWorker, InfraredSignal, and NotificationApp allocated successfully");
    } else {
        FURI_LOG_E(TAG, "Failed to allocate resources");
        free(controller);
        return NULL;
    }

    FURI_LOG_I(TAG, "Setting up RX callback");
    infrared_worker_rx_set_received_signal_callback(
        controller->worker, infrared_rx_callback, controller);

    FURI_LOG_I(TAG, "InfraredController allocated successfully");
    return controller;
}

void infrared_controller_free(InfraredController* controller) {
    FURI_LOG_I(TAG, "Freeing InfraredController");

    if(controller) {
        FURI_LOG_I(TAG, "Freeing InfraredWorker and InfraredSignal");
        infrared_worker_free(controller->worker);
        infrared_signal_free(controller->signal);

        FURI_LOG_I(TAG, "Closing NotificationApp");
        furi_record_close(RECORD_NOTIFICATION);

        free(controller);

        FURI_LOG_I(TAG, "InfraredController freed successfully");
    } else {
        FURI_LOG_W(TAG, "Attempted to free NULL InfraredController");
    }
}

void infrared_controller_set_team(InfraredController* controller, LaserTagTeam team) {
    FURI_LOG_I(TAG, "Setting team to %s", team == TeamRed ? "Red" : "Blue");
    controller->team = team;
}

void infrared_controller_send(InfraredController* controller) {
    FURI_LOG_I(TAG, "Preparing to send infrared signal");

    InfraredMessage message = {
        .protocol = InfraredProtocolNEC,
        .address = 0x00,
        .command = (controller->team == TeamRed) ? IR_COMMAND_RED_TEAM : IR_COMMAND_BLUE_TEAM};

    FURI_LOG_I(
        TAG,
        "Prepared message: protocol=%d, address=0x%lx, command=0x%lx",
        message.protocol,
        (unsigned long)message.address,
        (unsigned long)message.command);

    FURI_LOG_I(TAG, "Setting message for infrared signal");
    infrared_signal_set_message(controller->signal, &message);

    FURI_LOG_I(TAG, "Starting infrared signal transmission");
    infrared_signal_transmit(controller->signal);

    FURI_LOG_I(TAG, "Infrared signal transmission completed");
}

bool infrared_controller_receive(InfraredController* controller) {
    FURI_LOG_I(TAG, "Starting infrared signal reception");

    if(controller->processing_signal) {
        FURI_LOG_W(TAG, "Cannot start reception, another signal is still being processed");
        return false;
    }

    infrared_worker_rx_start(controller->worker);

    furi_delay_ms(50);

    infrared_worker_rx_stop(controller->worker);

    bool hit = controller->hit_received;

    FURI_LOG_I(TAG, "Signal reception complete, hit received: %s", hit ? "true" : "false");

    controller->hit_received = false;

    return hit;
}
