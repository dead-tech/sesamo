#ifndef SESAMO_APPLICATION_HPP
#define SESAMO_APPLICATION_HPP

#include <optional>
#include <memory>
#include <vector>

#include <GLFW/glfw3.h>

#include "Serial.hpp"

class [[nodiscard]] App final
{
public:
    [[nodiscard]] static auto spawn() -> std::unique_ptr<App>;

    void run();

    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&&) = default;
    App& operator=(App&&) = default;

private:
    explicit App(GLFWwindow* window);

    void handle_input();

    void render_menu_buttons();
    void render_menu_combo_box();

private:
    constexpr static auto WINDOW_WIDTH = 1280;
    constexpr static auto WINDOW_HEIGHT = 720;
    constexpr static const char* GLSL_VERSION = "#version 330";

    constexpr static auto TTY_PATH = "/dev/";

private:
    GLFWwindow* window = nullptr;
    std::optional<SerialChannel> serial = std::nullopt;
    std::vector<std::string> available_ttys = {};
    size_t selected_tty = 0;
};

#endif // SESAMO_APPLICATION_HPP
