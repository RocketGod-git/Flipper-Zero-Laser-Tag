#include "infrared_controller.h"
#include <furi.h>
#include <furi_hal.h>
#include <infrared.h>
#include <infrared_worker.h>
#include <stdlib.h>

#define TAG "InfraredController"

struct InfraredController {
    LaserTagTeam team;
    InfraredWorker* worker;
    FuriThread* rx_thread;
    volatile bool rx_running;
    volatile bool hit_received;
    FuriMutex* mutex;
};

static void infrared_rx_callback(void* context, InfraredWorkerSignal* received_signal) {
    FURI_LOG_D(TAG, "RX callback triggered");
    furi_assert(context);
    furi_assert(received_signal);

    InfraredController* controller = (InfraredController*)context;
    furi_mutex_acquire(controller->mutex, FuriWaitForever);

    FURI_LOG_D(TAG, "Context and received signal validated");

    const InfraredMessage* message = infrared_worker_get_decoded_signal(received_signal);
    if(message != NULL) {
        uint32_t received_command = message->address;
        FURI_LOG_D(TAG, "Received command: 0x%lx", (unsigned long)received_command);

        if((controller->team == TeamRed && received_command == IR_COMMAND_BLUE_TEAM) ||
           (controller->team == TeamBlue && received_command == IR_COMMAND_RED_TEAM)) {
            controller->hit_received = true;
            FURI_LOG_D(
                TAG, "Hit detected for team: %s", controller->team == TeamRed ? "Red" : "Blue");
        }
    } else {
        FURI_LOG_E(TAG, "Received NULL message");
    }

    furi_mutex_release(controller->mutex);
}

static int32_t infrared_rx_thread(void* context) {
    FURI_LOG_D(TAG, "RX thread started");
    furi_assert(context);

    InfraredController* controller = (InfraredController*)context;

    while(controller->rx_running) {
        furi_mutex_acquire(controller->mutex, FuriWaitForever);
        FURI_LOG_D(TAG, "Starting infrared_worker_rx_start");

        // Check for worker validity before starting
        if(controller->worker) {
            infrared_worker_rx_start(controller->worker);
            FURI_LOG_D(TAG, "infrared_worker_rx_start succeeded");
        } else {
            FURI_LOG_E(TAG, "InfraredWorker is NULL");
            furi_mutex_release(controller->mutex);
            continue;
        }

        furi_mutex_release(controller->mutex);

        FURI_LOG_D(TAG, "Waiting for thread flags");
        FuriStatus status = furi_thread_flags_wait(0, FuriFlagWaitAny, 10);

        if(status == FuriStatusErrorTimeout) {
            FURI_LOG_D(TAG, "RX loop timeout, continuing");
        } else {
            FURI_LOG_D(TAG, "RX loop received flag: %d", status);
        }
    }

    FURI_LOG_D(TAG, "RX thread stopping");
    return 0;
}

InfraredController* infrared_controller_alloc() {
    FURI_LOG_D(TAG, "Allocating InfraredController");

    InfraredController* controller = malloc(sizeof(InfraredController));
    if(!controller) {
        FURI_LOG_E(TAG, "Failed to allocate InfraredController struct");
        return NULL;
    }
    FURI_LOG_D(TAG, "InfraredController struct allocated");

    controller->team = TeamRed;
    FURI_LOG_D(TAG, "Team initialized to Red");

    FURI_LOG_D(TAG, "Allocating InfraredWorker");
    controller->worker = infrared_worker_alloc();
    if(!controller->worker) {
        FURI_LOG_E(TAG, "Failed to allocate InfraredWorker");
        free(controller);
        return NULL;
    }
    FURI_LOG_D(TAG, "InfraredWorker allocated");

    controller->rx_running = true;
    controller->hit_received = false;

    FURI_LOG_D(TAG, "Creating mutex");
    controller->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!controller->mutex) {
        FURI_LOG_E(TAG, "Failed to create mutex");
        infrared_worker_free(controller->worker);
        free(controller);
        return NULL;
    }

    FURI_LOG_D(TAG, "Setting up RX callback");
    infrared_worker_rx_set_received_signal_callback(
        controller->worker, infrared_rx_callback, controller);

    FURI_LOG_D(TAG, "Allocating RX thread");
    controller->rx_thread = furi_thread_alloc();
    if(!controller->rx_thread) {
        FURI_LOG_E(TAG, "Failed to allocate RX thread");
        furi_mutex_free(controller->mutex);
        infrared_worker_free(controller->worker);
        free(controller);
        return NULL;
    }

    furi_thread_set_name(controller->rx_thread, "IR_Rx_Thread");
    furi_thread_set_stack_size(controller->rx_thread, 1024);
    furi_thread_set_context(controller->rx_thread, controller);
    furi_thread_set_callback(controller->rx_thread, infrared_rx_thread);

    FURI_LOG_D(TAG, "Starting RX thread");
    furi_thread_start(controller->rx_thread);

    FURI_LOG_D(TAG, "Starting InfraredWorker RX");
    infrared_worker_rx_start(controller->worker);

    FURI_LOG_D(TAG, "InfraredController allocated successfully");
    return controller;
}

void infrared_controller_free(InfraredController* controller) {
    FURI_LOG_D(TAG, "Freeing InfraredController");
    furi_assert(controller);

    controller->rx_running = false;
    FURI_LOG_D(TAG, "Stopping RX thread");
    furi_thread_join(controller->rx_thread);
    furi_thread_free(controller->rx_thread);

    FURI_LOG_D(TAG, "Stopping InfraredWorker RX");
    furi_mutex_acquire(controller->mutex, FuriWaitForever);
    infrared_worker_rx_stop(controller->worker);
    infrared_worker_free(controller->worker);
    furi_mutex_release(controller->mutex);

    furi_mutex_free(controller->mutex);
    free(controller);
    FURI_LOG_D(TAG, "InfraredController freed");
}

void infrared_controller_set_team(InfraredController* controller, LaserTagTeam team) {
    furi_assert(controller);
    furi_mutex_acquire(controller->mutex, FuriWaitForever);
    controller->team = team;
    furi_mutex_release(controller->mutex);
    FURI_LOG_D(TAG, "Team set to %s", (team == TeamRed) ? "Red" : "Blue");
}

void infrared_controller_send(InfraredController* controller) {
    FURI_LOG_D(TAG, "Sending IR signal");
    furi_assert(controller);

    furi_mutex_acquire(controller->mutex, FuriWaitForever);

    uint32_t command = (controller->team == TeamRed) ? IR_COMMAND_RED_TEAM : IR_COMMAND_BLUE_TEAM;
    InfraredMessage message = {
        .protocol = InfraredProtocolNEC, .address = 0x00, .command = command, .repeat = false};

    infrared_worker_set_decoded_signal(controller->worker, &message);

    FURI_LOG_D(TAG, "Starting IR transmission");
    infrared_worker_tx_set_get_signal_callback(
        controller->worker, infrared_worker_tx_get_signal_steady_callback, NULL);

    infrared_worker_tx_start(controller->worker);

    furi_delay_ms(250);

    infrared_worker_tx_stop(controller->worker);
    FURI_LOG_D(TAG, "IR signal sent");

    furi_mutex_release(controller->mutex);
}

bool infrared_controller_receive(InfraredController* controller) {
    furi_assert(controller);
    furi_mutex_acquire(controller->mutex, FuriWaitForever);
    bool hit = controller->hit_received;
    controller->hit_received = false;
    furi_mutex_release(controller->mutex);
    FURI_LOG_D(TAG, "Checking for hit: %s", hit ? "Hit received" : "No hit");
    return hit;
}
