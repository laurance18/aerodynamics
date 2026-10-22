#include "app/app.hpp"

#include "app/platform_win32.hpp"
#include "core/particles.hpp"
#include "core/scene.hpp"
#include "core/streamline.hpp"
#include "core/surface_pressure.hpp"
#include "io/data_export.hpp"
#include "io/examples.hpp"
#include "io/scene_io.hpp"
#include "render/gl.hpp"
#include "render/gl_debug.hpp"
#include "render/particle_renderer.hpp"
#include "render/shader_sources.hpp"
#include "render/trail_renderer.hpp"
#include "ui/main_menu.hpp"
#include "ui/parameter_dialog.hpp"
#include "ui/plot_view.hpp"
#include "ui/status_bar.hpp"
#include "ui/surface_panel.hpp"
#include "ui/style.hpp"
#include "ui/toolbar.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace pfl {
namespace {

constexpr const char* k_appName = "Potential Flow Lab";
constexpr int k_windowWidth = 1400;
constexpr int k_windowHeight = 860;
constexpr double k_maxFrameDt = 1.0 / 20.0;
constexpr float k_fontSize = 17.0f;

// Streamlines: traced a little beyond the view so panning shows lines at the edges.
constexpr double k_streamlineMargin = 0.05;
constexpr double k_singularRadiusPx = 3.0; // lines stop this many pixels from a singular point

// Contours: |V| (and Cp from it) is sampled on cells this many pixels wide; the |V| scale tops out
// at this multiple of the typical speed in the view, in this many bands.
constexpr float k_speedCellPx = 4.0f;
constexpr double k_speedScaleFactor = 2.0;
constexpr int k_speedBands = 10;

// Field CSV export: grid nodes across the view (rows follow from the aspect ratio).
constexpr int k_exportColumns = 201;

// Image export: frames drawn after the menu or file dialog closes, so neither is in the picture.
constexpr int k_captureDelayFrames = 3;
// Command-line exports: frames drawn first, so the plot layout and streamlines have settled.
constexpr int k_batchWarmupFrames = 6;

// Tracer particles.
constexpr std::size_t k_particleCount = 4'000;
constexpr double k_crossingSeconds = 6.0;  // at the typical speed a particle crosses the view this fast
constexpr double k_particleLifetime = 6.0; // seconds before a particle respawns
constexpr float k_trailFade = 0.08f;       // fraction of the trail removed per frame
constexpr float k_pointSize = 2.0f;        // pixels

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

// The particle layer holds premultiplied colour; ImGui's default blending would darken it.
void premultipliedBlend(const ImDrawList*, const ImDrawCmd*) {
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

// Where new elements are placed: the view centre, rounded so the dialog shows tidy numbers.
Vec2 newElementPosition(const Rect& view) {
    const Vec2 c = view.center();
    const double step = std::pow(10.0, std::floor(std::log10(std::max(view.size().y, 1e-12))) - 1.0);
    return {std::round(c.x / step) * step + 0.0, std::round(c.y / step) * step + 0.0};
}

// The Cp colour scale: from 1 (stagnation) down to the Cp of twice the typical speed in the view,
// rounded down to a band edge, so that it matches the |V| scale. At least -1, at most 24 bands.
struct PressureScale {
    double lowest = -3.0;
    int bands = 16;
};

PressureScale pressureScale(double typicalSpeed, double freeStreamSpeed) {
    const double raw = std::clamp(pressureCoefficient(k_speedScaleFactor * typicalSpeed, freeStreamSpeed), -23.0, -1.0);
    const double step = raw >= -5.0 ? 0.25 : raw >= -11.0 ? 0.5 : 1.0;
    const double lowest = std::floor(raw / step) * step;
    return {lowest, static_cast<int>(std::lround((1.0 - lowest) / step))};
}

// Actions that throw away the current scene, so they may need confirmation first.
enum class DiscardingAction { None, New, Open, Example, Quit };

class App {
public:
    App() = default;
    ~App() { shutdown(); }
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init();
    void run();
    int exitCode() const { return m_exitCode; }

private:
    bool createWindow();
    bool initGraphics();
    void initImGui();
    void reloadShaders();
    void frame(double dt);
    void shutdown();

    // Scene management.
    void handleMenu(const MainMenuActions& actions);
    void request(DiscardingAction action);
    void perform(DiscardingAction action);
    void openScene();
    void openExample(const ExampleScene& example);
    void saveScene(bool askForPath);
    void setScene(Scene scene, std::filesystem::path path, const ExampleScene* example = nullptr);
    void sceneEdited();
    void rebuildModel();
    void updateTitle();
    void onCloseRequested();

    // Exports.
    std::filesystem::path suggestedExportPath(const char* suffix) const;
    void exportImage();
    void exportField();
    void exportLines();
    void exportSurface();
    bool writePlotImage(const std::filesystem::path& path, std::string& error) const;
    bool writeFieldCsv(const std::filesystem::path& path, std::string& error) const;
    bool writeLinesCsv(const std::filesystem::path& path, std::string& error) const;
    bool writeSurfaceCsv(const std::filesystem::path& path, std::string& error) const;
    void captureIfDue();
    void runBatchStep();

    // Per-frame work after the UI.
    void updateStreamlines();
    void renderParticles(double dt);
    bool particlesVisible() const;
    StatusInfo statusInfo() const;

    bool m_glfwReady = false;
    bool m_imguiReady = false;
    GLFWwindow* m_window = nullptr;

    // GL resources; destroyed in shutdown() while the context is still current.
    std::unique_ptr<ParticleRenderer> m_particleRenderer;
    std::unique_ptr<TrailRenderer> m_trailRenderer;

    // The scene and what is derived from it.
    Scene m_scene;
    FlowModel m_model;
    std::vector<std::string> m_warnings;
    std::filesystem::path m_path;
    const ExampleScene* m_example = nullptr; // the built-in example shown, until the scene is saved
    bool m_dirty = false;
    std::string m_title;

    // UI state.
    PlotView m_plotView;
    SurfacePanel m_surfacePanel;
    std::optional<Vec2> m_surfaceMarker; // surface point picked in the Cp panel (last frame)
    ParameterDialog m_dialog;
    PlotViewResult m_plot; // plot geometry from the latest frame
    ImVec2 m_plotWindowPos{0.0f, 0.0f};  // the plot window (axes included), for image export
    ImVec2 m_plotWindowSize{0.0f, 0.0f};
    DiscardingAction m_pendingAction = DiscardingAction::None;
    std::size_t m_pendingExample = 0; // for DiscardingAction::Example
    bool m_confirmDiscard = false;
    bool m_quitConfirmed = false;

    // Streamlines, equipotentials, contours and body surfaces, recomputed when the flow, the view or
    // the plot settings change.
    StreamlineSet m_streamlines;
    StreamlineSet m_equipotentials;
    ContourMap m_contours;
    std::vector<BodySurface> m_surfaces;
    double m_lowestCp = -3.0; // bottom of the Cp colour scale
    std::optional<Rect> m_streamlineView;
    bool m_streamlinesDirty = true;

    // Tracer particles.
    ParticleSystem m_particles{k_particleCount};
    double m_referenceSpeed = 0.0;
    std::optional<Rect> m_particleView;
    bool m_respawnParticles = true;

    // A pending File > Export image: written once the frame no longer shows the menu or dialog.
    struct ImageCapture {
        std::filesystem::path path;
        int framesLeft = k_captureDelayFrames;
    };
    std::optional<ImageCapture> m_imageCapture;

    // Command-line exports (then quit), if any were requested.
    CommandLine m_commandLine;
    int m_batchFrames = 0;
    int m_exitCode = 0;
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
    setScene(Scene{}, {});

    m_commandLine = parseCommandLine();
    if (!m_commandLine.error.empty()) {
        showErrorMessage(m_window, "Command line", m_commandLine.error);
        if (m_commandLine.batch()) return false;
    }
    if (m_commandLine.scene) {
        SceneLoadResult result = loadSceneFile(*m_commandLine.scene);
        if (result.scene) {
            setScene(std::move(*result.scene), *m_commandLine.scene);
        } else {
            showErrorMessage(m_window, "Could not open scene", result.error);
            if (m_commandLine.batch()) return false;
        }
    }
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
    int workX = 0, workY = 0, workW = 0, workH = 0;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
        if (workW > 0 && workH > 0) {
            width = std::min(width, workW * 9 / 10);
            height = std::min(height, workH * 9 / 10);
        }
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // shown once centred
    m_window = glfwCreateWindow(width, height, k_appName, nullptr, nullptr);
    if (m_window == nullptr) {
        const char* reason = nullptr;
        glfwGetError(&reason);
        std::string details = k_glRequiredText;
        if (reason != nullptr) details += std::string("\n\nDetails: ") + reason;
        showFatalError(k_glRequiredTitle, details);
        return false;
    }
    if (workW > 0 && workH > 0) {
        // Centre the whole window, title bar included, in the work area (above the taskbar).
        int left = 0, top = 0, right = 0, bottom = 0;
        glfwGetWindowFrameSize(m_window, &left, &top, &right, &bottom);
        glfwSetWindowPos(m_window, workX + (workW - width - left - right) / 2 + left,
                         workY + std::max((workH - height - top - bottom) / 2, 0) + top);
    }
    glfwShowWindow(m_window);
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        static_cast<App*>(glfwGetWindowUserPointer(window))->onCloseRequested();
    });
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

    // Segoe UI ships with every Windows 10/11 and has the Greek letters and symbols the labels use
    // (Λ, Γ, μ, θ, °, ...). Without it ImGui's built-in font shows them as '?'.
    const char* windows = std::getenv("WINDIR");
    const std::filesystem::path font = std::filesystem::path(windows != nullptr ? windows : "C:\\Windows") / "Fonts" / "segoeui.ttf";
    std::error_code ec;
    if (std::filesystem::exists(font, ec)) {
        io.Fonts->AddFontFromFileTTF(font.string().c_str(), k_fontSize);
    } else {
        logMessage("Segoe UI not found; using ImGui's default font (Greek letters will not display).");
    }

    ImGui::StyleColorsDark();
    const float scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;

    // Double-click on the plot edits elements, so ImPlot's "fit on double-click" moves to the middle button.
    ImPlot::GetInputMap().Fit = ImGuiMouseButton_Middle;

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

    // Size the particle layer to the plot before the UI records draw commands that use its texture
    // (resizing recreates the texture). The plot size is last frame's; a change shows a frame later.
    const ImVec2 fbScale = ImGui::GetIO().DisplayFramebufferScale;
    const int layerWidth = static_cast<int>(m_plot.pixelSize.x * fbScale.x);
    const int layerHeight = static_cast<int>(m_plot.pixelSize.y * fbScale.y);
    if (particlesVisible() && layerWidth > 0 && layerHeight > 0) m_trailRenderer->resize(layerWidth, layerHeight);

    // --- UI -------------------------------------------------------------------------------
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    // While an image is about to be captured, act as if the mouse had left the window, so no
    // hover highlight or tooltip ends up in the picture.
    if (m_imageCapture || m_commandLine.batch()) ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    ImGui::NewFrame();

    const MainMenuActions actions = drawMainMenu(m_scene, m_scene.plot, shaderHotReloadAvailable());

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 workPos = viewport->WorkPos;
    const ImVec2 workSize = viewport->WorkSize;
    const float statusHeight = statusBarHeight();
    const float toolWidth = toolbarWidth();
    const float bodyHeight = std::max(workSize.y - statusHeight, 1.0f);

    if (const std::optional<ElementKind> kind = drawToolbar(workPos, ImVec2(toolWidth, bodyHeight))) {
        m_dialog.openNew(defaultElement(*kind, newElementPosition(m_scene.plot.view)));
    }

    PlotLayers layers{.streamlines = m_streamlines,
                      .equipotentials = m_equipotentials,
                      .contours = m_contours,
                      .surfaces = {.bodies = m_surfaces, .lowestCp = m_lowestCp, .marker = m_surfaceMarker},
                      .particles = {}};
    if (particlesVisible() && m_trailRenderer->texture() != 0) {
        layers.particles.texture = static_cast<ImTextureID>(m_trailRenderer->texture());
        layers.particles.beginBlend = premultipliedBlend;
    }
    const bool surfacePanel = m_scene.plot.showSurfacePressure;
    const float panelWidth = surfacePanel ? surfacePanelWidth(workSize.x - toolWidth) : 0.0f;
    m_plotWindowPos = ImVec2(workPos.x + toolWidth, workPos.y);
    m_plotWindowSize = ImVec2(std::max(workSize.x - toolWidth - panelWidth, 1.0f), bodyHeight);
    m_plot = m_plotView.draw(m_plotWindowPos, m_plotWindowSize, m_scene, m_model, layers);
    m_surfaceMarker.reset();
    if (surfacePanel) {
        m_surfaceMarker = m_surfacePanel.draw(ImVec2(m_plotWindowPos.x + m_plotWindowSize.x, workPos.y),
                                              ImVec2(panelWidth, bodyHeight), m_surfaces, m_model);
    }
    m_scene.plot.view = m_plot.limits; // panning and zooming are saved with the scene, but are not "edits"
    if (m_plot.editRequest) m_dialog.openEdit(*m_plot.editRequest, m_scene.elements[*m_plot.editRequest]);

    handleMenu(actions);

    if (const std::optional<ElementEdit> edit = m_dialog.draw(m_scene)) {
        m_scene = applyEdit(std::move(m_scene), *edit);
        sceneEdited();
    }

    if (m_confirmDiscard) {
        openDiscardChangesDialog();
        m_confirmDiscard = false;
    }
    if (const std::optional<bool> discard = drawDiscardChangesDialog()) {
        if (*discard) perform(m_pendingAction);
        m_pendingAction = DiscardingAction::None;
    }

    drawStatusBar(ImVec2(workPos.x, workPos.y + bodyHeight), ImVec2(workSize.x, statusHeight), statusInfo());
    ImGui::Render();

    // --- Flow -----------------------------------------------------------------------------
    updateStreamlines();
    renderParticles(dt);

    // --- Screen ---------------------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // The back buffer now holds this frame; exports read it before it is swapped.
    captureIfDue();
    runBatchStep();
}

void App::handleMenu(const MainMenuActions& actions) {
    if (actions.quit) request(DiscardingAction::Quit);
    if (actions.newScene) request(DiscardingAction::New);
    if (actions.open) request(DiscardingAction::Open);
    if (actions.example) {
        m_pendingExample = *actions.example;
        request(DiscardingAction::Example);
    }
    if (actions.save || actions.saveAs) saveScene(actions.saveAs);
    if (actions.exportImage) exportImage();
    if (actions.exportField) exportField();
    if (actions.exportLines) exportLines();
    if (actions.exportSurface) exportSurface();
    if (actions.reloadShaders) reloadShaders();
    if (actions.resetView) m_plotView.setView(PlotSettings{}.view);
    if (actions.plotChanged) {
        m_dirty = true;
        m_streamlinesDirty = true;
        updateTitle();
    }

    const ElementsMenuAction& elements = actions.elements;
    if (elements.edit) m_dialog.openEdit(*elements.edit, m_scene.elements[*elements.edit]);
    if (elements.remove) {
        m_scene.elements.erase(m_scene.elements.begin() + static_cast<std::ptrdiff_t>(*elements.remove));
        sceneEdited();
    }
    if ((actions.clearAll || elements.removeAll) && !m_scene.elements.empty()) {
        m_scene.elements.clear();
        sceneEdited();
    }
}

void App::request(DiscardingAction action) {
    if (m_dirty) {
        m_pendingAction = action;
        m_confirmDiscard = true;
    } else {
        perform(action);
    }
}

void App::perform(DiscardingAction action) {
    switch (action) {
    case DiscardingAction::New: setScene(Scene{}, {}); break;
    case DiscardingAction::Open: openScene(); break;
    case DiscardingAction::Example: openExample(exampleScenes()[m_pendingExample]); break;
    case DiscardingAction::Quit:
        m_quitConfirmed = true;
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        break;
    case DiscardingAction::None: break;
    }
}

void App::openScene() {
    const std::optional<std::filesystem::path> path = showOpenSceneDialog(m_window);
    if (!path) return;
    SceneLoadResult result = loadSceneFile(*path);
    if (!result.scene) {
        showErrorMessage(m_window, "Could not open scene", result.error);
        return;
    }
    setScene(std::move(*result.scene), *path);
}

// Loaded as an untitled scene, so that Save asks where to put it rather than overwriting anything.
void App::openExample(const ExampleScene& example) {
    SceneLoadResult result = loadExampleScene(example);
    if (!result.scene) {
        showErrorMessage(m_window, "Could not open example", result.error);
        return;
    }
    setScene(std::move(*result.scene), {}, &example);
}

void App::saveScene(bool askForPath) {
    std::filesystem::path path = m_path;
    if (askForPath || path.empty()) {
        const std::optional<std::filesystem::path> chosen =
            showSaveSceneDialog(m_window, !path.empty() ? path
                                          : m_example != nullptr ? std::filesystem::path(m_example->file)
                                                                 : std::filesystem::path("scene.json"));
        if (!chosen) return;
        path = *chosen;
    }
    std::string error;
    if (!saveSceneFile(m_scene, path, error)) {
        showErrorMessage(m_window, "Could not save scene", error);
        return;
    }
    m_path = path;
    m_example = nullptr;
    m_dirty = false;
    updateTitle();
}

void App::setScene(Scene scene, std::filesystem::path path, const ExampleScene* example) {
    m_scene = std::move(scene);
    m_path = std::move(path);
    m_example = example;
    m_dirty = false;
    m_plotView.setView(m_scene.plot.view);
    rebuildModel();
    updateTitle();
}

void App::sceneEdited() {
    m_dirty = true;
    rebuildModel();
    updateTitle();
}

void App::rebuildModel() {
    m_model = buildFlowModel(m_scene);
    m_warnings = sceneWarnings(m_scene, m_model);
    m_streamlinesDirty = true;
    m_respawnParticles = true;
    m_trailRenderer->clear();
}

void App::updateTitle() {
    const std::string name = !m_path.empty()        ? m_path.filename().string()
                             : m_example != nullptr ? std::string(m_example->title) + " (example)"
                                                    : std::string("Untitled");
    const std::string title = name + (m_dirty ? "*" : "") + " - " + k_appName;
    if (title != m_title) {
        m_title = title;
        glfwSetWindowTitle(m_window, m_title.c_str());
    }
}

void App::onCloseRequested() {
    if (m_quitConfirmed || !m_dirty) return;
    glfwSetWindowShouldClose(m_window, GLFW_FALSE);
    request(DiscardingAction::Quit);
}

void App::updateStreamlines() {
    const Rect view = m_plot.limits;
    if (m_plot.pixelSize.x <= 0.0f || m_plot.pixelSize.y <= 0.0f) return;
    if (!m_streamlinesDirty && m_streamlineView == view) return;

    m_referenceSpeed = typicalSpeed(m_model.field, view);
    m_streamlines = {};
    m_equipotentials = {};
    m_contours = {};
    m_surfaces.clear();
    if (!m_model.field.elements.empty()) {
        const double worldPerPixel = view.size().x / m_plot.pixelSize.x;
        StreamlineSettings settings;
        settings.bounds = view.expanded(k_streamlineMargin);
        settings.separation = view.size().y / std::max(m_scene.plot.streamlineDensity, 1);
        settings.singularRadius = k_singularRadiusPx * worldPerPixel;
        settings.arrowSpacing = m_scene.plot.arrowSpacingPx * uiScale() * worldPerPixel;
        m_streamlines = computeStreamlines(m_model.field, settings);

        if (m_scene.plot.showEquipotentials) {
            // Same spacing as the streamlines, so where the speed is typical the two families form
            // roughly square cells (a flow net).
            settings.orthogonal = true;
            settings.arrowSpacing = 0.0;
            m_equipotentials = computeStreamlines(m_model.field, settings);
        }
        const double freeStream = m_model.freeStreamSpeed;
        const PressureScale pressure = freeStream > 0.0 ? pressureScale(m_referenceSpeed, freeStream) : PressureScale{};
        m_lowestCp = pressure.lowest;
        m_contours.quantity = m_scene.plot.background;
        const bool speed = m_contours.quantity == Background::Speed;
        const bool cp = m_contours.quantity == Background::PressureCoefficient && freeStream > 0.0;
        if ((speed || cp) && m_referenceSpeed > 0.0) {
            const float cell = k_speedCellPx * uiScale();
            const int columns = static_cast<int>(std::ceil(m_plot.pixelSize.x / cell));
            const int rows = static_cast<int>(std::ceil(m_plot.pixelSize.y / cell));
            m_contours.values = sampleSpeed(m_model.field, view, columns, rows, settings.singularRadius);
            if (speed) {
                m_contours.high = k_speedScaleFactor * m_referenceSpeed;
                m_contours.bands = k_speedBands;
            } else {
                // Bernoulli: NaN outside the domain stays NaN, infinite speed gives Cp = -infinity.
                for (double& value : m_contours.values.values) value = pressureCoefficient(value, freeStream);
                m_contours.low = pressure.lowest;
                m_contours.high = 1.0;
                m_contours.bands = pressure.bands;
            }
        }
        if (m_scene.plot.showSurfacePressure) m_surfaces = findBodySurfaces(m_model, view);
    }
    m_streamlineView = view;
    m_streamlinesDirty = false;
}

bool App::particlesVisible() const { return m_scene.plot.showParticles && !m_model.field.elements.empty(); }

void App::renderParticles(double dt) {
    if (!particlesVisible() || m_trailRenderer->width() == 0) return;
    const Rect view = m_plot.limits;
    if (m_particleView != view) {
        m_particleView = view;
        m_trailRenderer->clear(); // old trails would be in the wrong place
    }

    // Time runs so that a particle at the typical speed crosses the view in k_crossingSeconds.
    const double timeScale = m_referenceSpeed > 0.0 ? view.size().x / (k_crossingSeconds * m_referenceSpeed) : 0.0;
    ParticleSettings& settings = m_particles.settings();
    settings.bounds = view;
    settings.maxStepLength = view.size().x / 300.0;
    settings.maxAge = k_particleLifetime * timeScale;

    const FlowField& field = m_model.field;
    const double guard = view.size().x / 150.0;
    const auto valid = [&field, guard](Vec2 p) { return field.inDomain(p) && !field.nearSingularity(p, guard); };
    if (m_respawnParticles) {
        m_particles.respawnAll(valid);
        m_respawnParticles = false;
    }
    if (timeScale > 0.0) {
        m_particles.step(dt * timeScale, [&field](Vec2 p) { return field.velocity(p); }, valid);
    }

    m_particleRenderer->upload(m_particles.particles());
    ParticleView particleView;
    particleView.center = view.center();
    particleView.halfExtent = view.size() * 0.5;
    particleView.speedScale = static_cast<float>(1.0 / std::max(2.0 * m_referenceSpeed, 1e-12));
    particleView.pointSize = k_pointSize * uiScale();

    m_trailRenderer->begin(m_scene.plot.particleTrails ? k_trailFade : 1.0f);
    m_particleRenderer->draw(particleView);
    m_trailRenderer->end();
}

std::filesystem::path App::suggestedExportPath(const char* suffix) const {
    if (m_path.empty()) {
        const std::filesystem::path name = m_example != nullptr ? std::filesystem::path(m_example->file).stem() : "flow";
        return std::filesystem::path(name).concat(suffix);
    }
    return m_path.parent_path() / std::filesystem::path(m_path.stem()).concat(suffix);
}

void App::exportImage() {
    const std::optional<std::filesystem::path> path = showSavePngDialog(m_window, suggestedExportPath(".png"));
    if (path) m_imageCapture = ImageCapture{.path = *path};
}

void App::exportField() {
    const std::optional<std::filesystem::path> path = showSaveCsvDialog(m_window, suggestedExportPath("_field.csv"));
    std::string error;
    if (path && !writeFieldCsv(*path, error)) showErrorMessage(m_window, "Could not export field data", error);
}

void App::exportLines() {
    const std::optional<std::filesystem::path> path = showSaveCsvDialog(m_window, suggestedExportPath("_lines.csv"));
    std::string error;
    if (path && !writeLinesCsv(*path, error)) showErrorMessage(m_window, "Could not export lines", error);
}

void App::exportSurface() {
    const std::optional<std::filesystem::path> path = showSaveCsvDialog(m_window, suggestedExportPath("_surface.csv"));
    std::string error;
    if (path && !writeSurfaceCsv(*path, error)) showErrorMessage(m_window, "Could not export surface Cp", error);
}

bool App::writePlotImage(const std::filesystem::path& path, std::string& error) const {
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const ImVec2 origin = ImGui::GetMainViewport()->Pos;
    const int left = std::clamp(static_cast<int>(std::lround((m_plotWindowPos.x - origin.x) * scale.x)), 0, fbWidth);
    const int top = std::clamp(static_cast<int>(std::lround((m_plotWindowPos.y - origin.y) * scale.y)), 0, fbHeight);
    const int width = std::min(static_cast<int>(std::lround(m_plotWindowSize.x * scale.x)), fbWidth - left);
    const int height = std::min(static_cast<int>(std::lround(m_plotWindowSize.y * scale.y)), fbHeight - top);
    if (width <= 0 || height <= 0) {
        error = "The plot is not visible.";
        return false;
    }

    // GL rows start at the bottom; PNG rows at the top.
    const std::size_t rowBytes = 4 * static_cast<std::size_t>(width);
    std::vector<std::uint8_t> pixels(rowBytes * static_cast<std::size_t>(height));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(left, fbHeight - top - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<std::uint8_t> flipped(pixels.size());
    for (int row = 0; row < height; ++row) {
        std::copy_n(pixels.begin() + static_cast<std::ptrdiff_t>(rowBytes * static_cast<std::size_t>(height - 1 - row)),
                    rowBytes, flipped.begin() + static_cast<std::ptrdiff_t>(rowBytes * static_cast<std::size_t>(row)));
    }
    return writePng(path, width, height, flipped.data(), error);
}

bool App::writeFieldCsv(const std::filesystem::path& path, std::string& error) const {
    const Rect view = m_plot.limits;
    const int rows = std::max(2, static_cast<int>(std::lround((k_exportColumns - 1) * view.size().y / view.size().x)) + 1);
    return writeTextFile(path, fieldCsv(m_model, view, k_exportColumns, rows), error);
}

bool App::writeLinesCsv(const std::filesystem::path& path, std::string& error) const {
    return writeTextFile(path, linesCsv(m_streamlines, m_equipotentials), error);
}

bool App::writeSurfaceCsv(const std::filesystem::path& path, std::string& error) const {
    if (!(m_model.freeStreamSpeed > 0.0)) {
        error = "Surface Cp needs a free stream: add a uniform flow.";
        return false;
    }
    const std::vector<BodySurface> bodies = findBodySurfaces(m_model, m_plot.limits);
    if (bodies.empty()) {
        error = "No body surface was found in the flow.";
        return false;
    }
    return writeTextFile(path, surfaceCsv(bodies, m_model), error);
}

void App::captureIfDue() {
    if (!m_imageCapture || --m_imageCapture->framesLeft > 0) return;
    std::string error;
    if (!writePlotImage(m_imageCapture->path, error)) showErrorMessage(m_window, "Could not export image", error);
    m_imageCapture.reset();
}

void App::runBatchStep() {
    if (!m_commandLine.batch() || ++m_batchFrames < k_batchWarmupFrames || m_streamlinesDirty) return;
    const auto run = [this](const std::optional<std::filesystem::path>& path, const char* what, auto write) {
        std::string error;
        if (!path || (this->*write)(*path, error)) return;
        logMessage(std::string("Could not export ") + what + " to " + path->string() + ": " + error);
        m_exitCode = 1;
    };
    run(m_commandLine.exportImage, "the image", &App::writePlotImage);
    run(m_commandLine.exportField, "field data", &App::writeFieldCsv);
    run(m_commandLine.exportLines, "lines", &App::writeLinesCsv);
    run(m_commandLine.exportSurface, "surface Cp", &App::writeSurfaceCsv);
    m_quitConfirmed = true;
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

StatusInfo App::statusInfo() const {
    StatusInfo info;
    info.streamlineCount = m_streamlines.lines.size();
    info.warnings = m_warnings;

    const FlowField& field = m_model.field;
    if (m_plot.mouse) {
        ProbeInfo probe;
        probe.position = *m_plot.mouse;
        probe.inDomain = field.inDomain(probe.position);
        if (probe.inDomain) {
            probe.velocity = field.velocity(probe.position);
            if (m_model.freeStreamSpeed > 0.0) {
                probe.pressureCoefficient = pressureCoefficient(length(probe.velocity), m_model.freeStreamSpeed);
            }
        }
        info.probe = probe;
    }

    for (std::size_t i = 0; i < m_scene.elements.size(); ++i) {
        const auto* wall = std::get_if<SymmetryWall>(&m_scene.elements[i]);
        if (wall == nullptr) continue;
        const WallCheck check = checkWall(field, *wall, m_plot.limits);
        if (check.samples == 0 || check.maxSpeed <= 0.0) continue;
        info.walls.push_back({i + 1, check.maxNormalSpeed / check.maxSpeed, check.samples});
    }
    return info;
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
    return app.exitCode();
}

} // namespace pfl
