#pragma once


/// recently gcc, msvc all support __has_include()
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#if __has_include(<experimental/filesystem>)
// GCC 7.3 has only experimental support, 8.0 has it in std namespace
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
/// However, boost::filesystem does not have path::u8string() function!
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif
#endif
