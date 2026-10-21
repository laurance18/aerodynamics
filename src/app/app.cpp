#include "app/app.hpp"

#include "app/platform_win32.hpp"
#include "core/dummy_flow.hpp"
#include "core/particles.hpp"
#include "render/gl.hpp"
#include "render/gl_debug.hpp"
#include "render/particle_renderer.hpp"
#include "render/shader_sources.hpp"
#include "render/trail_renderer.hpp"
#include "ui/demo_panel.hpp"
#include "ui/main_menu.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

namespace pfl {
namespace {

constexpr const char* k_windowTitle = "Potential Flow Lab — demo";
constexpr int k_windowWidth = 1280;
constexpr int k_windowHeight = 800;
constexpr std::size_t k_particleCount = 10'000;
constexpr double k_viewHalfHeight = 1.25; // world units visible above/below the origin
constexpr double k_maxFrameDt = 1.0 / 20.0;

constexpr const char* k_glRequiredTitle = "Potential Flow Lab - OpenGL 3.3 required";
constexpr const char* k_glRequiredText =
    "Potential Flow Lab could not create an OpenGL 3.3 window.\n\n"
    "This program needs a graphics driver that supports OpenGL 3.3 or newer. "
    "Please install the latest driver for your graphics card (NVIDIA, AMD or Intel).\n\n"
    "Virtual machines and Remote Desktop sessions often do not provide OpenGL 3.3; "
    "run the program directly on the computer instead.";

void onGlfwError(int code, const char* description) {
    logMessage("GLFW error " + std::to_string(code) + ": " + description);
}

class App {
public:
    App() = default;
    ~App() { shutdown(); }
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init();
    void run();

private:
    bool createWindow();
    bool initGraphics();
    void initImGui();
    void reloadShaders();
    void frame(double dt);
    void shutdown();

    bool m_glfwReady = false;
    bool m_imguiReady = false;
    GLFWwindow* m_window = nullptr;

    // GL resources; destroyed in shutdown() while the context is still current.
    std::unique_ptr<ParticleRenderer> m_particleRenderer;
    std::unique_ptr<TrailRenderer> m_trailRenderer;

    ParticleSystem m_particles{k_particleCount};
    DemoSettings m_settings;
    DemoPanel m_panel;
    bool m_trailsWereOn = true;
};

bool App::init() {
    glfwSetErrorCallback(onGlfwError);
    if (glfwInit() != GLFW_TRUE) {
        showFatalError(k_glRequiredTitle, std::string("GLFW could not initialise.\n\n") + k_glRequiredText);
        return false;
    }
    m_glfwReady = true;

    if (!createWindow() || !initGraphics()) return false;
    initImGui();
    return true;
}

bool App::createWindow() {
    int major = 3;
    int minor = 3;
#ifdef _DEBUG
    // Lets you test the "no OpenGL 3.3" message box without a VM: set PFL_FORCE_GL_FAIL=1.
    if (const char* force = std::getenv("PFL_FORCE_GL_FAIL"); force != nullptr && force[0] == '1') {
        major = 9;
        minor = 9;
    }
#endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    // Fit the default size on small screens.
    int width = k_windowWidth;
    int height = k_windowHeight;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        int x = 0, y = 0, workW = 0, workH = 0;
        glfwGetMonitorWorkarea(monitor, &x, &y, &workW, &workH);
        if (workW > 0 && workH > 0) {
            width = std::min(width, workW * 9 / 10);
            height = std::min(height, workH * 9 / 10);
        }
    }

    m_window = glfwCreateWindow(width, height, k_windowTitle, nullptr, nullptr);
    if (m_window == nullptr) {
        const char* reason = nullptr;
        glfwGetError(&reason);
        std::string details = k_glRequiredText;
        if (reason != nullptr) details += std::string("\n\nDetails: ") + reason;
        showFatalError(k_glRequiredTitle, details);
        return false;
    }
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
    return true;
}

bool App::initGraphics() {
    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0 || GLAD_VERSION_MAJOR(version) * 10 + GLAD_VERSION_MINOR(version) < 33) {
        showFatalError(k_glRequiredTitle, k_glRequiredText);
        return false;
    }
    logMessage(std::string("OpenGL ") + reinterpret_cast<const char*>(glGetString(GL_VERSION)) + " on " +
               reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    if (installGlDebugOutput()) logMessage("OpenGL debug output enabled");

    m_particleRenderer = std::make_unique<ParticleRenderer>();
    m_trailRenderer = std::make_unique<TrailRenderer>();
    std::string error;
    if (!m_particleRenderer->init(error) || !m_trailRenderer->init(error)) {
        showFatalError("Potential Flow Lab - shader error", error);
        return false;
    }
    return true;
}

void App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // write nothing next to the exe
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    const float scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    m_imguiReady = true;
}

void App::reloadShaders() {
    std::string error;
    const bool particlesOk = m_particleRenderer->reloadShaders(error);
    const bool trailsOk = m_trailRenderer->reloadShaders(error);
    if (particlesOk && trailsOk) {
        logMessage("Shaders reloaded");
    } else {
        logMessage("Shader reload failed; keeping the previous programs.\n" + error);
    }
}

void App::run() {
    double last = glfwGetTime();
    while (glfwWindowShouldClose(m_window) == GLFW_FALSE) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE) {
            glfwWaitEventsTimeout(0.1); // minimised: don't spin
            last = glfwGetTime();
            continue;
        }
        const double now = glfwGetTime();
        frame(std::min(now - last, k_maxFrameDt));
        last = now;
        glfwSwapBuffers(m_window);
    }
}

void App::frame(double dt) {
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    if (fbWidth <= 0 || fbHeight <= 0) return;

    // --- UI -------------------------------------------------------------------------------
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const MainMenuActions actions = drawMainMenu(shaderHotReloadAvailable());
    if (actions.quit) glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    if (actions.reloadShaders) reloadShaders();
    m_panel.draw(m_settings, m_particles.particles().size());
    ImGui::Render();

    // --- Simulation -----------------------------------------------------------------------
    const double aspect = static_cast<double>(fbWidth) / fbHeight;
    const Vec2 halfExtent{k_viewHalfHeight * aspect, k_viewHalfHeight};
    m_particles.setDomain({-halfExtent * 1.05, halfExtent * 1.05}); // respawn just off-screen
    const DemoFlowParams flow = m_settings.flow;
    m_particles.step(dt, [&flow](Vec2 p) { return demoVelocity(p, flow); });

    // --- Rendering ------------------------------------------------------------------------
    m_particleRenderer->upload(m_particles.particles());
    ParticleView view;
    view.halfExtent = halfExtent;
    view.speedScale = static_cast<float>(1.0 / std::max(demoMaxSpeed(flow), 1e-6));
    view.pointSize = m_settings.pointSize * ImGui::GetStyle().FontScaleDpi;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_settings.trails) {
        if (!m_trailsWereOn) m_trailRenderer->clear();
        m_trailRenderer->begin(fbWidth, fbHeight, m_settings.trailFade);
        m_particleRenderer->draw(view);
        m_trailRenderer->composite(fbWidth, fbHeight);
    } else {
        m_particleRenderer->draw(view);
    }
    m_trailsWereOn = m_settings.trails;

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::shutdown() {
    m_trailRenderer.reset();
    m_particleRenderer.reset();
    if (m_imguiReady) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        m_imguiReady = false;
    }
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_glfwReady) {
        glfwTerminate();
        m_glfwReady = false;
    }
}

} // namespace

int runApp() {
    App app;
    if (!app.init()) return 1;
    app.run();
    return 0;
}

} // namespace pfl
