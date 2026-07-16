#pragma once

#include "input/Input.hpp"
#include "terminal/Keys.hpp"
#include "util/Settings.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

// Editable rows in the settings screen (order = display order).
// Visual gaps are inserted by the renderer before LinesPerLevel and ClearScores.
enum class SettingsItem : std::uint8_t {
  MoveInterval = 0,
  SoftDropInterval,
  ReleaseMs,
  KeyboardProtocol,
  LockDelay,
  LinesPerLevel,
  NextCount,
  Randomizer,
  PlayMode,
  FreakColors,
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
  KeyScores,
  ClearScores,
  Save,
  Reset,
  Back,
  Count,
};

inline constexpr int kSettingsItemCount = static_cast<int>(SettingsItem::Count);

[[nodiscard]] inline bool settings_gap_before(SettingsItem item) {
  return item == SettingsItem::LinesPerLevel || item == SettingsItem::KeyLeft ||
         item == SettingsItem::ClearScores;
}

// Visual row of an item's content line (0-based), including gap rows above it.
[[nodiscard]] inline int settings_item_visual_row(int item_idx) {
  int gaps = 0;
  for (int i = 0; i <= item_idx; ++i) {
    if (settings_gap_before(static_cast<SettingsItem>(i))) {
      ++gaps;
    }
  }
  return item_idx + gaps;
}

[[nodiscard]] inline int settings_list_rows() {
  return settings_item_visual_row(kSettingsItemCount - 1) + 1;
}

// Must match Renderer::draw_settings header/footer layout (list starts at row 4).
[[nodiscard]] inline int settings_viewport_rows(int term_rows) {
  constexpr int kListTopRow = 4;
  constexpr int kFooterRows = 3;
  return std::max(1, term_rows - kListTopRow - kFooterRows);
}

struct SettingsMenuView {
  const Settings& draft;
  int selected = 0;
  int scroll = 0;  // first visible visual row (not item index)
  bool capturing = false;
  bool confirming_clear = false;
  bool dirty = false;
  std::string_view status{};
  std::string_view clear_buf{};
};

// Pure menu state: browse / adjust / rebind / save commands.
class SettingsMenu {
 public:
  enum class Result : std::uint8_t { None, Saved, Back, ClearedScores };

  void open(const Settings& current);
  void on_key(const KeyEvent& ev);
  [[nodiscard]] Result take_result();

  // Keep selection on-screen for the given viewport height (visual rows).
  void ensure_visible(int viewport_rows);

  [[nodiscard]] const Settings& draft() const { return draft_; }
  [[nodiscard]] SettingsMenuView view() const;
  [[nodiscard]] bool dirty() const { return dirty_; }

 private:
  void move_sel(int delta);
  void adjust(int delta);
  void activate();
  void capture(const KeyEvent& ev);
  void capture_clear(const KeyEvent& ev);
  void reset_defaults();
  void mark_dirty();
  [[nodiscard]] static bool is_timing(SettingsItem item);
  [[nodiscard]] static bool is_keybind(SettingsItem item);
  [[nodiscard]] std::vector<KeySpec>* key_list(SettingsItem item);

  Settings draft_{};
  int selected_ = 0;
  int scroll_ = 0;  // visual row offset
  bool capturing_ = false;
  bool confirming_clear_ = false;
  bool dirty_ = false;
  Result result_ = Result::None;
  std::string status_{};
  std::string clear_buf_{};
};

}  // namespace tp
