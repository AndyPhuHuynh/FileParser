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

    BitWriter b("bitWriters.test");
    b.flushByte();
    
    b.writeZero();
    b.writeOne();
    b.writeZero();
    b.writeOne();
    b.writeZero();
    b.writeOne();
    b.writeZero();
    b.writeOne();

    b.writeBit(false);
    b.writeBit(true);
    b.writeBit(false);
    b.writeBit(true);
    b.writeBit(false);
    b.writeBit(true);
    b.writeBit(false);
    b.writeBit(true);

    int v = 255;
    b.writeValue(v);
    b.writeZero();
    b.writeBit(true);
    b.writeBit(false);
    b.flushByte();

    int w = 14;
    b.writeBits(w, 4);
    b.flushByte();
    b.flushBuffer();
    
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
