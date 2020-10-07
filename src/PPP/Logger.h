#pragma once

#include "../third-party/nlohmann/json.hpp"

#define LOGLEVEL_PROGRESS 1
#define LOGLEVEL_DEBUG 2


#include "../third-party/loguru/loguru.hpp" // cpp can only be compiled once


namespace loguru
{
    // this macro must be called in the enum declaring namespace
    NLOHMANN_JSON_SERIALIZE_ENUM(NamedVerbosity, {
                                                     {Verbosity_OFF, "OFF"},
                                                     {Verbosity_FATAL, "FATAL"},     /// program will terminate
                                                     {Verbosity_ERROR, "ERROR"},     /// red text print in console
                                                     {Verbosity_WARNING, "WARNING"}, /// yellow text in console
                                                     {Verbosity_INFO, "INFO"},  /// default 0 for loguru, 20 for python
                                                     {Verbosity_1, "PROGRESS"}, /// 1 for loguru, write into log file
                                                     {Verbosity_2, "DEBUG"},    /// 2 for loguru, write into file
                                                 });
} // namespace loguru
