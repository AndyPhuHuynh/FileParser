#include "FileParser/ShaderUtil.h"

#include <iostream>
#include <fstream>
#include <optional>
#include <vector>

#include <GL/glew.h>

float Shaders::Util::NormalizeToNdc(const float value, const int span) {
    return (value / static_cast<float>(span)) * 2 - 1;
}

unsigned int Shaders::Util::CompileShader(const unsigned int shaderType, const std::string& source) {
    const unsigned int id = glCreateShader(shaderType);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(static_cast<size_t>(length));
        glGetShaderInfoLog(id, length, &length, message.data());
        std::string shaderTypeString;
        if (shaderType == GL_VERTEX_SHADER) {
            shaderTypeString = "Vertex";
        } else if (shaderType == GL_FRAGMENT_SHADER) {
            shaderTypeString = "Fragment";
        } else if (shaderType == GL_GEOMETRY_SHADER) {
            shaderTypeString = "Geometry";
        }
        std::cout << "Error compiling " << shaderTypeString << "shader: " << message.data() << "\n";
        glDeleteShader(id);
        return 0;
    }

    return id;
}

unsigned int Shaders::Util::CreateShader(const std::string& vertexShader, const std::optional<std::string>& geometryShader, const std::string& fragmentShader) {
    const unsigned int program = glCreateProgram();
    const unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    const unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    unsigned int gs = 0;
    
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    if (geometryShader.has_value()) {
        gs = CompileShader(GL_GEOMETRY_SHADER, geometryShader.value());
        glAttachShader(program, gs);
    }
    
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Error linking shader:\n" << infoLog << '\n';
        return 0;
    }
    
    glValidateProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Error validating shader:\n" << infoLog << '\n';
        return 0;
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (geometryShader.has_value()) {
        glDeleteShader(gs);
    }
    
    return program;
}
