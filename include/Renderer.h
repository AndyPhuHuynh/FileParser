#pragma once
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

enum class RenderMode : std::uint8_t;
class Renderer;
class RenderWindow;

class Renderer {
public:
    Renderer();
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer();
    
    bool isGlewInitialized();
    void initializeGlew();
    
    std::unique_ptr<RenderWindow>& createWindow(int width, int height, const std::string& title, RenderMode renderMode);
    void removeWindow(const std::unique_ptr<RenderWindow>& renderWindow);

    // Initializes a separate thread where the renderer can run in the background rendering all windows
    void run();
    void stopRendering();
private:
    bool m_glewInitialized = false;
    std::vector<std::unique_ptr<RenderWindow>> m_renderWindows;
    std::atomic<bool> m_running = false;
    std::mutex m_functionQueueMutex;
    std::queue<std::function<void()>> m_functionQueue;

    void addFunction(std::function<void()> function);
};
