#include "util/HighScores.hpp"

#include "util/Xdg.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace tp {
namespace {

std::string trim(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  return std::string(s);
}

bool entry_better(const ScoreEntry& a, const ScoreEntry& b) {
  if (a.score != b.score) {
    return a.score > b.score;
  }
  if (a.lines != b.lines) {
    return a.lines > b.lines;
  }
  return a.unix_time > b.unix_time;
}

bool parse_i64(std::string_view s, std::int64_t& out) {
  if (s.empty()) {
    return false;
  }
  std::int64_t v = 0;
  const char* first = s.data();
  const char* last = s.data() + s.size();
  const auto result = std::from_chars(first, last, v);
  if (result.ec != std::errc{} || result.ptr != last) {
    return false;
  }
  out = v;
  return true;
}

bool parse_int(std::string_view s, int& out) {
  std::int64_t v = 0;
  if (!parse_i64(s, v) || v < 0 || v > 2147483647LL) {
    return false;
  }
  out = static_cast<int>(v);
  return true;
}

std::optional<ScoreEntry> parse_entry_line(std::string_view line) {
  std::string fields[7];
  int n = 0;
  std::size_t i = 0;
  while (i < line.size() && n < 7) {
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
      ++i;
    }
    if (i >= line.size()) {
      break;
    }
    const std::size_t start = i;
    while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i]))) {
      ++i;
    }
    fields[n++] = std::string(line.substr(start, i - start));
  }
  if (n != 6 && n != 7) {
    return std::nullopt;
  }
  ScoreEntry e;
  if (!parse_int(fields[0], e.score) || !parse_int(fields[1], e.level) ||
      !parse_int(fields[2], e.lines) || !parse_int(fields[3], e.lines_per_level)) {
    return std::nullopt;
  }
  if (e.level < 1) {
    e.level = 1;
  }
  if (e.lines_per_level < 1) {
    e.lines_per_level = kLinesPerLevel;
  }
  e.name = HighScores::sanitize_name(fields[4], true);
  if (!parse_i64(fields[5], e.unix_time)) {
    return std::nullopt;
  }
  if (n == 7) {
    if (!parse_int(fields[6], e.elapsed_ms)) {
      return std::nullopt;
    }
  }
  return e;
}

}  // namespace

std::size_t HighScores::index_of(Randomizer r, PlayMode mode) {
  const auto ri = static_cast<std::size_t>(r);
  const auto mi = static_cast<std::size_t>(mode);
  if (ri >= 6) {
    return static_cast<std::size_t>(mode) * 6;
  }
  if (mi >= static_cast<std::size_t>(kPlayModeCount)) {
    return ri;
  }
  return mi * 6 + ri;
}

int HighScores::flat_index(Randomizer r, PlayMode mode) {
  return static_cast<int>(index_of(r, mode));
}

Randomizer HighScores::flat_randomizer(int flat) {
  const int i = ((flat % kHighScoreBoardCount) + kHighScoreBoardCount) % kHighScoreBoardCount;
  return static_cast<Randomizer>(i % 6);
}

PlayMode HighScores::flat_mode(int flat) {
  const int i = ((flat % kHighScoreBoardCount) + kHighScoreBoardCount) % kHighScoreBoardCount;
  return static_cast<PlayMode>(i / 6);
}

Randomizer HighScores::cycle_randomizer(Randomizer r, int delta) {
  constexpr int n = 6;
  int cur = static_cast<int>(r);
  cur = (cur + delta) % n;
  if (cur < 0) {
    cur += n;
  }
  return static_cast<Randomizer>(cur);
}

const char* HighScores::token(Randomizer r) {
  switch (r) {
    case Randomizer::SevenPlusOne:
      return "7+1";
    case Randomizer::FullRandom:
      return "random";
    case Randomizer::Torture:
      return "torture";
    case Randomizer::Funk:
      return "funk";
    case Randomizer::Freak:
      return "freak";
    case Randomizer::SevenBag:
      break;
  }
  return "7bag";
}

std::string HighScores::section_token(Randomizer r, PlayMode mode) {
  std::string t = token(r);
  if (mode != PlayMode::Endless) {
    t += ':';
    t += play_mode_token(mode);
  }
  return t;
}

std::optional<PlayMode> HighScores::parse_mode_token(std::string_view t) {
  std::string lower;
  lower.reserve(t.size());
  for (unsigned char c : t) {
    lower.push_back(static_cast<char>(std::tolower(c)));
  }
  if (lower == "endless" || lower.empty()) {
    return PlayMode::Endless;
  }
  if (lower == "marathon") {
    return PlayMode::Marathon;
  }
  if (lower == "sprint") {
    return PlayMode::Sprint;
  }
  return std::nullopt;
}

std::optional<Randomizer> HighScores::parse_token(std::string_view t) {
  std::string lower;
  lower.reserve(t.size());
  for (unsigned char c : t) {
    lower.push_back(static_cast<char>(std::tolower(c)));
  }
  if (lower == "7bag" || lower == "bag" || lower == "7-bag") {
    return Randomizer::SevenBag;
  }
  if (lower == "7+1" || lower == "7plus1" || lower == "sevenplusone") {
    return Randomizer::SevenPlusOne;
  }
  if (lower == "random" || lower == "full" || lower == "fullrandom") {
    return Randomizer::FullRandom;
  }
  if (lower == "torture") {
    return Randomizer::Torture;
  }
  if (lower == "funk") {
    return Randomizer::Funk;
  }
  if (lower == "freak") {
    return Randomizer::Freak;
  }
  return std::nullopt;
}

std::optional<std::pair<Randomizer, PlayMode>> HighScores::parse_section(std::string_view t) {
  const auto colon = t.find(':');
  if (colon == std::string_view::npos) {
    auto r = parse_token(t);
    if (!r) {
      return std::nullopt;
    }
    return std::make_pair(*r, PlayMode::Endless);
  }
  auto r = parse_token(t.substr(0, colon));
  auto m = parse_mode_token(t.substr(colon + 1));
  if (!r || !m) {
    return std::nullopt;
  }
  return std::make_pair(*r, *m);
}

std::string HighScores::sanitize_name(std::string_view raw, bool empty_as_dashes) {
  std::string out;
  out.reserve(kHighScoreNameMax);
  for (unsigned char c : raw) {
    if (out.size() >= static_cast<std::size_t>(kHighScoreNameMax)) {
      break;
    }
    if (std::isalnum(c) || c == '_' || c == '-') {
      out.push_back(static_cast<char>(c));
    }
  }
  if (out.empty() && empty_as_dashes) {
    return "---";
  }
  return out;
}

std::string HighScores::default_name_from_env() {
  const char* user = std::getenv("USER");
  if (user == nullptr || user[0] == '\0') {
    user = std::getenv("LOGNAME");
  }
  if (user == nullptr) {
    return {};
  }
  return sanitize_name(user, false);
}

const std::vector<ScoreEntry>& HighScores::board(Randomizer r, PlayMode mode) const {
  return boards_[index_of(r, mode)];
}

std::vector<ScoreEntry>& HighScores::board_mut(Randomizer r, PlayMode mode) {
  return boards_[index_of(r, mode)];
}

void HighScores::sort_board(Randomizer r, PlayMode mode) {
  auto& b = board_mut(r, mode);
  std::stable_sort(b.begin(), b.end(), entry_better);
}

void HighScores::truncate_board(Randomizer r, PlayMode mode) {
  auto& b = board_mut(r, mode);
  if (static_cast<int>(b.size()) > kHighScoreBoardMax) {
    b.resize(static_cast<std::size_t>(kHighScoreBoardMax));
  }
}

std::optional<int> HighScores::consider(Randomizer r, PlayMode mode, ScoreEntry entry) {
  if (entry.score <= 0) {
    return std::nullopt;
  }
  entry.name = sanitize_name(entry.name, true);
  if (entry.level < 1) {
    entry.level = 1;
  }
  if (entry.lines_per_level < 1) {
    entry.lines_per_level = kLinesPerLevel;
  }

  const ScoreEntry key = entry;
  auto& b = board_mut(r, mode);
  if (static_cast<int>(b.size()) >= kHighScoreBoardMax) {
    if (!entry_better(entry, b.back())) {
      return std::nullopt;
    }
  }
  b.push_back(std::move(entry));
  sort_board(r, mode);
  truncate_board(r, mode);

  const auto& sorted = board(r, mode);
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    const auto& e = sorted[i];
    if (e.score == key.score && e.lines == key.lines && e.unix_time == key.unix_time &&
        e.name == key.name && e.level == key.level && e.lines_per_level == key.lines_per_level &&
        e.elapsed_ms == key.elapsed_ms) {
      return static_cast<int>(i) + 1;
    }
  }
  return sorted.empty() ? std::nullopt : std::optional<int>{1};
}

bool HighScores::set_name(Randomizer r, PlayMode mode, int rank_1based, std::string_view name) {
  if (rank_1based < 1) {
    return false;
  }
  auto& b = board_mut(r, mode);
  const std::size_t i = static_cast<std::size_t>(rank_1based - 1);
  if (i >= b.size()) {
    return false;
  }
  b[i].name = sanitize_name(name, true);
  return true;
}

bool HighScores::remove(Randomizer r, PlayMode mode, int rank_1based) {
  if (rank_1based < 1) {
    return false;
  }
  auto& b = board_mut(r, mode);
  const std::size_t i = static_cast<std::size_t>(rank_1based - 1);
  if (i >= b.size()) {
    return false;
  }
  b.erase(b.begin() + static_cast<std::ptrdiff_t>(i));
  return true;
}

void HighScores::clear_all() {
  for (auto& b : boards_) {
    b.clear();
  }
}

HighScores HighScores::parse(const std::string& text) {
  HighScores hs;
  std::istringstream in(text);
  std::string line;
  std::optional<std::pair<Randomizer, PlayMode>> section;
  while (std::getline(in, line)) {
    const auto hash = line.find('#');
    if (hash != std::string::npos) {
      line.resize(hash);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']' && line.size() >= 3) {
      const auto tok = trim(std::string_view(line).substr(1, line.size() - 2));
      section = parse_section(tok);
      continue;
    }
    if (!section.has_value()) {
      continue;
    }
    if (auto e = parse_entry_line(line)) {
      hs.board_mut(section->first, section->second).push_back(std::move(*e));
    }
  }
  for (int m = 0; m < kPlayModeCount; ++m) {
    for (int i = 0; i < 6; ++i) {
      const auto r = static_cast<Randomizer>(i);
      const auto mode = static_cast<PlayMode>(m);
      hs.sort_board(r, mode);
      hs.truncate_board(r, mode);
    }
  }
  return hs;
}

std::string HighScores::serialize() const {
  std::ostringstream out;
  out << "# terminalpolyominos high scores — top " << kHighScoreBoardMax
      << " per randomizer × play mode\n"
      << "# Fields: score level lines lines_per_level name unix_time [elapsed_ms]\n"
      << "# Sections: 7bag | 7bag:marathon | 7bag:sprint | …\n";
  for (int m = 0; m < kPlayModeCount; ++m) {
    for (int i = 0; i < 6; ++i) {
      const auto r = static_cast<Randomizer>(i);
      const auto mode = static_cast<PlayMode>(m);
      out << '\n' << '[' << section_token(r, mode) << "]\n";
      for (const auto& e : board(r, mode)) {
        out << e.score << ' ' << e.level << ' ' << e.lines << ' ' << e.lines_per_level << ' '
            << sanitize_name(e.name, true) << ' ' << e.unix_time;
        if (e.elapsed_ms > 0) {
          out << ' ' << e.elapsed_ms;
        }
        out << '\n';
      }
    }
  }
  return out.str();
}

bool HighScores::load() {
  std::ifstream in(tpoly_scores_path());
  if (!in) {
    *this = HighScores{};
    return true;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  *this = parse(ss.str());
  return true;
}

bool HighScores::save() const {
  if (!ensure_dir(tpoly_data_dir())) {
    return false;
  }
  const std::string path = tpoly_scores_path();
  const std::string tmp = path + ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc);
    if (!out) {
      return false;
    }
    out << serialize();
    if (!out) {
      return false;
    }
  }
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}

}  // namespace tp
