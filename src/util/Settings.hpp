#pragma once

#include "game/Types.hpp"
#include "input/Input.hpp"

#include <string>

namespace tp {

struct Settings {
  InputConfig input{};
  GameConfig game{};

  static Settings load_or_create();

  static Settings parse(const std::string& text);

  [[nodiscard]] std::string serialize() const;
  bool save() const;

  [[nodiscard]] static std::string default_rc_text();
};

}  // namespace tp
