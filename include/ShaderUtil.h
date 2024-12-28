#pragma once
#include <string>

float NormalizeToNdc(float value, int span);
int GetBit(unsigned char byte, int pos);
int GetNibble(unsigned char byte, int pos);

unsigned int CompileShader(unsigned int shaderType, const std::string& source);
unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
