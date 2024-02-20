#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define BCC_VERSION "0.1.1"
#include "./bcc.h"

#define CONFIG_PATH "./src/config.h"

// Stage 2 - Once the config file is generated, include it and compile the program.
#ifdef CONFIGURED
#include CONFIG_PATH

#define TARGET_LINUX 0
#define TARGET_WIN64_MINGW 1
#define TARGET_WIN64_MSVC 2
#define TARGET_MACOS 3

#if BUILD_TARGET == TARGET_LINUX
#include "src/bcc_linux.c"
#elif BUILD_TARGET == TARGET_MACOS
#include "src/bcc_macos.c"
#elif BUILD_TARGET == TARGET_WIN64_MINGW
#include "src/bcc_win64_mingw.c"
#elif BUILD_TARGET == TARGET_WIN64_MSVC
#include "src/bcc_win64_msvc.c"
#endif // BUILD_TARGET

// Extensible logging function
void log_available_subcommands(const char *program, BCC_Log_Level level)
{
    bcc_log(level, "Usage: %s [subcommand]", program);
    bcc_log(level, "Subcommands:");
    bcc_log(level, "    build (default)");
    bcc_log(level, "    dist");
    bcc_log(level, "    svg");
    bcc_log(level, "    help");
}

// Extensible config logging function
void log_config(BCC_Log_Level level)
{
    bcc_log(level, "Build Target: %s", BUILD_TARGET_NAME);
#ifdef BUILD_HOTRELOAD
    bcc_log(level, "Hotreload: ENABLED");
#else
    bcc_log(level, "Hotreload: DISABLED");
#endif // BUILD_HOTRELOAD
}

int main(int argc, char **argv)
{
    bcc_log(BCC_INFO, "Building... ");
    log_config(BCC_INFO);

    // Build function here.
    build_chain(argc, argv);
}

#else // if not configured, generate the config file

void generate_default_config(BCC_String_Builder *content)
{
    bcc_sb_append_cstr(content, "//// Build target.\n");
#ifdef _WIN32
#   if defined(_MSC_VER)
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_LINUX\n");
    bcc_sb_append_cstr(content, "//#define BUILD_TARGET TARGET_WIN64_MINGW\n");
    bcc_sb_append_cstr(content, "#define BUILD_TARGET TARGET_WIN64_MSVC\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_MACOS\n");
#   else
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_LINUX\n");
    bcc_sb_append_cstr(content, "#define BUILD_TARGET TARGET_WIN64_MINGW\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_WIN64_MSVC\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_MACOS\n");
#   endif
#else
#   if defined (__APPLE__) || defined (__MACH__)
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_LINUX\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_WIN64_MINGW\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_WIN64_MSVC\n");
    bcc_sb_append_cstr(content, "#define BUILD_TARGET TARGET_MACOS\n");
#   else
    bcc_sb_append_cstr(content, "#define BUILD_TARGET TARGET_LINUX\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_WIN64_MINGW\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_WIN64_MSVC\n");
    bcc_sb_append_cstr(content, "// #define BUILD_TARGET TARGET_MACOS\n");
#   endif
#endif
    bcc_sb_append_cstr(content, "\n");
    bcc_sb_append_cstr(content, "//// Moves everything in src/plub.c to a separate \"DLL\" so it can be hotreloaded. Works only for Linux right now\n");
    bcc_sb_append_cstr(content, "// #define BUILD_HOTRELOAD\n");
}

// Stage 1
int main(int argc, char **argv) {
    BCC_GO_REBUILD_URSELF(argc, argv);
    bcc_log(BCC_INFO, "Build Config Compile (BCC) Version %s\n", BCC_VERSION);
    bcc_log(BCC_INFO, "Checking build folder...\n");

    if (!bcc_mkdir_if_not_exists("build")) return 1;

    int config_exists = bcc_file_exists(CONFIG_PATH);
    if (config_exists < 0) return 1;
    if (config_exists == 0) {
        // Generate the config file
        bcc_log(BCC_INFO, "Checking for config file %s", CONFIG_PATH);
        BCC_String_Builder content = {0};  // TODO: Read content from file in src.
        generate_default_config(&content);
        if (!bcc_write_entire_file(CONFIG_PATH, content.items, content.count)) return 1;
    } else {
        bcc_log(BCC_INFO, "Config file `%s` exists", CONFIG_PATH);
    }

    BCC_Cmd cmd = {0};
    const char *configured_binary = "build/bcc.configured";
    bcc_cmd_append(&cmd, BCC_REBUILD_URSELF(configured_binary, "bc.c"), "-DCONFIGURED");
    if (!bcc_cmd_run_sync(cmd)) return 1;

    cmd.count = 0;
    bcc_cmd_append(&cmd, configured_binary);
    bcc_da_append_many(&cmd, argv, argc);
    if (!bcc_cmd_run_sync(cmd)) return 1;

    return 0;
}


#endif // CONFIGURED