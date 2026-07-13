#include "util/Xdg.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace tp {
namespace {

std::string home_dir() {
  if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0') {
    return home;
  }
  return ".";
}

}  // namespace

std::string xdg_config_home() {
  if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0') {
    return xdg;
  }
  return home_dir() + "/.config";
}

std::string xdg_data_home() {
  if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && xdg[0] != '\0') {
    return xdg;
  }
  return home_dir() + "/.local/share";
}

std::string tpoly_config_dir() { return xdg_config_home() + "/tpoly"; }

std::string tpoly_data_dir() { return xdg_data_home() + "/tpoly"; }

std::string tpoly_rc_path() { return tpoly_config_dir() + "/.tpolyrc"; }

std::string tpoly_rc_legacy_path() { return home_dir() + "/.tpolyrc"; }

bool ensure_dir(const std::string& path) {
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return !ec;
}

}  // namespace tp
