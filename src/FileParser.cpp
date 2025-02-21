#include <filesystem>
#include <iostream>

#include "Bmp.h"
#include "FileUtil.h"
#include "Jpg.h"
#include "Renderer.h"

static int Main(const int argc, char* argv[], Renderer& renderer) {
    using namespace fileUtils;
    
    if (argc != 2) {
        std::cerr << "Usage: ./FileParser <filename>" << '\n';
        return -1;
    }
    
    std::string filePath = argv[1];
    
    if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
        std::cout << "The file path is valid and points to a file: " << filePath << '\n';
    } else {
        std::cerr << "Error: The file path is invalid or does not point to a regular file: " << filePath << '\n';
        return -1;
    }

    switch (GetFileType(filePath)) {
    case FileType::Bmp: {
        Bmp bmp(filePath);
        bmp.render(renderer);
        break;
    }
    case FileType::Jpg: {
        Jpg jpg(filePath);
        jpg.render(renderer);
        break;
    }
    case FileType::None:
        break;
    }
    
    return 0;
}

// TODO: Separate renderer and conversion functions into separate modules
int main(const int argc, char* argv[]) {
    Renderer renderer;
    renderer.run();
    
    std::thread cmdThread([&] {
        Main(argc, argv, renderer);
    });
    
    cmdThread.detach();
    while (renderer.isRunning()) {
        
    }
    std::cout << "End main\n";
    return 0;
}