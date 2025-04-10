#include "Application.hpp"

#include <iostream>
#include <format>
#include <vector>
#include <termios.h>
#include <numeric>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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

    return std::unique_ptr<App>(new App { window });
}

App::App(GLFWwindow* window)
    : window { window }
{
}

App::~App()
{
    if (serial) {
        serial->close();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::run()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::vector<std::string> buffer = {};
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("sesamo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);


            if (ImGui::Button("Connect")) {
                // FIXME: Proper error handling
                serial = SerialChannel::open("/dev/ttyACM0", B19200);
                assert(serial);
            }

            if (serial) {
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(serial->fd, &readfds);

                struct timeval timeout = {0, 0}; // 0 seconds, 0 microseconds = non-blocking
                int ready = select(serial->fd+ 1, &readfds, nullptr, nullptr, &timeout);

                if (ready > 0 && FD_ISSET(serial->fd, &readfds)) {
                    const auto message = serial->read();
                    if (message) {
                        buffer.push_back(*message);
                    }
                }

                const auto result = std::accumulate(buffer.begin(), buffer.end(), std::string{});
                ImGui::Text("%s", result.c_str());
            }

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
