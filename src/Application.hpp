#ifndef SESAMO_APPLICATION_HPP
#define SESAMO_APPLICATION_HPP

#include <optional>
#include <memory>

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

private:
    constexpr static auto WINDOW_WIDTH = 1280;
    constexpr static auto WINDOW_HEIGHT = 720;
    constexpr static const char* GLSL_VERSION = "#version 330";

private:
    GLFWwindow* window = nullptr;
    std::optional<SerialChannel> serial = std::nullopt;
};

#endif // SESAMO_APPLICATION_HPP
