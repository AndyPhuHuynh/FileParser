#include <algorithm>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "RenderWindow.h"

Renderer::Renderer() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
    }
}

Renderer::~Renderer() {
    stopRendering();
    m_renderWindows.clear();
    glfwTerminate();
}

bool Renderer::isGlewInitialized() {
    return m_glewInitialized;
}

void Renderer::initializeGlew() {
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << '\n';
        std::terminate();
    } else {
        std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
        m_glewInitialized = true;
    }
}

std::unique_ptr<RenderWindow>& Renderer::createWindow(int width, int height, const std::string& title, RenderMode renderMode) {
    m_renderWindows.emplace_back(std::make_unique<RenderWindow>(this, width, height, title, renderMode));
    return m_renderWindows.back();
}

void Renderer::removeWindow(const std::unique_ptr<RenderWindow>& renderWindow) {
    std::erase(m_renderWindows, renderWindow);
}

void Renderer::run() {
    m_running = true;
    while (m_running) {
        for (int i = static_cast<int>(m_renderWindows.size()) - 1; i >= 0; i--) {
            auto& window = m_renderWindows[i];
            window->makeCurrentContext();
            if (window->windowShouldClose()) {
                removeWindow(window);
                continue;
            }
            window->renderFrame();
            glfwPollEvents();
        }
    }
}

void Renderer::stopRendering() {
    m_running = false;
}

void Renderer::addFunction(std::function<void()> function) {
    std::unique_lock lock(m_functionQueueMutex);
    m_functionQueue.push(function);
}
