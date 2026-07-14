#include "test_assert.hpp"
#include "util/HighScores.hpp"

#include <string>

namespace {

void test_sanitize_and_token() {
  TP_CHECK(tp::HighScores::sanitize_name("Alice!", true) == "Alice");
  TP_CHECK(tp::HighScores::sanitize_name("...", true) == "---");
  TP_CHECK(tp::HighScores::sanitize_name("toolongnamehere", true) == "toolongn");
  TP_CHECK(tp::HighScores::token(tp::Randomizer::Freak) == std::string("freak"));
  TP_CHECK(tp::HighScores::parse_token("7+1") == tp::Randomizer::SevenPlusOne);
  TP_CHECK(tp::HighScores::cycle_randomizer(tp::Randomizer::Freak, 1) == tp::Randomizer::SevenBag);
  TP_CHECK(tp::HighScores::cycle_randomizer(tp::Randomizer::SevenBag, -1) ==
           tp::Randomizer::Freak);
}

void test_section_roundtrip() {
  const char* text = R"(
# comment
[7bag]
100 2 10 10 alice 100
50 1 5 10 bob 90

[freak]
999 9 90 10 zed 200
bogus line
[unknown]
1 1 1 1 x 1
)";
  const tp::HighScores hs = tp::HighScores::parse(text);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless).size() == 2);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless)[0].score == 100);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless)[0].name == "alice");
  TP_CHECK(hs.board(tp::Randomizer::Freak, tp::PlayMode::Endless).size() == 1);
  TP_CHECK(hs.board(tp::Randomizer::Freak, tp::PlayMode::Endless)[0].score == 999);
  TP_CHECK(hs.board(tp::Randomizer::Torture, tp::PlayMode::Endless).empty());

  const tp::HighScores again = tp::HighScores::parse(hs.serialize());
  TP_CHECK(again.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless).size() == 2);
  TP_CHECK(again.board(tp::Randomizer::Freak, tp::PlayMode::Endless)[0].name == "zed");
}

void test_marathon_section() {
  const char* text = R"(
[7bag:marathon]
500 3 150 10 hero 1000 120000
)";
  const tp::HighScores hs = tp::HighScores::parse(text);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless).empty());
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Marathon).size() == 1);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Marathon)[0].score == 500);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Marathon)[0].elapsed_ms == 120000);

  const auto parsed = tp::HighScores::parse_section("7bag:marathon");
  TP_CHECK(parsed.has_value());
  TP_CHECK(parsed->first == tp::Randomizer::SevenBag);
  TP_CHECK(parsed->second == tp::PlayMode::Marathon);
  TP_CHECK(tp::HighScores::section_token(tp::Randomizer::SevenBag, tp::PlayMode::Marathon) ==
           "7bag:marathon");
  TP_CHECK(tp::HighScores::section_token(tp::Randomizer::SevenBag, tp::PlayMode::Endless) ==
           "7bag");
}

void test_consider_rank_and_truncate() {
  tp::HighScores hs;
  for (int i = 0; i < 5; ++i) {
    tp::ScoreEntry e;
    e.score = (i + 1) * 100;
    e.level = 1;
    e.lines = i;
    e.name = "p";
    e.unix_time = i;
    const auto rank = hs.consider(tp::Randomizer::Funk, tp::PlayMode::Endless, e);
    TP_CHECK(rank.has_value());
  }
  TP_CHECK(hs.board(tp::Randomizer::Funk, tp::PlayMode::Endless).size() == 5);
  TP_CHECK(hs.board(tp::Randomizer::Funk, tp::PlayMode::Endless).front().score == 500);
  TP_CHECK(hs.board(tp::Randomizer::Funk, tp::PlayMode::Endless).back().score == 100);

  tp::ScoreEntry low;
  low.score = 50;
  low.unix_time = 999;
  TP_CHECK(!hs.consider(tp::Randomizer::Funk, tp::PlayMode::Endless, low).has_value());
  TP_CHECK(hs.board(tp::Randomizer::Funk, tp::PlayMode::Endless).size() == 5);

  tp::ScoreEntry mid;
  mid.score = 350;
  mid.lines = 0;
  mid.unix_time = 1000;
  mid.name = "mid";
  const auto r = hs.consider(tp::Randomizer::Funk, tp::PlayMode::Endless, mid);
  TP_CHECK(r.has_value());
  TP_CHECK(*r >= 1 && *r <= 5);
  TP_CHECK(hs.board(tp::Randomizer::Funk, tp::PlayMode::Endless).size() == 5);
  TP_CHECK(hs.board(tp::Randomizer::Funk, tp::PlayMode::Endless).back().score >= 100);
}

void test_score_zero_rejected() {
  tp::HighScores hs;
  tp::ScoreEntry e;
  e.score = 0;
  e.unix_time = 1;
  TP_CHECK(!hs.consider(tp::Randomizer::SevenBag, tp::PlayMode::Endless, e).has_value());
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless).empty());
}

void test_boards_independent() {
  tp::HighScores hs;
  tp::ScoreEntry e;
  e.score = 500;
  e.unix_time = 1;
  e.name = "a";
  TP_CHECK(hs.consider(tp::Randomizer::Torture, tp::PlayMode::Sprint, e).has_value());
  TP_CHECK(hs.board(tp::Randomizer::Torture, tp::PlayMode::Sprint).size() == 1);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless).empty());
}

void test_set_name_and_clear() {
  tp::HighScores hs;
  tp::ScoreEntry e;
  e.score = 100;
  e.unix_time = 1;
  e.name = "tmp";
  const auto rank = hs.consider(tp::Randomizer::FullRandom, tp::PlayMode::Endless, e);
  TP_CHECK(rank == 1);
  TP_CHECK(hs.set_name(tp::Randomizer::FullRandom, tp::PlayMode::Endless, 1, "hero"));
  TP_CHECK(hs.board(tp::Randomizer::FullRandom, tp::PlayMode::Endless)[0].name == "hero");
  hs.clear_all();
  TP_CHECK(hs.board(tp::Randomizer::FullRandom, tp::PlayMode::Endless).empty());
}

void test_remove_pending() {
  tp::HighScores hs;
  tp::ScoreEntry keep;
  keep.score = 200;
  keep.unix_time = 1;
  keep.name = "keep";
  tp::ScoreEntry drop;
  drop.score = 100;
  drop.unix_time = 2;
  drop.name = "drop";
  TP_CHECK(hs.consider(tp::Randomizer::SevenBag, tp::PlayMode::Endless, keep) == 1);
  TP_CHECK(hs.consider(tp::Randomizer::SevenBag, tp::PlayMode::Endless, drop) == 2);
  TP_CHECK(hs.remove(tp::Randomizer::SevenBag, tp::PlayMode::Endless, 2));
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless).size() == 1);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless)[0].name == "keep");
  TP_CHECK(!hs.remove(tp::Randomizer::SevenBag, tp::PlayMode::Endless, 9));
}

void test_tie_break_newer_wins() {
  tp::HighScores hs;
  tp::ScoreEntry a;
  a.score = 100;
  a.lines = 10;
  a.unix_time = 10;
  a.name = "old";
  tp::ScoreEntry b = a;
  b.unix_time = 20;
  b.name = "new";
  (void)hs.consider(tp::Randomizer::SevenBag, tp::PlayMode::Endless, a);
  (void)hs.consider(tp::Randomizer::SevenBag, tp::PlayMode::Endless, b);
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless)[0].name == "new");
  TP_CHECK(hs.board(tp::Randomizer::SevenBag, tp::PlayMode::Endless)[1].name == "old");
}

}  // namespace

int main() {
  test_sanitize_and_token();
  test_section_roundtrip();
  test_marathon_section();
  test_consider_rank_and_truncate();
  test_score_zero_rejected();
  test_boards_independent();
  test_set_name_and_clear();
  test_remove_pending();
  test_tie_break_newer_wins();
  return tp::test::summary("tp_scores_tests");
}
