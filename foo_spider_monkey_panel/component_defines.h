#pragma once

#include <commit_hash.h>
#include <version.h>

#define SMP_NAME              "Spider Monkey Panel"
#define SMP_UNDERSCORE_NAME   "foo_spider_monkey_panel"
#define SMP_WINDOW_CLASS_NAME SMP_UNDERSCORE_NAME "_class"
#define SMP_DLL_NAME          SMP_UNDERSCORE_NAME ".dll"

#define SMP_STRINGIFY_LITERAL( x ) #x
#define SMP_STRINGIFY( x )         SMP_STRINGIFY_LITERAL( x )

#ifdef SMP_VERSION_PRERELEASE_TEXT
#    define SMP_VERSION_PRERELEASE "-" SMP_VERSION_PRERELEASE_TEXT
#    define SMP_VERSION_METADATA   "+" SMP_STRINGIFY( SMP_COMMIT_HASH )
#else
#    define SMP_VERSION_PRERELEASE ""
#    define SMP_VERSION_METADATA   ""
#endif

#ifdef _DEBUG
#    define SMP_VERSION_DEBUG_SUFFIX " (Debug)"
#else
#    define SMP_VERSION_DEBUG_SUFFIX ""
#endif

#define SMP_VERSION                                                               \
    SMP_STRINGIFY( SMP_VERSION_MAJOR )                                            \
    "." SMP_STRINGIFY( SMP_VERSION_MINOR ) "." SMP_STRINGIFY( SMP_VERSION_PATCH ) \
        SMP_VERSION_PRERELEASE SMP_VERSION_METADATA
#define SMP_NAME_WITH_VERSION SMP_NAME " v" SMP_VERSION SMP_VERSION_DEBUG_SUFFIX

#define SMP_ABOUT                                                   \
    SMP_NAME_WITH_VERSION " by TheQwertiest\n"                      \
                          "Based on JScript Panel by marc2003\n"    \
                          "Based on WSH Panel Mod by T.P. Wang\n\n" \
                          "Build: " __TIME__ ", " __DATE__ "\n"     \
                          "Columns UI SDK Version: " UI_EXTENSION_VERSION

#ifdef _DEBUG
#    define SMP_ENABLE_CXX_STACKTRACE
#endif
