#pragma once

#include "FileParser/Utils.hpp"

/**
 * @param varName Name of the variable
 * @param funcCall Call to a function that returns a std::expected
 * @param errorMsg Message to be returned if the expected has an error
 */
#define ASSIGN_OR_RETURN(varName, funcCall, errorMsg) \
    const auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return FileParser::utils::getUnexpected(READ_OR_RETURN_##varName, errorMsg); \
    } \
    [[maybe_unused]] const auto& varName = *READ_OR_RETURN_##varName

/**
 * @param varName Name of the variable
 * @param funcCall Call to a function that returns a std::expected
 * @param errorMsg Message to be returned if the expected has an error
 */
#define ASSIGN_OR_PROPAGATE(varName, funcCall) \
    const auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return std::unexpected(READ_OR_RETURN_##varName.error()); \
    } \
    [[maybe_unused]] const auto& varName = *READ_OR_RETURN_##varName

/**
 * @param varName Name of the variable
 * @param funcCall Call to a function that returns a std::expected
 * @param errorMsg Message to be returned if the expected has an error
 */
#define ASSIGN_OR_RETURN_MUT(varName, funcCall, errorMsg) \
    auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return FileParser::utils::getUnexpected(READ_OR_RETURN_##varName, errorMsg); \
    } \
    [[maybe_unused]] auto& varName = *READ_OR_RETURN_##varName

/**
 * @param varName Name of the variable
 * @param funcCall Call to a function that returns a std::expected
 * @param errorMsg Message to be returned if the expected has an error
 */
#define ASSIGN_OR_PROPAGATE_MUT(varName, funcCall) \
    auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return std::unexpected(READ_OR_RETURN_##varName.error()); \
    } \
    [[maybe_unused]] auto& varName = *READ_OR_RETURN_##varName

#define CHECK_VOID_AND_RETURN(funcCall, errorMsg) \
    const auto READ_OR_RETURN_##varName = funcCall; \
    if (!READ_OR_RETURN_##varName) { \
        return FileParser::utils::getUnexpected(READ_OR_RETURN_##varName, errorMsg); \
    }

#define CHECK_VOID_OR_PROPAGATE(funcCall) \
    if (const auto CHECK_VOID_var = funcCall; !CHECK_VOID_var) { \
        return std::unexpected(CHECK_VOID_var.error()); \
    }
