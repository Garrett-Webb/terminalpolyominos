#pragma once

#include "input/Input.hpp"
#include "terminal/Keys.hpp"
#include "util/Settings.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

// Editable rows in the settings screen (order = display order).
enum class SettingsItem : std::uint8_t {
  MoveInterval = 0,
  SoftDropInterval,
  ReleaseMs,
  LockDelay,
  LinesPerLevel,
  NextCount,
  Randomizer,
  KeyLeft,
  KeyRight,
  KeySoftDrop,
  KeyHardDrop,
  KeySonicDrop,
  KeyRotateCw,
  KeyRotateCcw,
  KeyHold,
  KeyPause,
  KeyQuit,
  KeyRestart,
  KeySettings,
  Save,
  Reset,
  Back,
  Count,
};

inline constexpr int kSettingsItemCount = static_cast<int>(SettingsItem::Count);

struct SettingsMenuView {
  const Settings& draft;
  int selected = 0;
  int scroll = 0;
  bool capturing = false;
  bool dirty = false;
  std::string_view status{};
};

// Pure menu state: browse / adjust / rebind / save commands.
class SettingsMenu {
 public:
  enum class Result : std::uint8_t { None, Saved, Back };

  void open(const Settings& current);
  void on_key(const KeyEvent& ev);
  [[nodiscard]] Result take_result();

  [[nodiscard]] const Settings& draft() const { return draft_; }
  [[nodiscard]] SettingsMenuView view() const;
  [[nodiscard]] bool dirty() const { return dirty_; }

 private:
  void move_sel(int delta);
  void adjust(int delta);
  void activate();
  void capture(const KeyEvent& ev);
  void reset_defaults();
  void mark_dirty();
  [[nodiscard]] static bool is_timing(SettingsItem item);
  [[nodiscard]] static bool is_keybind(SettingsItem item);
  [[nodiscard]] std::vector<KeySpec>* key_list(SettingsItem item);

  Settings draft_{};
  int selected_ = 0;
  int scroll_ = 0;
  bool capturing_ = false;
  bool dirty_ = false;
  Result result_ = Result::None;
  std::string status_{};
};

}  // namespace tp
