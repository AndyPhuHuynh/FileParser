#include <algorithm>
#include <iostream>
#include <future>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "FileParser/Gui/Renderer.h"
#include "FileParser/Gui/RenderWindow.h"

static Gui::Renderer *s_renderer = nullptr;

Gui::Renderer::Renderer() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
    }
}

Gui::Renderer::~Renderer() {
    stopRendering();
    m_renderWindows.clear();
    glfwTerminate();
}

void Gui::Renderer::Init() {
    s_renderer = new Renderer();
    s_renderer->run();
}

Gui::Renderer* Gui::Renderer::GetInstance() {
    return s_renderer;
}

std::shared_ptr<Gui::RenderWindow> Gui::Renderer::getRenderWindow(GLFWwindow* window) {
    if (m_renderWindows.contains(window)) {
        return m_renderWindows.at(window);
    }
    return nullptr;
}

bool Gui::Renderer::isRunning() const {
    return m_running;
}

bool Gui::Renderer::onRenderThread() const {
    return std::this_thread::get_id() == m_renderThreadId;
}

void Gui::Renderer::initializeGlew() {
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << '\n';
        std::terminate();
    } else {
        std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
        m_glewInitialized = true;
    }
}

std::future<std::weak_ptr<Gui::RenderWindow>> Gui::Renderer::createWindowAsync(
    const int width, const int height, const std::string& title, const RenderMode renderMode) {
    auto promise = std::make_shared<std::promise<std::weak_ptr<RenderWindow>>>();

    if (std::this_thread::get_id() == m_renderThreadId) {
        promise->set_value(createWindow(width, height, title, renderMode));
    } else {
        queueFunction([this, promise, width, height, title, renderMode]() {
            promise->set_value(createWindow(width, height, title, renderMode));
        });
    }

    return promise->get_future();
}

std::future<void> Gui::Renderer::removeWindowAsync(const std::shared_ptr<RenderWindow>& renderWindow) {
    auto promise = std::make_shared<std::promise<void>>();

    if (std::this_thread::get_id() == m_renderThreadId) {
        removeWindow(renderWindow);
        promise->set_value();
    } else {
        queueFunction([this, promise, renderWindow]() {
            removeWindow(renderWindow);
            promise->set_value();
        });
    }

    return promise->get_future();
}

std::future<void> Gui::Renderer::removeWindowAsync(const std::weak_ptr<RenderWindow>& renderWindow) {
    auto promise = std::make_shared<std::promise<void>>();

    if (std::this_thread::get_id() == m_renderThreadId) {
        removeWindow(renderWindow);
        promise->set_value();
    } else {
        queueFunction([this, promise, renderWindow]() {
            removeWindow(renderWindow);
            promise->set_value();
        });
    }

    return promise->get_future();
}

std::weak_ptr<Gui::RenderWindow> Gui::Renderer::createWindow(const int width, const int height, const std::string& title, const RenderMode renderMode) {
    auto renderWindow = std::make_shared<RenderWindow>(width, height, title, renderMode);
    m_renderWindows.emplace(renderWindow->m_window, renderWindow);
    return renderWindow;
}

void Gui::Renderer::removeWindow(const std::shared_ptr<RenderWindow>& renderWindow) {
    m_renderWindows.erase(renderWindow->m_window);
}

void Gui::Renderer::removeWindow(const std::weak_ptr<RenderWindow>& renderWindow) {
    if (const auto renderWindowShared = renderWindow.lock()) {
        m_renderWindows.erase(renderWindowShared->m_window);
    }
}

void Gui::Renderer::run() {
    m_running = true;
    m_renderThread = std::thread([&] {
        processEventLoop();
    });
    m_renderThreadId = m_renderThread.get_id();
    m_renderThread.detach();
}

void Gui::Renderer::stopRendering() {
    m_running = false;
}

void Gui::Renderer::processEventLoop() {
    while (m_running) {
        // Handle functions queue
        std::queue<std::function<void()>> functions;
        std::unique_lock lock(m_functionQueueMutex);
        std::swap(functions, m_functionQueue);
        lock.unlock();
        while (!functions.empty()) {
            auto function = functions.front();
            function();
            functions.pop();
        }
        
        // Render windows
        for (auto it = m_renderWindows.begin(); it != m_renderWindows.end(); ) {
            const auto& window = it->second;

            window->makeCurrentContext();
    
            if (window->windowShouldClose()) {
                it = m_renderWindows.erase(it); 
                continue;
            }

            window->renderFrame();
            glfwPollEvents();

            it++;
        }
    }
}

void Gui::Renderer::queueFunction(const std::function<void()>& function) {
    std::unique_lock lock(m_functionQueueMutex);
    m_functionQueue.push(function);
}
