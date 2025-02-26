#pragma once
#include <string>
#include <vector>
#include <future>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "Point.h"

namespace Gui {
    class Renderer;

    struct PanSettings {
        float startPanX = 0;            
        float startPanY = 0;            
        float xOffset = 0;              
        float yOffset = 0;              
        GLint xOffsetUniform = -1;      
        GLint yOffsetUniform = -1;      
    };
    
    class RenderWindow {
    public:
        RenderWindow(int width, int height, std::string title, RenderMode renderMode);
        RenderWindow(const RenderWindow&) = delete;
        RenderWindow& operator=(const RenderWindow&) = delete;
        RenderWindow(RenderWindow&& other) noexcept;
        RenderWindow& operator=(RenderWindow&& other) noexcept;
        ~RenderWindow();
    
        void setRenderMode(RenderMode mode);
        bool isVisible();

        std::future<void> windowShouldCloseAsync();
        std::future<void> hideWindowAsync();
        std::future<void> showWindowAsync();
        
        std::future<void> setBufferDataPointsAsync(const std::shared_ptr<std::vector<Point>>& points);
        void renderFrame();
    private:
        friend class Renderer;
        std::string m_title;
        int m_width = 0;
        int m_height = 0;
        GLFWwindow *m_window;
        bool m_visible = false;

        bool m_isDraggingMouse = false;
        PanSettings m_panSettings;
        
        RenderMode m_renderMode = RenderMode::Point;
        unsigned int m_shaderProgram = 0;
        unsigned int m_vertexBuffer = 0;
        int m_vertexCount = 0;

        void makeCurrentContext();
        bool windowShouldClose();
        void hideWindow();
        void showWindow();
        
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
        void setMouseCallbacksForPanning();
        
        void setBufferDataPoints(const std::shared_ptr<std::vector<Point>>& points);
        
        void (RenderWindow::*m_render)() = nullptr;
        void renderPoints();
    };
}