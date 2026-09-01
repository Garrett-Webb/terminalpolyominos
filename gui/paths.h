#pragma once

#include <stddef.h>

// Resolve the terminalpolyominos game binary to execute in the PTY.
// Writes an absolute or PATH-resolvable path into |out| (NUL-terminated).
// Returns 0 on success, -1 if |out| is too small or no candidate is found.
int tp_gui_resolve_game_path(char* out, size_t out_size);
