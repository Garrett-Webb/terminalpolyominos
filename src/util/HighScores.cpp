#include "util/HighScores.hpp"

#include "util/Xdg.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

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
  char* end = nullptr;
  const auto v = std::strtoll(std::string(s).c_str(), &end, 10);
  if (end == nullptr || *end != '\0') {
    return false;
  }
  out = static_cast<std::int64_t>(v);
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
  std::string fields[6];
  int n = 0;
  std::size_t i = 0;
  while (i < line.size() && n < 6) {
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
  if (n != 6) {
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
  return e;
}

}  // namespace

std::size_t HighScores::index_of(Randomizer r) {
  const auto i = static_cast<std::size_t>(r);
  if (i >= 6) {
    return 0;
  }
  return i;
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

Randomizer HighScores::cycle(Randomizer r, int delta) {
  constexpr int n = 6;
  int cur = static_cast<int>(r);
  cur = (cur + delta) % n;
  if (cur < 0) {
    cur += n;
  }
  return static_cast<Randomizer>(cur);
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

const std::vector<ScoreEntry>& HighScores::board(Randomizer r) const {
  return boards_[index_of(r)];
}

std::vector<ScoreEntry>& HighScores::board_mut(Randomizer r) {
  return boards_[index_of(r)];
}

void HighScores::sort_board(Randomizer r) {
  auto& b = board_mut(r);
  std::stable_sort(b.begin(), b.end(), entry_better);
}

void HighScores::truncate_board(Randomizer r) {
  auto& b = board_mut(r);
  if (static_cast<int>(b.size()) > kHighScoreBoardMax) {
    b.resize(static_cast<std::size_t>(kHighScoreBoardMax));
  }
}

std::optional<int> HighScores::consider(Randomizer r, ScoreEntry entry) {
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
  auto& b = board_mut(r);
  if (static_cast<int>(b.size()) >= kHighScoreBoardMax) {
    if (!entry_better(entry, b.back())) {
      return std::nullopt;
    }
  }
  b.push_back(std::move(entry));
  sort_board(r);
  truncate_board(r);

  const auto& sorted = board(r);
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    const auto& e = sorted[i];
    if (e.score == key.score && e.lines == key.lines && e.unix_time == key.unix_time &&
        e.name == key.name && e.level == key.level && e.lines_per_level == key.lines_per_level) {
      return static_cast<int>(i) + 1;
    }
  }
  return sorted.empty() ? std::nullopt : std::optional<int>{1};
}

bool HighScores::set_name(Randomizer r, int rank_1based, std::string_view name) {
  if (rank_1based < 1) {
    return false;
  }
  auto& b = board_mut(r);
  const std::size_t i = static_cast<std::size_t>(rank_1based - 1);
  if (i >= b.size()) {
    return false;
  }
  b[i].name = sanitize_name(name, true);
  return true;
}

bool HighScores::remove(Randomizer r, int rank_1based) {
  if (rank_1based < 1) {
    return false;
  }
  auto& b = board_mut(r);
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
  std::optional<Randomizer> section;
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
      section = parse_token(tok);
      continue;
    }
    if (!section.has_value()) {
      continue;
    }
    if (auto e = parse_entry_line(line)) {
      hs.board_mut(*section).push_back(std::move(*e));
    }
  }
  for (int i = 0; i < 6; ++i) {
    const auto r = static_cast<Randomizer>(i);
    hs.sort_board(r);
    hs.truncate_board(r);
  }
  return hs;
}

std::string HighScores::serialize() const {
  std::ostringstream out;
  out << "# terminalpolyominos high scores — one board per randomizer (top "
      << kHighScoreBoardMax << " each)\n"
      << "# Fields: score level lines lines_per_level name unix_time\n"
      << "# (randomizer = section name)\n";
  for (int i = 0; i < 6; ++i) {
    const auto r = static_cast<Randomizer>(i);
    out << '\n' << '[' << token(r) << "]\n";
    for (const auto& e : board(r)) {
      out << e.score << ' ' << e.level << ' ' << e.lines << ' ' << e.lines_per_level << ' '
          << sanitize_name(e.name, true) << ' ' << e.unix_time << '\n';
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
