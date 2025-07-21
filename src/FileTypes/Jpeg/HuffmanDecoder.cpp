#include "Jpeg/HuffmanDecoder.hpp"

#include <iostream>

auto FileParser::Jpeg::decodeNextValue(BitReader& bitReader, const HuffmanTable& table) -> uint8_t {
    auto [bitLength, value] = table.decode(bitReader.getWordConstant());
    bitReader.skipBits(bitLength);
    return value;
}
