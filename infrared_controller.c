#include "infrared_controller.h"
#include <furi.h>
#include <furi_hal.h>
#include <infrared.h>
#include <infrared_worker.h>
#include <stdlib.h>

#define TAG "LaserTagInfrared"

#define IR_COMMAND_RED_TEAM 0xA1
#define IR_COMMAND_BLUE_TEAM 0xB2

struct InfraredController {
    LaserTagTeam team;
    InfraredWorker* worker;
    FuriThread* rx_thread;
    volatile bool rx_running;
    volatile bool hit_received;
};

static void infrared_rx_callback(void* context, InfraredWorkerSignal* received_signal) {
    InfraredController* controller = (InfraredController*)context;
    
    const InfraredMessage* message = infrared_worker_get_decoded_signal(received_signal);
    if (message != NULL) {
        uint32_t received_command = message->address;
        if((controller->team == TeamRed && received_command == IR_COMMAND_BLUE_TEAM) ||
           (controller->team == TeamBlue && received_command == IR_COMMAND_RED_TEAM)) {
            controller->hit_received = true;
        }
    }
}

static int32_t infrared_rx_thread(void* context) {
    InfraredController* controller = (InfraredController*)context;
    
    while(controller->rx_running) {
        infrared_worker_rx_start(controller->worker);
        furi_thread_flags_wait(0, FuriFlagWaitAny, 10);
    }

    return 0;
}

InfraredController* infrared_controller_alloc() {
    InfraredController* controller = malloc(sizeof(InfraredController));
    controller->team = TeamRed;
    controller->worker = infrared_worker_alloc();
    controller->rx_running = true;
    controller->hit_received = false;

    infrared_worker_rx_set_received_signal_callback(controller->worker, infrared_rx_callback, controller);

    controller->rx_thread = furi_thread_alloc();
    furi_thread_set_name(controller->rx_thread, "IR_Rx_Thread");
    furi_thread_set_stack_size(controller->rx_thread, 1024);
    furi_thread_set_context(controller->rx_thread, controller);
    furi_thread_set_callback(controller->rx_thread, infrared_rx_thread);
    furi_thread_start(controller->rx_thread);

    infrared_worker_rx_start(controller->worker);

    return controller;
}

void infrared_controller_free(InfraredController* controller) {
    furi_assert(controller);

    controller->rx_running = false;
    furi_thread_join(controller->rx_thread);
    furi_thread_free(controller->rx_thread);

    infrared_worker_rx_stop(controller->worker);
    infrared_worker_free(controller->worker);
    free(controller);
}

void infrared_controller_set_team(InfraredController* controller, LaserTagTeam team) {
    furi_assert(controller);
    controller->team = team;
}

void infrared_controller_send(InfraredController* controller) {
    furi_assert(controller);
    uint32_t command = (controller->team == TeamRed) ? IR_COMMAND_RED_TEAM : IR_COMMAND_BLUE_TEAM;
    InfraredMessage message = {
        .protocol = InfraredProtocolNEC,
        .address = 0x00,
        .command = command,
        .repeat = false
    };
    
    infrared_worker_set_decoded_signal(controller->worker, &message);
    
    infrared_worker_tx_set_get_signal_callback(
        controller->worker,
        infrared_worker_tx_get_signal_steady_callback,
        NULL);
        
    infrared_worker_tx_start(controller->worker);
    
    furi_delay_ms(250);  // Delay to ensure the signal is sent
    
    infrared_worker_tx_stop(controller->worker);
}

bool infrared_controller_receive(InfraredController* controller) {
    furi_assert(controller);
    bool hit = controller->hit_received;
    controller->hit_received = false;
    return hit;
}