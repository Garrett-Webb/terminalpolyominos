#pragma once

#include "game/Types.hpp"
#include "input/Input.hpp"

#include <string>

namespace tp {

// User settings persisted in ~/.config/tpoly/.tpolyrc (XDG).
struct Settings {
  InputConfig input{};
  GameConfig game{};

  // Load from disk (XDG .tpolyrc, then legacy ~/.tpolyrc). Missing file -> defaults
  // and write a fresh XDG rc with comments (like many terminal apps).
  static Settings load_or_create();

  // Parse rc text (for tests). Unknown keys ignored; bad values keep defaults.
  static Settings parse(const std::string& text);

  [[nodiscard]] std::string serialize() const;
  bool save() const;

  [[nodiscard]] static std::string default_rc_text();
};

}  // namespace tp
