#pragma once

#include <expected>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Jpeg/QuantizationTable.hpp"

namespace FileParser::Jpeg {
    class QuantizationTableBuilder {
    public:
        // Called AFTER DQT
        static auto readFromBitReader(BitReader& bytes) -> std::expected<QuantizationTable, std::string>;
    };
}
