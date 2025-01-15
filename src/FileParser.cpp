#include <filesystem>
#include <iostream>

#include <Bmp.h>
#include "Jpg.h"
#include <simde/x86/sse4.1.h>

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
    // constexpr int size = 100;
    // int array[size] = {0};
    // int mult[size] = {2};
    //
    // for (int i = 0; i < size; i++) {
    //     array[i] = i;
    //     mult[i] = 2;
    // }
    // size_t simdLength = size / 4 * 4; // Ensure multiple of 4 for SIMD
    //
    // // Process 4 elements at a time
    // for (size_t i = 0; i < simdLength; i += 4) {
    //     simde__m128i arrayVec = simde_mm_loadu_si128(&array[i]); // Load 4 integers
    //     simde__m128i quantTableVec = simde_mm_loadu_si128(&mult[i]); // Load 4 integers
    //     simde__m128i resultVec = simde_mm_mullo_epi32(arrayVec, quantTableVec); // Multiply element-wise
    //     simde_mm_storeu_si128(&array[i], resultVec); // Store the result back to the array
    // }
    //
    // // Handle any remaining elements (if dataUnitLength is not a multiple of 4)
    // for (size_t i = simdLength; i < size; ++i) {
    //     array[i] *= mult[i];
    // }
    //
    // for (int i = 0; i < size; ++i) {
    //     std::cout << array[i] << '\n';
    // }
    // return 0;
    clock_t begin = clock();
    std::string filename = "turtle";
    std::cout << "Processing file: " << filename << '\n';
    std::ostringstream path;
    path << filename << ".jpg";
    
    std::ostringstream out;
    out << filename << ".bmp";
    
    Jpg jpg(path.str());
    // jpg.writeBmp(out.str());
    // jpg.printInfo();
    // Bmp bmp("sample.bmp");
    // bmp.render();
    clock_t end = clock();
    double time_spent = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    std::cout << "Total time: " << time_spent << '\n';
}