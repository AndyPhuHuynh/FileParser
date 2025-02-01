#include <filesystem>
#include <iostream>

#include "Bmp.h"
#include "Jpg.h"

static int bmpMain(const int argc, char* argv[]) {
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
    
    Bmp bmp(filePath);
    bmp.render();
    
    return 0;
}

int main(const int argc, char* argv[]) {
    clock_t begin = clock();
    std::string filename = "crab";
    std::cout << "Processing file: " << filename << '\n';
    std::ostringstream path;
    path << filename << ".jpg";
    
    std::ostringstream out;
    out << filename << ".bmp";
    
    Jpg jpg(path.str());

    if (jpg.frameHeader.encodingProcess != 0xC0) {
        for (auto& mcu : jpg.mcus) {
            mcu->dequantize(&jpg);
            mcu->performInverseDCT();
            mcu->generateColorBlocks();
        }
    }
    
    jpg.writeBmp(out.str());
    // jpg.printInfo();
    // Bmp bmp("sample.bmp");
    // bmp.render();
    clock_t end = clock();
    double time_spent = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    std::cout << "Total time: " << time_spent << '\n';
}