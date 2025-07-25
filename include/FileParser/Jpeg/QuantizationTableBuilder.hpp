#pragma once
#include <iosfwd>

#include "QuantizationTable.hpp"

namespace FileParser::Jpeg {
    class QuantizationTableBuilder {
        // Called AFTER DQT marker
        static auto readFromFile(std::ifstream& file) -> QuantizationTable;
    };
}
