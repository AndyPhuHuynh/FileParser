#include "ShaderUtil.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <Gl/glew.h>

float NormalizeToNdc(const float value, const int span) {
    return (value / static_cast<float>(span)) * 2 - 1;
}

int GetBit(const unsigned char byte, const int pos) {
    if (pos < 0 || pos >= 8) {
        std::cout << "Error in GetBit: pos is out of bounds: " << pos << "\n";
        return -1;
    }
    return (byte & (1 << (7 - pos))) >> (7 - pos);
}

int GetNibble(const unsigned char byte, const int pos) {
    if (pos < 0 || pos > 1) {
        std::cout << "Error in GetNibble: pos is out of bounds: " << pos << "\n";
        return -1;
    }
    if (pos == 1) {
        return byte & 15;
    }
    return (byte >> 4) & 15;
}

unsigned int CompileShader(const unsigned int shaderType, const std::string& source) {
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

unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
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
