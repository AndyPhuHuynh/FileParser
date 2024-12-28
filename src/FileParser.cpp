#include <Bmp.h>
#include <filesystem>
#include <iostream>
int main(const int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./FileParser <filename>" << '\n';
    }
    
    std::string filePath = argv[1];
    
    if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
        std::cout << "The file path is valid and points to a file: " << filePath << '\n';
    } else {
        std::cerr << "Error: The file path is invalid or does not point to a regular file: " << filePath << '\n';
        return 1;
    }

    std::cout << "File Path: " << filePath << '\n';
    
    Bmp bmp(filePath);
    bmp.render();
    
    return 0;
}
