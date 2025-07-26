#pragma once

#include <expected>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Jpeg/QuantizationTable.hpp"

namespace FileParser::Jpeg {
    /**
     * @class QuantizationTableBuilder
     * @breif Provides functionality for parsing JPEG quantization tables (DQT segments).
     *
     * This class is responsible for interpreting the bytes of a JPEG DQT segment and construct a corresponding
     * QuantizationTable object.
     */
    class QuantizationTableBuilder {
    public:
        /**
         * @brief Parses the bytes of a JPEG DQT (Define Quantization Table) segment, starting after the DQT marker and
         *        length, consuming data from the bit reader.
         *
         * @param bytes A BitReader containing all the bytes from the DQT segment.
         * @return A @code std::expected<QuantizationTable, std::string>@endcode containing the parsed quantization table
         *         on success, or an error message on failure.
         */
        static auto readFromBitReader(BitReader& bytes) -> std::expected<QuantizationTable, std::string>;
    };
}
