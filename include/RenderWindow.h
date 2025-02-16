#pragma once
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "Point.h"

enum class RenderMode : std::uint8_t {
    Point,
};

class RenderWindow {
public:
    RenderWindow(Renderer *renderer, int width, int height, std::string title, RenderMode renderMode);
    RenderWindow(const RenderWindow&) = delete;
    RenderWindow& operator=(const RenderWindow&) = delete;
    RenderWindow(RenderWindow&& other) noexcept;
    RenderWindow& operator=(RenderWindow&& other) noexcept;
    ~RenderWindow();
    
    void hideWindow();
    void showWindow();
    void setRenderMode(RenderMode mode);
    bool isVisible();
    void makeCurrentContext();
    bool windowShouldClose();
    
    void setBufferDataPoints(const std::vector<Point>& points);
    void renderFrame();
private:
    Renderer *m_renderer;
    std::string m_title;
    int m_width = 0;
    int m_height = 0;
    GLFWwindow *m_window;
    bool m_visible = false;
    
    RenderMode m_renderMode = RenderMode::Point;
    unsigned int m_shaderProgram = 0;
    unsigned int m_vertexBuffer = 0;
    int m_vertexCount = 0;

    void (RenderWindow::*m_render)() = nullptr;
    void renderPoints();
};
