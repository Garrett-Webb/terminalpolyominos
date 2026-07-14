#pragma once

#include "game/Types.hpp"
#include "terminal/Keys.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

// One physical key: arrow/special (`Key::Left` …) or a printable char (`Key::Char`).
struct KeySpec {
  Key key = Key::None;
  char ch = 0;

  [[nodiscard]] bool matches(const KeyEvent& ev) const;
  [[nodiscard]] std::string token() const;
  [[nodiscard]] static std::optional<KeySpec> from_token(std::string_view t);
};

// Remappable controls. Defaults match the locked gameplay controls.
struct Keybinds {
  std::vector<KeySpec> left;
  std::vector<KeySpec> right;
  std::vector<KeySpec> soft_drop;
  std::vector<KeySpec> hard_drop;
  std::vector<KeySpec> sonic_drop;
  std::vector<KeySpec> rotate_cw;
  std::vector<KeySpec> rotate_ccw;
  std::vector<KeySpec> hold;
  std::vector<KeySpec> pause;
  std::vector<KeySpec> quit;
  std::vector<KeySpec> restart;
  std::vector<KeySpec> settings;
  std::vector<KeySpec> scores;

  Keybinds();  // built-in defaults

  [[nodiscard]] std::optional<Action> action_for(const KeyEvent& ev) const;
  [[nodiscard]] std::string format_list(const std::vector<KeySpec>& keys) const;
  // Replace `dest` and drop any overlapping tokens from other actions.
  bool set_list(std::vector<KeySpec>& dest, std::string_view csv);

 private:
  void remove_from_others(const std::vector<KeySpec>& claimed, const std::vector<KeySpec>* keep);
};

struct InputConfig {
  // While a direction is held (after OS key-repeat confirms), step this often.
  int move_interval_ms = 50;
  int soft_drop_interval_ms = 35;
  // No key-up in terminals — treat direction as released after this silence.
  int release_ms = 120;
  Keybinds keys{};
};

// Movement uses a gravity-style timer. Instant actions (hard drop, rotate, …)
// fire immediately and are never swallowed by hold state.
class Input {
 public:
  explicit Input(InputConfig config = {});

  void set_config(InputConfig config) { config_ = config; }
  void reset();

  void on_key(const KeyEvent& ev);
  void tick(int elapsed_ms);

  [[nodiscard]] std::vector<Action> drain();

  [[nodiscard]] std::optional<Action> to_action(const KeyEvent& ev) const;

 private:
  enum class Dir : std::uint8_t { None = 0, Left, Right, SoftDrop };

  struct Stick {
    bool held = false;
    bool repeating = false;  // true after terminal key-repeat confirms a hold
    int since_event_ms = 0;
    int step_acc_ms = 0;
  };

  void clear_dirs();
  void press_dir(Dir dir);
  void tick_dir(Stick& stick, Dir dir, int elapsed_ms, int interval_ms);
  void queue(Action action);
  [[nodiscard]] static Action action_for(Dir dir);

  InputConfig config_;
  Stick left_{};
  Stick right_{};
  Stick soft_{};
  std::vector<Action> out_;
};

}  // namespace tp
