#include "Application.hpp"

auto main() -> int
{
    auto app = App::spawn();
    if (!app) return 1;
    app->run();
}
