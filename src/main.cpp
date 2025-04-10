#include <vector>
#include <iostream>
#include <numeric>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <termios.h>
#include <sys/ioctl.h>

#include "Serial.hpp"
#include "Application.hpp"

auto main() -> int
{
    auto app = App::spawn();
    if (!app) return 1;
    app->run();
}
