#pragma once

#include "game/Types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tp {

inline constexpr int kHighScoreNameMax = 8;
inline constexpr int kHighScoreBoardMax = 5;
inline constexpr int kHighScoreBoardCount = 6 * kPlayModeCount;  // randomizer × mode

struct ScoreEntry {
  int score = 0;
  int level = 1;
  int lines = 0;
  int lines_per_level = kLinesPerLevel;
  std::string name = "---";
  std::int64_t unix_time = 0;
  int elapsed_ms = 0;  // play time; shown for Sprint / Finished runs
};

// Per (randomizer × play-mode) top-10 boards in one sectioned file.
class HighScores {
 public:
  [[nodiscard]] const std::vector<ScoreEntry>& board(Randomizer r, PlayMode mode) const;
  [[nodiscard]] std::vector<ScoreEntry>& board_mut(Randomizer r, PlayMode mode);

  // Insert into the given board; returns 1-based rank if kept, else nullopt.
  // Score 0 never qualifies. Truncates to kHighScoreBoardMax.
  std::optional<int> consider(Randomizer r, PlayMode mode, ScoreEntry entry);

  bool set_name(Randomizer r, PlayMode mode, int rank_1based, std::string_view name);
  bool remove(Randomizer r, PlayMode mode, int rank_1based);

  void clear_all();

  [[nodiscard]] static HighScores parse(const std::string& text);
  [[nodiscard]] std::string serialize() const;

  bool load();
  bool save() const;

  [[nodiscard]] static std::string sanitize_name(std::string_view raw, bool empty_as_dashes = true);
  [[nodiscard]] static std::string default_name_from_env();

  [[nodiscard]] static const char* token(Randomizer r);
  [[nodiscard]] static std::string section_token(Randomizer r, PlayMode mode);
  // Parses "7bag" or "7bag:marathon". Returns nullopt on unknown.
  [[nodiscard]] static std::optional<std::pair<Randomizer, PlayMode>> parse_section(
      std::string_view t);
  [[nodiscard]] static std::optional<Randomizer> parse_token(std::string_view t);
  [[nodiscard]] static std::optional<PlayMode> parse_mode_token(std::string_view t);

  // Flat helpers (storage index); UI cycles randomizers only - see cycle_randomizer.
  [[nodiscard]] static int flat_index(Randomizer r, PlayMode mode);
  [[nodiscard]] static Randomizer flat_randomizer(int flat);
  [[nodiscard]] static PlayMode flat_mode(int flat);
  [[nodiscard]] static Randomizer cycle_randomizer(Randomizer r, int delta);

 private:
  void sort_board(Randomizer r, PlayMode mode);
  void truncate_board(Randomizer r, PlayMode mode);

  std::vector<ScoreEntry> boards_[static_cast<std::size_t>(kHighScoreBoardCount)]{};
  [[nodiscard]] static std::size_t index_of(Randomizer r, PlayMode mode);
};

}  // namespace tp
