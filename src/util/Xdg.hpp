#pragma once

#include <string>

namespace tp {

// XDG Base Directory helpers (with classic fallbacks).
// Config dir: $XDG_CONFIG_HOME/tpoly or ~/.config/tpoly
// Data dir:   $XDG_DATA_HOME/tpoly or ~/.local/share/tpoly

[[nodiscard]] std::string xdg_config_home();
[[nodiscard]] std::string xdg_data_home();
[[nodiscard]] std::string tpoly_config_dir();
[[nodiscard]] std::string tpoly_data_dir();
[[nodiscard]] std::string tpoly_rc_path();
[[nodiscard]] std::string tpoly_scores_path();
[[nodiscard]] std::string tpoly_rc_legacy_path();

bool ensure_dir(const std::string& path);

}  // namespace tp
