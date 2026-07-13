#include "app/App.hpp"
#include "terminal/Terminal.hpp"

int main() {
  tp::Terminal term;
  if (!term.ok()) {
    return 1;
  }

  tp::App app(term);
  return app.run();
}
