#pragma once
///  corresponding to  FreeCAD FCConfig.h, please keep the block comment as a reference

// Define EXPORTED for any platform
// #if defined _WIN32 || defined __CYGWIN__
// // to compile this module into dll on windows
// #   if defined(BASE_DLL_EXPORT)
// #       ifdef __GNUC__
// #           define EXPORTED __attribute__((dllexport))
// #       else
// #           define EXPORTED __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
// #       endif
// #   else // to use this dll on windows
// #       ifdef __GNUC__
// #           define EXPORTED __attribute__((dllimport))
// #       else
// #           define EXPORTED __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
// #       endif
// #   endif
// #   define NOT_EXPORTED
// #else // non windows platforms
// #   if __GNUC__ >= 4
// #       define EXPORTED __attribute__((visibility("default")))
// #       define NOT_EXPORTED __attribute__((visibility("hidden")))
// #   else // other compiler, such as CLANG, not yet tested
// #       define EXPORTED
// #       define NOT_EXPORTED
// #   endif
// #endif

#if defined _WIN32 || defined _MSC_VER
#ifdef APP_DLL_EXPORT
#define AppExport __declspec(dllexport)
#else
#define AppExport __declspec(dllimport)
#endif

#ifdef BASE_DLL_EXPORT
#define BaseExport __declspec(dllexport)
#else
#define BaseExport __declspec(dllimport)
#endif

#ifdef GEOM_DLL_EXPORT
#define GeomExport __declspec(dllexport)
#else
#define GeomExport __declspec(dllimport)
#endif

#else // other compilers, such as G++ (tested), CLANG (not yet tested)
#define BaseExport
#define AppExport
#define GeomExport
#endif
