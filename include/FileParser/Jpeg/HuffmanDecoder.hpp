#pragma once

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Huffman/Table.hpp"

namespace FileParser::Jpeg {
    auto decodeNextValue(BitReader& bitReader, const HuffmanTable& table) -> uint8_t;
}
