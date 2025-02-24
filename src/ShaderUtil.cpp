#include "ShaderUtil.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <GL/glew.h>

float Shaders::Util::NormalizeToNdc(const float value, const int span) {
    return (value / static_cast<float>(span)) * 2 - 1;
}

unsigned int Shaders::Util::CompileShader(const unsigned int shaderType, const std::string& source) {
    unsigned int id = glCreateShader(shaderType);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(length);
        glGetShaderInfoLog(id, length, &length, message.data());
        std::cout << "Failed to compile " << (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment") <<" shader!\n";
        std::cout << message.data() << '\n';
        glDeleteShader(id);
        return 0;
    }

    return id;
}

unsigned int Shaders::Util::CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}
