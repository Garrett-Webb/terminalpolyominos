#pragma once

#include "game/Types.hpp"
#include "terminal/Keys.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

enum class KeyboardProtocol : std::uint8_t {
  Auto = 0,  // detect Kitty protocol; fall back to legacy
  Kitty,     // prefer Kitty press/release; fall back if unsupported
  Legacy,    // never query / push - OS-repeat model
};

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
  // Auto-repeat rate while a direction is held (after DAS / OS-repeat arms ARR).
  int move_interval_ms = 35;
  int soft_drop_interval_ms = 35;
  // Legacy: silence before treating a direction as released.
  // Enhanced (Kitty): delay after the first step before ARR starts (DAS).
  int release_ms = 90;
  KeyboardProtocol keyboard_protocol = KeyboardProtocol::Auto;
  Keybinds keys{};
};

// Movement uses a gravity-style timer. Instant actions (hard drop, rotate, …)
// fire immediately and are never swallowed by hold state.
class Input {
 public:
  explicit Input(InputConfig config = {});

  void set_config(InputConfig config) { config_ = config; }
  void reset();

  // When true: press/release sticks (Kitty protocol). When false: legacy silence model.
  void set_key_up_aware(bool on);
  [[nodiscard]] bool key_up_aware() const { return key_up_aware_; }

  void on_key(const KeyEvent& ev);
  void tick(int elapsed_ms);

  [[nodiscard]] std::vector<Action> drain();

  [[nodiscard]] std::optional<Action> to_action(const KeyEvent& ev) const;

 private:
  enum class Dir : std::uint8_t { None = 0, Left, Right, SoftDrop };

  struct Stick {
    bool held = false;
    bool repeating = false;  // ARR active (legacy: after OS key-repeat; enhanced: after DAS)
    int held_ms = 0;         // time since press (enhanced DAS)
    int since_event_ms = 0;  // silence / keep-alive watchdog
    int step_acc_ms = 0;     // ARR accumulator - only advanced while repeating
  };

  void clear_dirs();
  void press_dir(Dir dir);
  void release_dir(Dir dir);
  void repeat_dir(Dir dir);
  void tick_dir(Stick& stick, Dir dir, int elapsed_ms, int interval_ms);
  void queue(Action action);
  [[nodiscard]] static Action action_for(Dir dir);
  [[nodiscard]] Stick* stick_for(Dir dir);

  InputConfig config_;
  bool key_up_aware_ = false;
  Stick left_{};
  Stick right_{};
  Stick soft_{};
  std::vector<Action> out_;
};

[[nodiscard]] const char* keyboard_protocol_token(KeyboardProtocol p);
[[nodiscard]] KeyboardProtocol parse_keyboard_protocol(std::string_view s);

}  // namespace tp
