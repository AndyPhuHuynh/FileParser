#include <filesystem>
#include <iostream>

#include "Bmp.h"
#include "Jpg.h"
#include "Renderer.h"

static int bmpMain(const int argc, char* argv[], Renderer& renderer) {
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
    
    Bmp bmp(filePath);
    bmp.render(renderer);
    
    return 0;
}

int main(const int argc, char* argv[]) {
    Renderer renderer;

    // std::thread cmdThread([&] {
    //     bmpMain(argc, argv, renderer);
    // });
    //
    // cmdThread.detach();
    bmpMain(argc, argv, renderer);
    renderer.run();
    std::cout << "End main\n";
    return 0;
    clock_t begin = clock();
    std::string filename = "./test-images/cat";
    std::cout << "Processing file: " << filename << '\n';
    std::ostringstream path;
    path << filename << ".jpg";
    
    std::ostringstream out;
    out << filename << ".bmp";
    
    Jpg jpg(path.str());
    
    jpg.writeBmp(out.str());
    jpg.printInfo();
    // Bmp bmp("sample.bmp");
    // bmp.render();
    clock_t end = clock();
    double time_spent = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    std::cout << "Total time: " << time_spent << '\n';
}