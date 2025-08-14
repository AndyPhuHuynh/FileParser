#pragma once
#include "FileParser/BitManipulationUtil.h"

class JpegBitWriter final : public BitWriter{
public:
    explicit JpegBitWriter(const std::string& filepath, size_t bufferSize = 4096) : BitWriter(filepath, bufferSize) {}
    
    void setByteStuffing(const bool byteStuffing) {
        m_byteStuffing = byteStuffing;
    }
    
    void flushByte(const bool padWithOnes = false) override {
        BitWriter::flushByte(padWithOnes);
        if (m_byteStuffing && m_prevByte == 0xFF) {
            BitWriter::flushByte(false);
        }
    }
private:
    bool m_byteStuffing = false;    
};
