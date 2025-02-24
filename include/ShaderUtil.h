#pragma once
#include <string>

namespace Shaders::Util {
    float NormalizeToNdc(float value, int span);
    unsigned int CompileShader(unsigned int shaderType, const std::string& source);
    unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
}
