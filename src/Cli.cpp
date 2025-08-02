#include "FileParser/Cli.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

#include "FileParser/Bmp/BmpImage.h"
#include "FileParser/Bmp/BmpJpegConverter.h"
#include "FileParser/Bmp/BmpRenderer.h"
#include "FileParser/FileUtil.h"
#include "FileParser/Jpeg/Decoder.hpp"
#include "FileParser/Jpeg/JpegBmpConverter.h"
#include "FileParser/Jpeg/JpegEncoder.h"
#include "FileParser/Jpeg/JpegImage.h"
#include "FileParser/Jpeg/JpegRenderer.h"

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
    using namespace FileUtils;
    using namespace FileParser;
    if (args.size() != 2) {
        std::cerr << "Usage: render <filename>\n";
        // return;
    }
    
    const std::string& filepath = args[1];
    
    if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath)) {
        std::cout << "The file path is valid and points to a file: " << filepath << '\n';
    } else {
        std::cerr << "Error: The file path is invalid or does not point to a regular file: " << filepath << '\n';
        return;
    }

    switch (getFileType(filepath)) {
        case FileType::Bmp: {
            Bmp::BmpImage bmp(filepath);
            Bmp::Renderer::renderBmp(bmp);
            break;
        }
        case FileType::Jpeg: {
            // Jpeg::JpegImage jpeg(filepath);
            if (args.size() > 2 && args[2] == "new") {
                uint16_t width, height;
                const auto& mcus = Jpeg::JpegDecoder::decode(filepath, &width, &height);
                if (!mcus) {
                    std::cerr << "Error: Failed to decode Jpeg file: " << filepath << ": " << mcus.error() << '\n';
                    return;
                }

                Jpeg::Renderer::renderJpeg(*mcus, width, height);
            } else {
                Jpeg::JpegImage jpeg(filepath);
                Jpeg::Renderer::renderJpeg(jpeg);
            }
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
    using namespace FileUtils;
    using namespace FileParser;

    if (args.size() != 3 && args.size() != 4) {
        std::cerr << "Usage: convert <filename> <new format> [<new filepath>] (default: <filename>.<new format>)\n";
        return;
    }

    const std::string& filepath = args[1];
    const std::string& formatStr = args[2];
    const FileType newFormat = stringToFileType(formatStr);
    const std::string& newFilepath = args.size() < 4? filepath + "." + formatStr : args[3];
    
    switch (getFileType(filepath)) {
        case FileType::Bmp: {
            Bmp::BmpImage bmp(filepath);
            switch (newFormat) {
                case FileType::Bmp: {
                    break;
                }
                case FileType::Jpeg: {
                    Jpeg::Encoder::EncodingSettings settings;
                    settings.luminanceQuality = 100;
                    settings.chrominanceQuality = 100;
                    settings.optimizeHuffmanTables = true;
                    Bmp::Converter::writeBmpAsJpeg(bmp, newFilepath, settings);
                    break;
                }
                case FileType::None: {
                    std::cout << "Unknown file type: " << newFilepath << '\n';
                    break;
                }
            }
            break;
        }
        case FileType::Jpeg: {
            Jpeg::JpegImage jpeg(filepath);
            switch (newFormat) {
                case FileType::Bmp: {
                    Jpeg::Converter::writeJpegAsBmp(jpeg, newFilepath);
                    break;
                }
                case FileType::Jpeg: {
                    break;
                }
                case FileType::None: {
                    std::cout << "Unknown file type: " << newFilepath << '\n';
                    break;
                }
            }
            break;
        }
        case FileType::None:
            std::cout << "Original file type not supported\n";
            break;
    }
}

void printComponent(const std::array<float, 64>& component) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << std::round(component[i * 8 + j]) << " ";
        }
        std::cout << '\n';
    }
}

static void Test(const std::vector<std::string>& args) {
    using namespace FileParser;
    using namespace Jpeg;
    using namespace Jpeg::Encoder;

    const std::string& filepath = args[1];
    const auto result = JpegParser::parseFile(filepath);
    if (!result) {
        std::cerr << "Error: Failed to parse file: " << filepath << " " << result.error() << '\n';
    }
}

static std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> commands = {
    {"render", Render},
    {"convert", Convert},
    {"test", Test}
};

void Cli::RunCli() {
    while (true) {
        std::string input;
        std::filesystem::path cwd = std::filesystem::current_path();
        std::cout << "[" << cwd.string() << "]" << " FileParser> ";
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
