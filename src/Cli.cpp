#include "Cli.h"

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Bmp.h"
#include "FileUtil.h"
#include "Jpeg/JpegBmpConverter.h"
#include "Jpeg/JpegEncoder.h"
#include "Jpeg/JpegImage.h"
#include "Jpeg/JpegRenderer.h"

/**
 * @brief Tokenizes a string based on a delimiter
 * @param str The string that will be tokenized
 * @param delimiter The character used as the delimiter
 * @return A vector of the tokens in the tokenized string
 */
static std::vector<std::string> SplitString(const std::string& str, const char delimiter) {
    std::vector<std::string> tokens;
    int prevIndex = 0;
    const int length = static_cast<int>(str.length());
    for (int i = 0; i < length; i++) {
        if (str[i] == delimiter) {
            // Add token only if it's non-empty
            if (prevIndex != i) {
                tokens.push_back(str.substr(prevIndex, i - prevIndex));
            }
            // Skip consecutive delimiters
            while (i < length && str[i] == delimiter) {
                i++;
            }
            prevIndex = i;
        }
    }

    // Add the last token if its non-empty
    if (prevIndex < length) {
        tokens.push_back(str.substr(prevIndex));
    }
    
    return tokens;
}

/**
 * @brief Renders an image file based on its type.
 * @param args A vector of the input arguments.
 *  - args[0] is the name of the command "render".
 *  - args[1] is the filepath to the image file.
 */
static void Render(const std::vector<std::string>& args) {
    using namespace fileUtils;
    using namespace ImageProcessing;
    if (args.size() != 2) {
        std::cerr << "Usage: render <filename>\n";
        return;
    }
    
    const std::string& filepath = args[1];
    
    if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath)) {
        std::cout << "The file path is valid and points to a file: " << filepath << '\n';
    } else {
        std::cerr << "Error: The file path is invalid or does not point to a regular file: " << filepath << '\n';
        return;
    }

    switch (GetFileType(filepath)) {
    case FileType::Bmp: {
        Bmp bmp(filepath);
        bmp.render();
        break;
    }
    case FileType::Jpeg: {
        Jpeg::JpegImage jpeg(filepath);
        Jpeg::Renderer::RenderJpeg(jpeg);
        break;
    }
    case FileType::None:
        break;
    }
}

/**
 * @brief Converts a file to a different file format.
 * @param args A vector of the input arguments.
 *  - args[0] is the name of the command "convert".
 *  - args[1] is the filepath to the image file.
 *  - args[2] is the new file format
 */
static void Convert(const std::vector<std::string>& args) {
    using namespace fileUtils;
    using namespace ImageProcessing;

    if (args.size() != 3 && args.size() != 4) {
        std::cerr << "Usage: convert <filename> <new format> [<new filepath>] (default: <filename>.<new format>)\n";
        return;
    }

    const std::string& filepath = args[1];
    const std::string& format = args[2];
    const std::string& newFilepath = args.size() < 4? filepath + "." + format : args[3];
    
    switch (GetFileType(filepath)) {
    case FileType::Bmp: {
        std::cout << "Bmp conversion to other files not supported yet\n";
        break;
    }
    case FileType::Jpeg: {
        Jpeg::JpegImage jpeg(filepath);
        if (format == "bmp") {
            Jpeg::Converter::WriteJpegAsBmp(jpeg, newFilepath);
        } else {
            std::cout << "Only conversion from jpg to bmp is supported\n";
        }
        break;
    }
    case FileType::None:
        std::cout << "Original file type not supported\n";
        break;
    }
}

static void Test(const std::vector<std::string>& args) {
    using namespace ImageProcessing;
    std::ofstream outfile("test.jpeg", std::ios::binary);

    Bmp bmp("./test-images/cat.bmp");
    Jpeg::Encoder::getMcus(bmp);
    
    std::array<float, 64> data = {
        5, 0, 2 ,0 ,4 ,0 ,0 ,0,
        -1, 0, 0 ,0 ,0 ,0 ,0 ,0,
        0, 0, 0 ,0 ,6 ,0 ,0 ,0,
        3, 0, 0 ,0 ,0 ,8 ,0 ,0,
        0, 5, 0 ,-7 ,0 ,0 ,0 ,0,
        0, 0, 0 ,0 ,0 ,0 ,0 ,0,
        0, 0, 0 ,0 ,0 ,0 ,10 ,0,
        0, 0, 0 ,-9 ,0 ,0 ,0 ,0,
    };

    std::array<float, 64> data2 = {
        10, 0, 2 ,0 ,4 ,0 ,0 ,0,
        -1, 0, 0 ,0 ,0 ,0 ,0 ,0,
        0, 0, 0 ,0 ,6 ,0 ,0 ,0,
        3, 3, 0 ,0 ,0 ,8 ,0 ,0,
        0, 5, 0 ,-7 ,0 ,0 ,0 ,0,
        0, 0, 0 ,0 ,0 ,-33 ,0 ,0,
        0, 0, 2 ,0 ,0 ,0 ,0 ,0,
        0, 0, 0 ,-9 ,0 ,0 ,0 ,0,
    };

    std::array<float, 64> data3 = {
        -100, 20, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
        0, 0, 0, 0 ,0 ,0 ,0, 0,
    };

    std::vector<Jpeg::Encoder::EncodedBlock> encodedBlocks; 
    std::vector<Jpeg::Encoder::Coefficient> dc;
    std::vector<Jpeg::Encoder::Coefficient> ac;
    int prevDc = 0;
    encodeCoefficients(data, encodedBlocks, dc, ac, prevDc);
    encodeCoefficients(data2, encodedBlocks, dc, ac, prevDc);
    encodeCoefficients(data3, encodedBlocks, dc, ac, prevDc);
    // std::cout << "DC:\n";
    // for (int i = 0; i < dc.size(); i++) {
    //     std::cout << std::hex << static_cast<int>(dc.at(i).encoding) << std::dec << ", " << static_cast<int>(dc.at(i).value) << "\n";
    // }
    //
    // std::cout << "AC:\n";
    // for (int i = 0; i < ac.size(); i++) {
    //     std::cout << std::hex << static_cast<int>(ac.at(i).encoding) << std::dec << ", " << static_cast<int>(ac.at(i).value) << "\n";
    // }


    std::array<uint32_t, 256> dcFrequencies{};
    std::array<uint32_t, 256> acFrequencies{};
    
    // countFrequencies(dc, ac, dcFrequencies, acFrequencies);
    dcFrequencies = {2, 3, 4, 5, 6};
    std::array<uint8_t, 257> dcCodeSizes{};
    std::array<uint8_t, 257> acCodeSizes{};
    Jpeg::Encoder::generateCodeSizes(dcFrequencies, dcCodeSizes);
    Jpeg::Encoder::generateCodeSizes(acFrequencies, acCodeSizes);
    
    std::array<uint8_t, 33> dcCodeFrequencies{};
    std::array<uint8_t, 33> acCodeFrequencies{};
    Jpeg::Encoder::countCodeSizes(dcCodeSizes, dcCodeFrequencies);
    Jpeg::Encoder::countCodeSizes(acCodeSizes, acCodeFrequencies);
    
    std::vector<uint8_t> dcSortedSymbols;
    std::vector<uint8_t> acSortedSymbols;
    Jpeg::Encoder::sortSymbolsByFrequencies(dcFrequencies, dcSortedSymbols);
    Jpeg::Encoder::sortSymbolsByFrequencies(acFrequencies, acSortedSymbols);

    Jpeg::Encoder::writeHuffmanTableNoMarker(0, 0, dcSortedSymbols, dcCodeFrequencies, outfile);
    
    outfile.close();
}

static std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> commands = {
    {"render", Render},
    {"convert", Convert},
    {"test", Test}
};

void Cli::RunCli() {
    while (true) {
        std::string input;
        std::cout << "Enter command: ";
        std::getline(std::cin, input);
        std::vector<std::string> tokens = SplitString(input, ' ');
        if (tokens.empty()) {
            std::cout << "No input given" << '\n';
            continue;
        }

        if (commands.contains(tokens[0])) {
            auto cmd = commands[tokens[0]];
            cmd(tokens);
        } else if (tokens[0] == "exit" || tokens[0] == "quit") {
            break;
        }
        else {
            std::cerr << "Unknown command: " << tokens[0] << '\n';
        }
    }
}
