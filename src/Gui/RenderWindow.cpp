#include "Gui/RenderWindow.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Gui/Renderer.h"
#include "Point.h"
#include "Shaders.h"
#include "ShaderUtil.h"

Gui::RenderWindow::RenderWindow(const int width, const int height, std::string title, const RenderMode renderMode)
    : m_title(std::move(title)), m_width(width), m_height(height) {
    // Create a window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_window = glfwCreateWindow(this->m_width, this->m_height, this->m_title.c_str(),nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        std::terminate();
    }
    glfwMakeContextCurrent(m_window);
    
    // Initialize glew
    if (!Renderer::GetInstance()->m_glewInitialized) {
        Renderer::GetInstance()->initializeGlew();
    }
    glGenBuffers(1, &m_vertexBuffer);
    glViewport(0, 0, width, height);
    setRenderMode(renderMode);
    setMouseCallbacksForPanning();
    setKeyboardCallback();

    // Disable VSync
    glfwSwapInterval(0);
    
    m_projectionMatrix = glm::ortho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f);
}

Gui::RenderWindow::RenderWindow(RenderWindow&& other) noexcept
    : m_title(std::move(other.m_title)), m_width(other.m_width), m_height(other.m_height),
    m_window(other.m_window), m_visible(other.m_visible), m_isDraggingMouse(other.m_isDraggingMouse),
    m_panSettings(other.m_panSettings), m_zoomSettings(other.m_zoomSettings),
    m_renderMode(other.m_renderMode), m_shaderProgram(other.m_shaderProgram),
    m_vertexBuffer(other.m_vertexBuffer),
    m_vertexCount(other.m_vertexCount) {
    other.m_window = nullptr;
    other.m_shaderProgram = 0;
}

Gui::RenderWindow& Gui::RenderWindow::operator=(RenderWindow&& other) noexcept {
    if (this != &other) {
        glDeleteProgram(m_shaderProgram);
        glfwDestroyWindow(m_window);
        
        m_title = std::move(other.m_title);
        m_width = other.m_width;
        m_height = other.m_height;
        m_window = other.m_window;
        m_visible = other.m_visible;

        m_isDraggingMouse = other.m_isDraggingMouse;
        m_panSettings = other.m_panSettings;
        m_zoomSettings = other.m_zoomSettings;

        m_renderMode = other.m_renderMode;
        m_shaderProgram = other.m_shaderProgram;
        m_vertexBuffer = other.m_vertexBuffer;
        m_vertexCount = other.m_vertexCount;
        
        other.m_window = nullptr;
        other.m_shaderProgram = 0;
    }
    return *this;
}

Gui::RenderWindow::~RenderWindow() {
    glDeleteProgram(m_shaderProgram);
    glfwDestroyWindow(m_window);
}

void Gui::RenderWindow::setRenderMode(const RenderMode mode) {
    m_renderMode = mode;
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
    }
    switch (mode) {
    case RenderMode::Point:
        m_shaderProgram = Shaders::Util::CreateShader(Shaders::PointVertexShader, Shaders::PointGeometryShader, Shaders::PointFragmentShader);
        glUseProgram(m_shaderProgram);
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        m_render = &RenderWindow::renderPoints;

        m_mvpUniform = glGetUniformLocation(m_shaderProgram, "u_mvp");
    }
}

bool Gui::RenderWindow::isVisible() const {
    return m_visible;
}

std::future<bool> Gui::RenderWindow::windowShouldCloseAsync() const {
    auto promise = std::make_shared<std::promise<bool>>();

    if (Renderer::GetInstance()->onRenderThread()) {
        promise->set_value(windowShouldClose());
    } else {
        Renderer::GetInstance()->queueFunction([this, promise] {
            promise->set_value(windowShouldClose());
        });
    }

    return promise->get_future();
}

std::future<void> Gui::RenderWindow::hideWindowAsync() {
    auto promise = std::make_shared<std::promise<void>>();

    if (Renderer::GetInstance()->onRenderThread()) {
        hideWindow();
        promise->set_value();
    } else {
        Renderer::GetInstance()->queueFunction([this, promise] {
            hideWindow();
            promise->set_value();
        });
    }

    return promise->get_future();
}

std::future<void> Gui::RenderWindow::showWindowAsync() {
    auto promise = std::make_shared<std::promise<void>>();

    if (Renderer::GetInstance()->onRenderThread()) {
        showWindow();
        promise->set_value();
    } else {
        Renderer::GetInstance()->queueFunction([this, promise] {
            showWindow();
            promise->set_value();
        });
    }

    return promise->get_future();
}

std::future<void> Gui::RenderWindow::setBufferDataPointsAsync(const std::shared_ptr<std::vector<Point>>& points) {
    auto promise = std::make_shared<std::promise<void>>();

    if (Renderer::GetInstance()->onRenderThread()) {
        setBufferDataPoints(points);
        promise->set_value();
    } else {
        Renderer::GetInstance()->queueFunction([this, promise, points] {
            setBufferDataPoints(points);
            promise->set_value();
        });
    }

    return promise->get_future();
}

void Gui::RenderWindow::renderFrame() {
    updateMVP();
    (this->*m_render)();
}

void Gui::RenderWindow::makeCurrentContext() {
    glfwMakeContextCurrent(m_window);
    Renderer::GetInstance()->m_currentWindow = this;
}

bool Gui::RenderWindow::windowShouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Gui::RenderWindow::hideWindow() {
    m_visible = false;
    glfwHideWindow(m_window);
}

void Gui::RenderWindow::showWindow() {
    m_visible = true;
    glfwShowWindow(m_window);
}

static void WorldToScreen(const float worldX, const float worldY, float& screenX, float& screenY, const Gui::PanSettings& pan, const Gui::ZoomSettings& zoom) {
    screenX = worldX * zoom.xScale + pan.xOffset;
    screenY = worldY * zoom.yScale + pan.yOffset;
}

static void ScreenToWorld(const double screenX, const double screenY, float& worldX, float& worldY, const Gui::PanSettings& pan, const Gui::ZoomSettings& zoom) {
    worldX = static_cast<float>(screenX - pan.xOffset) / zoom.xScale;
    worldY = static_cast<float>(screenY - pan.yOffset) / zoom.yScale; 
}

void Gui::RenderWindow::mouseButtonCallback(GLFWwindow* window, const int button, const int action, [[maybe_unused]] int mods) {
    const auto renderWindow = Renderer::GetInstance()->getRenderWindow(window);
    if (renderWindow == nullptr) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        renderWindow->m_isDraggingMouse = true;
        double xPosDouble, yPosDouble;
        glfwGetCursorPos(window, &xPosDouble, &yPosDouble);
        renderWindow->m_panSettings.lastMouseX = static_cast<float>(xPosDouble);
        renderWindow->m_panSettings.lastMouseY = static_cast<float>(yPosDouble);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        renderWindow->m_isDraggingMouse = false;
    }
}

void Gui::RenderWindow::cursorPositionCallback([[maybe_unused]] GLFWwindow* window, const double xPos, const double yPos) {
    const auto renderWindow = Renderer::GetInstance()->getRenderWindow(window);
    if (renderWindow == nullptr) return;
    if (!renderWindow->m_isDraggingMouse) return;

    const bool outOfBounds = xPos < 0 || xPos >= renderWindow->m_width || yPos < 0 || yPos >= renderWindow->m_height;
    if (outOfBounds) {
        renderWindow->m_isDraggingMouse = false;
        return;
    }

    const float deltaX = static_cast<float>(xPos) - renderWindow->m_panSettings.lastMouseX;
    const float deltaY = static_cast<float>(yPos) - renderWindow->m_panSettings.lastMouseY;
    
    renderWindow->m_panSettings.xOffset += deltaX;
    renderWindow->m_panSettings.yOffset += deltaY;

    renderWindow->m_panSettings.lastMouseX = static_cast<float>(xPos); 
    renderWindow->m_panSettings.lastMouseY = static_cast<float>(yPos);

    renderWindow->updateViewMatrix();
    renderWindow->updateMVP();
}

void Gui::RenderWindow::setMouseCallbacksForPanning() const {
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPositionCallback);
}

void Gui::RenderWindow::keyboardCallbacks(GLFWwindow* window, const int key, [[maybe_unused]] int scancode, const int action, [[maybe_unused]] int mods) {
    const auto renderWindow = Renderer::GetInstance()->getRenderWindow(window);
    if (renderWindow == nullptr) return;
    
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    const bool outOfBounds = xPos < 0 || xPos >= renderWindow->m_width || yPos < 0 || yPos >= renderWindow->m_height;
    if (outOfBounds) return;
    
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key != GLFW_KEY_Q && key != GLFW_KEY_W) return;

        float worldBeforeMouseX, worldBeforeMouseY;
        ScreenToWorld(xPos, yPos, worldBeforeMouseX, worldBeforeMouseY, renderWindow->m_panSettings, renderWindow->m_zoomSettings);
        
        const float zoomFactor =  key == GLFW_KEY_Q ? 1.1f : (1 / 1.1f);
        renderWindow->m_zoomSettings.xScale *= zoomFactor;
        renderWindow->m_zoomSettings.yScale *= zoomFactor;
        renderWindow->updateProjectionMatrix();
        
        float newScreenX, newScreenY;
        WorldToScreen(worldBeforeMouseX, worldBeforeMouseY, newScreenX, newScreenY, renderWindow->m_panSettings, renderWindow->m_zoomSettings);

        const float deltaX = newScreenX - static_cast<float>(xPos);
        const float deltaY = newScreenY - static_cast<float>(yPos);
        
        renderWindow->m_panSettings.xOffset -= deltaX;
        renderWindow->m_panSettings.yOffset -= deltaY;
        
        const float xOffset = renderWindow->m_panSettings.xOffset / renderWindow->m_zoomSettings.xScale;
        const float yOffset = renderWindow->m_panSettings.yOffset / renderWindow->m_zoomSettings.yScale;
        renderWindow->m_viewMatrix = translate(glm::mat4(1.0f),
            glm::vec3(xOffset, yOffset, 0.0f));
        
        renderWindow->updateMVP();
    }
}

void Gui::RenderWindow::setKeyboardCallback() const {
    glfwSetKeyCallback(m_window, keyboardCallbacks);
}

void Gui::RenderWindow::updateProjectionMatrix() {
    m_projectionMatrix = glm::ortho(0.0f,
            static_cast<float>(m_width) / m_zoomSettings.xScale,
            static_cast<float>(m_height) / m_zoomSettings.yScale,
            0.0f, -1.0f, 1.0f);
}

void Gui::RenderWindow::updateViewMatrix() {
    const float xOffset = m_panSettings.xOffset / m_zoomSettings.xScale;
    const float yOffset = m_panSettings.yOffset / m_zoomSettings.yScale;
    m_viewMatrix = translate(glm::mat4(1.0f),
        glm::vec3(xOffset, yOffset, 0.0f));
}

void Gui::RenderWindow::updateMVP() const {
    glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
    glUniformMatrix4fv(m_mvpUniform, 1, GL_FALSE, value_ptr(mvp));
}

void Gui::RenderWindow::setBufferDataPoints(const std::shared_ptr<std::vector<Point>>& points) {
    makeCurrentContext();
    const int pointsByteSize = static_cast<int>(sizeof(Point) * points->size());
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, pointsByteSize, points->data(), GL_STATIC_DRAW);
    m_vertexCount = static_cast<int>(points->size());
}

void Gui::RenderWindow::renderPoints() {
    makeCurrentContext();
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glfwSwapBuffers(m_window);
}
