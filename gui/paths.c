#include "paths.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int try_path(char* out, size_t out_size, const char* candidate) {
  if (candidate == NULL || candidate[0] == '\0') {
    return -1;
  }
  if (snprintf(out, out_size, "%s", candidate) >= (int)out_size) {
    return -1;
  }
  return access(out, X_OK) == 0 ? 0 : -1;
}

static int try_join(char* out, size_t out_size, const char* dir, const char* name) {
  if (dir == NULL || name == NULL) {
    return -1;
  }
  if (snprintf(out, out_size, "%s/%s", dir, name) >= (int)out_size) {
    return -1;
  }
  return access(out, X_OK) == 0 ? 0 : -1;
}

static int try_path_env(char* out, size_t out_size) {
  const char* override = getenv("TPOLY_GAME");
  if (override != NULL && override[0] != '\0') {
    return try_path(out, out_size, override);
  }

  const char* path_env = getenv("PATH");
  if (path_env == NULL) {
    return -1;
  }

  char dir[PATH_MAX];
  const char* start = path_env;
  while (*start != '\0') {
    const char* end = start;
    while (*end != '\0' && *end != ':') {
      ++end;
    }

    const size_t len = (size_t)(end - start);
    if (len > 0 && len < sizeof(dir)) {
      memcpy(dir, start, len);
      dir[len] = '\0';
      if (try_join(out, out_size, dir, "terminalpolyominos") == 0) {
        return 0;
      }
    }

    start = (*end == ':') ? end + 1 : end;
  }

  return -1;
}

int tp_gui_resolve_game_path(char* out, size_t out_size) {
  if (out == NULL || out_size == 0) {
    return -1;
  }

  char self[PATH_MAX];
  const ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
  if (n > 0) {
    self[n] = '\0';
    char* slash = strrchr(self, '/');
    if (slash != NULL) {
      *slash = '\0';
      const char* bindir = self;

      // Installed / Flatpak: both binaries in the same directory.
      if (try_join(out, out_size, bindir, "terminalpolyominos") == 0) {
        return 0;
      }

      // CMake build: game in build-gui/, launcher in build-gui/gui/.
      if (try_join(out, out_size, bindir, "../terminalpolyominos") == 0) {
        return 0;
      }
    }
  }

  return try_path_env(out, out_size);
}
