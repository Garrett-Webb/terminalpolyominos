#pragma once

#include <vte/vte.h>

// Spawn |game_path| inside |term| on a new PTY. Returns 0 on success.
int tp_gui_launch_game(VteTerminal* term, const char* game_path);

// Best-effort SIGTERM to the spawned game child (if any).
void tp_gui_terminate_child(void);

// Clear tracked child pid after VTE reports child-exited.
void tp_gui_clear_child_pid(void);
