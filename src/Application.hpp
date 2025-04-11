#ifndef SESAMO_APPLICATION_HPP
#define SESAMO_APPLICATION_HPP

#include <optional>
#include <memory>
#include <vector>

#include <termios.h>

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

    void connect_to_serial();
    void disconnect_from_serial();
    void clear_read_buffer();

    void handle_input();

    void render_control_buttons();
    void render_tty_device_combo_box();
    void render_baud_rate_combo_box();
    void render_timestamp_checkbox();
    void render_read_area();
    void render_connection_status();

private:
    constexpr static auto WINDOW_WIDTH = 1920;
    constexpr static auto WINDOW_HEIGHT = 1080;
    constexpr static auto READ_AREA_WIDTH = 1905;
    constexpr static auto READ_AREA_HEIGHT = 720;
    constexpr static const char* GLSL_VERSION = "#version 330";

    constexpr static auto TTY_PATH = "/dev/";

    inline static const std::vector<std::pair<std::string_view, int>> BAUD_RATES =  {
        { "0", B0 },
        { "50", B50 },
        { "75", B75 },
        { "110", B110 },
        { "134", B134 },
        { "150", B150 },
        { "200", B200 },
        { "300", B300 },
        { "600", B600 },
        { "1200", B1200 },
        { "1800", B1800 },
        { "2400", B2400 },
        { "4800", B4800 },
        { "9600", B9600 },
        { "19200", B19200 },
        { "38400", B38400 }
    };

private:
    GLFWwindow* window = nullptr;

    std::optional<SerialChannel> serial = std::nullopt;
    std::vector<std::string> read_buffer;

    std::vector<std::string> available_ttys = {};
    size_t selected_tty = 0;

    std::string_view selected_baud_rate = "19200";

    bool show_timestamps = true;
};

#endif // SESAMO_APPLICATION_HPP
