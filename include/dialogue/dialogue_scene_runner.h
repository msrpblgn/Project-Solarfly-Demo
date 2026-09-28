#ifndef DIALOGUE_SCENE_RUNNER_H
#define DIALOGUE_SCENE_RUNNER_H

#include "battle_ui/battle_message_queue.h"
#include "dialogue/dialogue_host.h"
#include "dialogue/dialogue_scene.h"

/*
 * Public conversation lifecycle (Milestone 6).
 * Returns 1 on accepted start, 0 if busy or invalid (no lock / no callback).
 *
 * Dialogue_Update controls (shared by overworld + Dialogue Demo):
 *   aPressed    — A edge: advance SAY / resolve printer WAIT/PAGE.
 *                 Callers may OR JustPressed(B) so B advances at the prompt.
 *   bHeld       — B held: reveal remaining printables on the current line.
 *   startCancel — START: cancel/exit (unchanged); B never cancels the scene.
 */
int Dialogue_Start(
    const DialogueScene *scene,
    const DialogueHost *host,
    TextSpeed baseSpeed);
void Dialogue_Update(int aPressed, int bHeld, int startCancel);
int Dialogue_IsActive(void);
void Dialogue_Cancel(void);

/* Consumes and clears the pending result (exactly once per accepted run). */
DialogueResult Dialogue_TakeResult(void);

void Dialogue_SetNextMessageSpeed(TextSpeed speed);
TextSpeed Dialogue_GetNextMessageSpeed(void);

#endif
