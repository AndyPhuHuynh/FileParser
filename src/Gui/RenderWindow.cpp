#include "Gui/RenderWindow.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "ShaderUtil.h"
#include "Gui/Renderer.h"
#include "Point.h"
#include "Shaders.h"

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
}

Gui::RenderWindow::RenderWindow(RenderWindow&& other) noexcept
    : m_title(std::move(other.m_title)), m_width(other.m_width), m_height(other.m_height),
    m_window(other.m_window), m_visible(other.m_visible), m_renderMode(other.m_renderMode), m_shaderProgram(other.m_shaderProgram) {
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
        m_shaderProgram = other.m_shaderProgram;
        
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
        m_shaderProgram = Shaders::Util::CreateShader(Shaders::PointVertexShader, Shaders::PointFragmentShader);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        m_render = &Gui::RenderWindow::renderPoints;
        m_panSettings.xOffsetUniform = glGetUniformLocation(m_shaderProgram, "xOffset");
        m_panSettings.yOffsetUniform = glGetUniformLocation(m_shaderProgram, "yOffset");
    }
    glUseProgram(m_shaderProgram);
}

bool Gui::RenderWindow::isVisible() {
    return m_visible;
}

std::future<void> Gui::RenderWindow::windowShouldCloseAsync() {
    auto promise = std::make_shared<std::promise<void>>();

    if (Renderer::GetInstance()->onRenderThread()) {
        windowShouldCloseAsync();
        promise->set_value();
    } else {
        Renderer::GetInstance()->queueFunction([this, promise] {
            windowShouldCloseAsync();
            promise->set_value();
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
    (this->*m_render)();
}

void Gui::RenderWindow::makeCurrentContext() {
    glfwMakeContextCurrent(m_window);
    Renderer::GetInstance()->m_currentWindow = this;
}

bool Gui::RenderWindow::windowShouldClose() {
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

void Gui::RenderWindow::mouseButtonCallback(GLFWwindow* window, const int button, const int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        auto renderWindow = Renderer::GetInstance()->m_currentWindow;
        renderWindow->m_isDraggingMouse = true;

        double xPosDouble, yPosDouble;
        glfwGetCursorPos(window, &xPosDouble, &yPosDouble);
        renderWindow->m_panSettings.startPanX = Shaders::Util::NormalizeToNdc(static_cast<float>(xPosDouble), renderWindow->m_width);
        renderWindow->m_panSettings.startPanY = Shaders::Util::NormalizeToNdc(static_cast<float>(yPosDouble), renderWindow->m_height) * -1;
        
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        auto renderWindow = Renderer::GetInstance()->m_currentWindow;
        renderWindow->m_isDraggingMouse = false;
    }
}

void Gui::RenderWindow::cursorPositionCallback(GLFWwindow* window, const double xPos, const double yPos) {
    auto renderWindow = Renderer::GetInstance()->m_currentWindow;
    if (!renderWindow->m_isDraggingMouse) return;
    
    bool outOfBounds = xPos < 0 || xPos >= renderWindow->m_width || yPos < 0 || yPos >= renderWindow->m_height;
    if (outOfBounds) return;
    
    float xPosNormalized = Shaders::Util::NormalizeToNdc(static_cast<float>(xPos), renderWindow->m_width);
    float yPosNormalized = Shaders::Util::NormalizeToNdc(static_cast<float>(yPos), renderWindow->m_height) * -1.0f;

    renderWindow->m_panSettings.xOffset += xPosNormalized - renderWindow->m_panSettings.startPanX;
    renderWindow->m_panSettings.yOffset += yPosNormalized - renderWindow->m_panSettings.startPanY;

    renderWindow->m_panSettings.startPanX = xPosNormalized;
    renderWindow->m_panSettings.startPanY = yPosNormalized;
    
    glUniform1f(renderWindow->m_panSettings.xOffsetUniform, renderWindow->m_panSettings.xOffset);
    glUniform1f(renderWindow->m_panSettings.yOffsetUniform, renderWindow->m_panSettings.yOffset);
}

void Gui::RenderWindow::setMouseCallbacksForPanning() {
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPositionCallback);
}

void Gui::RenderWindow::setBufferDataPoints(const std::shared_ptr<std::vector<Point>>& points) {
    makeCurrentContext();
    int pointsByteSize = static_cast<int>(sizeof(Point) * points->size());
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
