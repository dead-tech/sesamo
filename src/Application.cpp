#include "Application.hpp"

#include <algorithm>
#include <filesystem>
#include <termios.h>
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

void App::handle_input()
{
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        serial = SerialChannel::open(available_ttys[selected_tty], B19200);
        assert(serial);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        serial->close();
    }
}

void App::run()
{
    // TODO: cleanup all this mess
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // TODO: hook up alaso keyboard bindings to quickly
    // connect/disconnect and this sort of things
    std::vector<std::string> buffer = {};
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

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("sesamo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

            render_menu_buttons();
            ImGui::SameLine();
            render_menu_combo_box();

            if (serial->is_connected() && serial->has_data_to_read()) {
                if (const auto message = serial->read(); message) {
                    buffer.push_back(*message);
                }
            }

            const auto result = std::accumulate(buffer.begin(), buffer.end(), std::string{});
            ImGui::Text("%s", result.c_str());

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

void App::render_menu_buttons()
{
    // FIXME: Proper error handling
    ImGui::BeginDisabled(serial->is_connected());
    if (ImGui::Button("Connect")) {
        serial = SerialChannel::open(available_ttys[selected_tty], B19200);
        assert(serial);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(!serial->is_connected());
    if (ImGui::Button("Disconnect") && serial) {
        serial->close();
    }
    ImGui::EndDisabled();
}

void App::render_menu_combo_box()
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
