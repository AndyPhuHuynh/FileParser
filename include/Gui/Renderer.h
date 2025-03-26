#pragma once
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <future>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Gui {
    class RenderWindow;

    enum class RenderMode : std::uint8_t {
        Point,
    };

    class Renderer {
    public:
        static void Init();
        static Renderer* GetInstance();
    
        Renderer(Renderer const&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        std::shared_ptr<RenderWindow> getRenderWindow(GLFWwindow* window);
        bool isRunning() const;
        bool onRenderThread() const;

        std::future<std::weak_ptr<RenderWindow>> createWindowAsync(int width, int height, const std::string& title, RenderMode renderMode);
        std::future<void> removeWindowAsync(const std::shared_ptr<RenderWindow>& renderWindow);
        std::future<void> removeWindowAsync(const std::weak_ptr<RenderWindow>& renderWindow);
    private:
        Renderer();
        ~Renderer();
    
        friend class RenderWindow;
        bool m_glewInitialized = false;
        std::unordered_map<GLFWwindow*, std::shared_ptr<RenderWindow>> m_renderWindows;
        std::atomic<bool> m_running = false;
        std::mutex m_functionQueueMutex;
        std::queue<std::function<void()>> m_functionQueue;
        std::thread m_renderThread;
        std::thread::id m_renderThreadId;
        RenderWindow *m_currentWindow = nullptr;
        
        void initializeGlew();
        // Initializes a separate thread where the renderer can run in the background rendering all windows
        void run();
        void stopRendering();
    
        std::weak_ptr<RenderWindow> createWindow(int width, int height, const std::string& title, RenderMode renderMode);
        void removeWindow(const std::shared_ptr<RenderWindow>& renderWindow);
        void removeWindow(const std::weak_ptr<RenderWindow>& renderWindow);
    
        void processEventLoop();
        void queueFunction(const std::function<void()>& function);
    };
}