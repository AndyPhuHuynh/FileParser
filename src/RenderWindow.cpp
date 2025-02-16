#include "RenderWindow.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "ShaderUtil.h"
#include "Renderer.h"
#include "Point.h"
#include "Shaders.h"

RenderWindow::RenderWindow(Renderer *renderer, const int width, const int height, std::string title, const RenderMode renderMode)
    : m_renderer(renderer), m_title(std::move(title)), m_width(width), m_height(height) {
    // Create a window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_window = glfwCreateWindow(this->m_width, this->m_height, this->m_title.c_str(),nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        std::terminate();
    }
    glfwMakeContextCurrent(m_window);
    
    // Initialize glew
    if (!m_renderer->isGlewInitialized()) {
        m_renderer->initializeGlew();
    }
    glGenBuffers(1, &m_vertexBuffer);
    glViewport(0, 0, width, height);
    setRenderMode(renderMode);
}

RenderWindow::RenderWindow(RenderWindow&& other) noexcept
    : m_renderer(other.m_renderer), m_title(std::move(other.m_title)), m_width(other.m_width), m_height(other.m_height),
    m_window(other.m_window), m_visible(other.m_visible), m_renderMode(other.m_renderMode), m_shaderProgram(other.m_shaderProgram) {
    other.m_renderer = nullptr;
    other.m_window = nullptr;
    other.m_shaderProgram = 0;
}

RenderWindow& RenderWindow::operator=(RenderWindow&& other) noexcept {
    if (this != &other) {
        glDeleteProgram(m_shaderProgram);
        glfwDestroyWindow(m_window);
        
        m_renderer = other.m_renderer;
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

RenderWindow::~RenderWindow() {
    glDeleteProgram(m_shaderProgram);
    glfwDestroyWindow(m_window);
}

void RenderWindow::hideWindow() {
    m_visible = false;
    glfwHideWindow(m_window);
}

void RenderWindow::showWindow() {
    m_visible = true;
    glfwShowWindow(m_window);
}

void RenderWindow::setRenderMode(const RenderMode mode) {
    m_renderMode = mode;
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
    }
    switch (mode) {
    case RenderMode::Point:
        m_shaderProgram = CreateShader(shaders::PointVertexShader, shaders::PointFragmentShader);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        m_render = &RenderWindow::renderPoints;
    }
    glUseProgram(m_shaderProgram);
}

bool RenderWindow::isVisible() {
    return m_visible;
}

void RenderWindow::makeCurrentContext() {
    glfwMakeContextCurrent(m_window);
}

bool RenderWindow::windowShouldClose() {
    return glfwWindowShouldClose(m_window);
}

void RenderWindow::setBufferDataPoints(const std::vector<Point>& points) {
    int pointsByteSize = static_cast<int>(sizeof(Point) * points.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, pointsByteSize, points.data(), GL_STATIC_DRAW);
    m_vertexCount = static_cast<int>(points.size());
}

void RenderWindow::renderFrame() {
    (this->*m_render)();
}

void RenderWindow::renderPoints() {
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
        
    glfwSwapBuffers(m_window);
}