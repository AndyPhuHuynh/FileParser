#pragma once
#include <string>
#include <vector>
#include <future>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "FileParser/Gui/Renderer.h"
#include "FileParser/Point.h"

namespace Gui {
    class Renderer;

    struct PanSettings {
        float lastMouseY = 0;            
        float lastMouseX = 0;            
        float xOffset = 0;              
        float yOffset = 0;              
    };

    struct ZoomSettings {
        float xScale = 1;
        float yScale = 1;
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
        bool isVisible() const;

        std::future<bool> windowShouldCloseAsync() const;
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
        ZoomSettings m_zoomSettings;

        glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
        glm::mat4 m_viewMatrix = glm::mat4(1.0f);
        glm::mat4 m_modelMatrix = glm::mat4(1.0f);
        
        RenderMode m_renderMode = RenderMode::Point;
        unsigned int m_shaderProgram = 0;
        unsigned int m_vertexBuffer = 0;
        int m_vertexCount = 0;
        GLint m_mvpUniform = -1;

        void makeCurrentContext();
        bool windowShouldClose() const;
        void hideWindow();
        void showWindow();
        
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
        void setMouseCallbacksForPanning() const;

        static void keyboardCallbacks(GLFWwindow* window, int key, int scancode, int action, int mods);
        void setKeyboardCallback() const;

        void updateProjectionMatrix();
        void updateViewMatrix();
        void updateMVP() const;
        
        void setBufferDataPoints(const std::shared_ptr<std::vector<Point>>& points);
        
        void (RenderWindow::*m_render)() = nullptr;
        void renderPoints();
    };
}