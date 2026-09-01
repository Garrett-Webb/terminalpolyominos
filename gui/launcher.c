#include "launcher.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

enum {
  TP_GUI_COLS = 80,
  TP_GUI_ROWS = 26,
};

static GPid child_pid = -1;

static void on_child_setup(void* data) {
  (void)data;
  setenv("TERM", "xterm-256color", 1);
}

static void on_spawn_done(VteTerminal* term, GPid pid, GError* error, gpointer user_data) {
  (void)term;
  (void)user_data;
  if (error != NULL) {
    g_printerr("terminalpolyominos-gui: spawn failed: %s\n", error->message);
    g_error_free(error);
    return;
  }
  child_pid = pid;
}

void tp_gui_clear_child_pid(void) {
  child_pid = -1;
}

int tp_gui_launch_game(VteTerminal* term, const char* game_path) {
  if (term == NULL || game_path == NULL || game_path[0] == '\0') {
    return -1;
  }

  vte_terminal_set_size(term, TP_GUI_COLS, TP_GUI_ROWS);
  vte_terminal_set_scrollback_lines(term, 0);
  vte_terminal_set_scroll_on_output(term, FALSE);
  vte_terminal_set_scroll_on_keystroke(term, FALSE);

  char* argv[] = {(char*)game_path, NULL};

  vte_terminal_spawn_async(
      term,
      VTE_PTY_DEFAULT,
      NULL,
      argv,
      NULL,
      G_SPAWN_DEFAULT,
      on_child_setup,
      NULL,
      NULL,
      -1,
      NULL,
      on_spawn_done,
      NULL);

  return 0;
}

void tp_gui_terminate_child(void) {
  if (child_pid > 0) {
    kill((pid_t)child_pid, SIGTERM);
  }
}
