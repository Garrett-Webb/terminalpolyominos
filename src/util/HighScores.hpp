#pragma once

#include "game/Types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tp {

inline constexpr int kHighScoreNameMax = 8;
inline constexpr int kHighScoreBoardMax = 10;

struct ScoreEntry {
  int score = 0;
  int level = 1;
  int lines = 0;
  int lines_per_level = kLinesPerLevel;
  std::string name = "---";
  std::int64_t unix_time = 0;
};

// Per-randomizer top-10 boards in one sectioned plain-text file (~/.local/share/tpoly/scores).
class HighScores {
 public:
  [[nodiscard]] const std::vector<ScoreEntry>& board(Randomizer r) const;
  [[nodiscard]] std::vector<ScoreEntry>& board_mut(Randomizer r);

  // Insert into the given board; returns 1-based rank if kept, else nullopt.
  // Score 0 never qualifies. Truncates to kHighScoreBoardMax.
  std::optional<int> consider(Randomizer r, ScoreEntry entry);

  // Update name for an existing 1-based rank (no-op if out of range).
  bool set_name(Randomizer r, int rank_1based, std::string_view name);

  // Remove a 1-based rank (for discarding a pending high score). Returns false if invalid.
  bool remove(Randomizer r, int rank_1based);

  void clear_all();

  [[nodiscard]] static HighScores parse(const std::string& text);
  [[nodiscard]] std::string serialize() const;

  bool load();
  bool save() const;

  // Sanitize player / $USER name: keep [A-Za-z0-9_-], max 8. Empty → "---" if for_storage.
  [[nodiscard]] static std::string sanitize_name(std::string_view raw, bool empty_as_dashes = true);
  [[nodiscard]] static std::string default_name_from_env();

  [[nodiscard]] static const char* token(Randomizer r);
  [[nodiscard]] static std::optional<Randomizer> parse_token(std::string_view t);
  [[nodiscard]] static Randomizer cycle(Randomizer r, int delta);

 private:
  void sort_board(Randomizer r);
  void truncate_board(Randomizer r);

  std::vector<ScoreEntry> boards_[static_cast<std::size_t>(6)]{};
  [[nodiscard]] static std::size_t index_of(Randomizer r);
};

}  // namespace tp
