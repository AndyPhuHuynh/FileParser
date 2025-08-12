#pragma once

#include "FileParser/Utils.hpp"

/**
 * @param varName Name of the variable
 * @param funcCall Call to a function that returns a std::expected
 * @param errorMsg Message to be returned if the expected has an error
 */
#define READ_OR_RETURN(varName, funcCall, errorMsg) \
    const auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return FileParser::utils::getUnexpected(READ_OR_RETURN_##varName, errorMsg); \
    } \
    [[maybe_unused]] const auto& varName = *READ_OR_RETURN_##varName;

/**
 * @param varName Name of the variable
 * @param funcCall Call to a function that returns a std::expected
 * @param errorMsg Message to be returned if the expected has an error
 */
#define READ_OR_RETURN_MUT(varName, funcCall, errorMsg) \
    auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
    return FileParser::utils::getUnexpected(READ_OR_RETURN_##varName, errorMsg); \
    } \
    [[maybe_unused]] auto& varName = *READ_OR_RETURN_##varName;

#define CHECK_VOID_AND_RETURN(funcCall, errorMsg) \
    const auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return FileParser::utils::getUnexpected(READ_OR_RETURN_##varName, errorMsg); \
    } \
