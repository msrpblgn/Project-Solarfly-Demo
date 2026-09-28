#ifndef DIALOGUE_HOST_H
#define DIALOGUE_HOST_H

#include "core/gba_types.h"

typedef enum DialogueResult {
    DIALOGUE_RESULT_NONE = 0,
    DIALOGUE_RESULT_COMPLETED,
    DIALOGUE_RESULT_CANCELLED,
    DIALOGUE_RESULT_ERROR
} DialogueResult;

typedef enum DialogueOverlayMode {
    DIALOGUE_OVERLAY_MODE_DEMO = 0,
    DIALOGUE_OVERLAY_MODE_FIELD = 1
} DialogueOverlayMode;

/*
 * Host adapter: gameplay lock, overlay mode, events, completion.
 * Runner never calls overworld movement or changes AppMode directly.
 */
typedef struct DialogueHost {
    DialogueOverlayMode overlayMode;
    void (*lock)(void *ctx);
    void (*unlock)(void *ctx);
    void (*on_event)(u16 eventId, void *ctx);
    void (*on_finished)(DialogueResult result, void *ctx);
    void *ctx;
} DialogueHost;

#endif
