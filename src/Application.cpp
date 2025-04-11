#include "Application.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <numeric>
#include <ranges>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

[[nodiscard]] static auto load_available_ttys(const auto& path)
{
    const auto iterator = std::filesystem::directory_iterator(path);

    const auto stringify = [](const auto& dir_entry) {
        return dir_entry.path().string();
    };

    const auto is_valid_tty = [](const auto& str) {
        const std::string prefix = "/dev/tty";
        if (!str.starts_with(prefix)) {
            return false;
        }

        const auto substr = str.substr(prefix.size());
        return std::ranges::find_if(substr, [](const char ch) { return !std::isdigit(ch); }) != substr.end();
    };

    auto ret = iterator
        | std::views::transform(stringify)
        | std::views::filter(is_valid_tty)
        | std::ranges::to<std::vector>();

    std::ranges::sort(ret);
    return ret;
}

static void glfw_error_callback(int error, const char* description)
{
    std::cerr << std::format("[ERROR] GLFW Error ({}): {}\n", error, description);
}

auto App::spawn() -> std::unique_ptr<App>
{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << std::format("[ERROR] Glfw failed initialize\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "sesamo", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << std::format("[ERROR] Glfw failed to create window\n");
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    // Load the font
    io.Fonts->AddFontFromFileTTF("resources/JetBrainsMono-Regular.ttf", 20.0f);
    io.Fonts->Build();

    return std::unique_ptr<App>(new App { window });
}

App::App(GLFWwindow* window)
    : window { window }
{
    available_ttys = load_available_ttys(TTY_PATH);
}

App::~App()
{
    if (serial->is_connected()) {
        serial->close();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::connect_to_serial()
{
    if (serial->is_connected()) return;

    const auto baud_rate = std::ranges::find_if(BAUD_RATES, [this](const auto& pair){ return pair.first == selected_baud_rate; });
    assert(baud_rate != BAUD_RATES.end());

    serial = SerialChannel::open(available_ttys[selected_tty], baud_rate->second);
    assert(serial);
}

void App::disconnect_from_serial()
{
    if (!serial->is_connected()) return;
    serial->close();
}

void App::clear_read_buffer()
{
    read_buffer.clear();
}

void App::handle_input()
{
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        connect_to_serial();
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        disconnect_from_serial();
    }

    if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        && glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        clear_read_buffer();
    }
}

void App::run()
{
    // TODO: cleanup all this mess
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            continue;
        }

        handle_input();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main window
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("sesamo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

            // Menu
            {
                render_control_buttons();
                ImGui::SameLine();
                render_tty_device_combo_box();
                ImGui::SameLine();
                render_baud_rate_combo_box();
                ImGui::SameLine();
                render_timestamp_checkbox();
                ImGui::SameLine();
                render_connection_status();
            }

            // Read Area
            render_read_area();

            ImGui::End();
            ImGui::PopStyleVar(1);
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void App::render_control_buttons()
{
    // Connect button
    {
        ImGui::BeginDisabled(serial->is_connected());
        if (ImGui::Button("Connect")) {
            connect_to_serial();
        }
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Disconnect button
    {
        ImGui::BeginDisabled(!serial->is_connected());
        if (ImGui::Button("Disconnect") && serial) {
            disconnect_from_serial();
        }
        ImGui::EndDisabled();
    }
}

void App::render_tty_device_combo_box()
{
    ImGui::Text("Select tty device: ");
    ImGui::SameLine();

    const auto max = std::ranges::max_element(available_ttys, [](const auto rhs, const auto& lhs) {
        return lhs.size() > rhs.size();
    });

    assert(max != available_ttys.end());
    ImGui::PushItemWidth(ImGui::CalcTextSize(max->c_str()).x + 20.0f);

    if (ImGui::BeginCombo("##SelectTty", available_ttys[selected_tty].c_str())) {
        for (size_t i = 0; i < available_ttys.size(); ++i) {
            const bool selected = selected_tty == i;
            if (ImGui::Selectable(available_ttys[i].c_str(), selected)) {
                selected_tty = i;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
}

void App::render_baud_rate_combo_box()
{
    ImGui::Text("Baud Rate: ");
    ImGui::SameLine();

    auto items = BAUD_RATES | std::views::transform([](const auto& pair) {
        return pair.first;
    }) | std::ranges::to<std::vector>();
    assert(items.size() > 0);

    ImGui::PushItemWidth(ImGui::CalcTextSize(items.back().data()).x + 35.0f);
        if (ImGui::BeginCombo("##SelectBaudRate", selected_baud_rate.data())) {
        for (size_t i = 0; i < items.size(); ++i) {
            const bool selected = selected_baud_rate == items[i];
            if (ImGui::Selectable(items[i].data(), selected)) {
                selected_baud_rate = items[i];
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
}

void App::render_timestamp_checkbox()
{
    ImGui::Checkbox("Show timestamps", &show_timestamps);
}

void App::render_read_area()
{
    ImGui::BeginChild("##ReadArea", ImVec2(READ_AREA_WIDTH, READ_AREA_HEIGHT));

    if (serial->is_connected() && serial->has_data_to_read()) {
        if (const auto message = serial->read(); message) {
            if (show_timestamps) {
                using namespace std::chrono;

                const auto now = system_clock::now();
                const auto time = floor<milliseconds>(now);
                const auto time_t_now = system_clock::to_time_t(time);
                const auto local_tm = *std::localtime(&time_t_now);
                const auto ms = duration_cast<milliseconds>(time.time_since_epoch()) % 1000;
                const auto timestamp = std::format("{:02}:{:02}:{:02}:{:03}",
                    local_tm.tm_hour,
                    local_tm.tm_min,
                    local_tm.tm_sec,
                    ms.count());

                auto messages_with_timestamp = *message | std::views::split('\n')
                        | std::views::transform([](const auto& elem) {
                            return std::string_view { elem };
                        }) | std::views::filter([](const auto& line) {
                            return !line.empty() && line != "\n";
                        }) | std::views::transform([&](const auto& line) {
                            return std::format("[{}]: {}\n", timestamp, line);
                        });

                read_buffer.insert(read_buffer.end(), messages_with_timestamp.begin(), messages_with_timestamp.end());
            } else {
                read_buffer.push_back(*message);
            }
        }
    }

    const auto result = std::accumulate(read_buffer.begin(), read_buffer.end(), std::string{});
    ImGui::Text("%s", result.c_str());

    ImGui::EndChild();
}

void App::render_connection_status()
{
    const char* text = serial->is_connected() ? "Connected" : "Disconnected";
    ImVec2 text_pos = ImGui::GetCursorScreenPos();
    ImVec2 text_size = ImGui::CalcTextSize(text);

    ImU32 bg_color = serial->is_connected() ? IM_COL32(0, 255, 0, 150) : IM_COL32(255, 0, 0, 150);
    ImGui::GetWindowDrawList()->AddRectFilled(
        text_pos,
        ImVec2(text_pos.x + text_size.x, (text_pos.y * 2) + text_size.y),
        bg_color
    );

    ImGui::TextUnformatted(text);
}
