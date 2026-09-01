#include "launcher.h"
#include "paths.h"

#include <gtk/gtk.h>
#include <limits.h>
#include <pango/pango.h>
#include <stdio.h>
#include <string.h>

#ifndef TP_VERSION
#define TP_VERSION "0.0.0"
#endif

typedef struct {
  VteTerminal* term;
  char game_path[PATH_MAX];
} AppState;

static void set_monospace_font(VteTerminal* term) {
  PangoFontDescription* font =
      pango_font_description_from_string("Monospace 11");
  if (font != NULL) {
    vte_terminal_set_font(term, font);
    pango_font_description_free(font);
  }
}

static void on_child_exited(VteTerminal* term, int status, gpointer user_data) {
  (void)term;
  (void)status;
  tp_gui_clear_child_pid();
  g_application_quit(G_APPLICATION(user_data));
}

static gboolean on_window_close_request(GtkWindow* window, gpointer user_data) {
  (void)window;
  (void)user_data;
  tp_gui_terminate_child();
  return FALSE;
}

static void on_activate(GtkApplication* app, gpointer user_data) {
  (void)user_data;

  AppState state;
  memset(&state, 0, sizeof(state));

  if (tp_gui_resolve_game_path(state.game_path, sizeof(state.game_path)) != 0) {
    g_printerr("terminalpolyominos-gui: could not find terminalpolyominos binary\n");
    return;
  }

  GtkWidget* window = gtk_application_window_new(app);
  char title[128];
  snprintf(title, sizeof(title), "terminalpolyominos %s", TP_VERSION);
  gtk_window_set_title(GTK_WINDOW(window), title);
  gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
  g_signal_connect(window, "close-request", G_CALLBACK(on_window_close_request), NULL);

  state.term = VTE_TERMINAL(vte_terminal_new());
  set_monospace_font(state.term);
  vte_terminal_set_mouse_autohide(state.term, TRUE);
  g_signal_connect(state.term, "child-exited", G_CALLBACK(on_child_exited), app);

  gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(state.term));

  gtk_window_present(GTK_WINDOW(window));

  if (tp_gui_launch_game(state.term, state.game_path) != 0) {
    g_printerr("terminalpolyominos-gui: failed to launch game\n");
  }
}

static void print_help(const char* argv0) {
  printf("Usage: %s [--help]\n\n", argv0);
  printf("Open terminalpolyominos in a GTK terminal window.\n");
  printf("Run terminalpolyominos directly in your terminal for tmux/zellij use.\n");
  printf("\nVersion %s\n", TP_VERSION);
}

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_help(argv[0]);
      return 0;
    }
    g_printerr("terminalpolyominos-gui: unknown option: %s\n", argv[i]);
    print_help(argv[0]);
    return 1;
  }

  GtkApplication* app =
      gtk_application_new("io.github.garrett_webb.terminalpolyominos.gui",
                          G_APPLICATION_NON_UNIQUE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  const int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
