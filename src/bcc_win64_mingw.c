#define BUILD_TARGET_NAME "win64_mingw"
#define RAYLIB_VERSION "5.0"

static const char *raylib_modules[] = {
    "rcore",
    "raudio",
    "rglfw",
    "rmodels",
    "rshapes",
    "rtext",
    "rtextures",
    "utils",
};

bool build_program(void)
{
    bool result = true;
    BCC_Cmd cmd = {0};
    BCC_Procs procs = {0};

#ifdef BUILD_HOTRELOAD
#error "TODO: hotreloading is not yet supported."
#else
    cmd.count = 0;
    #ifdef _WIN32
        // On windows, mingw doesn't have the `x86_64-w64-mingw32-` prefix for windres.
        // For gcc, you can use both `x86_64-w64-mingw32-gcc` and just `gcc`
        bcc_cmd_append(&cmd, "windres");
    #else
        bcc_cmd_append(&cmd, "x86_64-w64-mingw32-windres");
    #endif // _WIN32
        bcc_cmd_append(&cmd, "./src/program.rc");
        bcc_cmd_append(&cmd, "-O", "coff");
        bcc_cmd_append(&cmd, "-o", "./build/program.res");
        
    if (!bcc_cmd_run_sync(cmd)) bcc_return_defer(false);
    cmd.count = 0;
    bcc_cmd_append(&cmd, "gcc");
    bcc_cmd_append(&cmd, "-mwindows", "-Wall", "-Wextra", "-ggdb");
    bcc_cmd_append(&cmd, "-I./build/");
    bcc_cmd_append(&cmd, "-I./raylib/raylib-"RAYLIB_VERSION"/src/");
    bcc_cmd_append(&cmd, "-o", "./build/program");
    bcc_cmd_append(&cmd,
        "./src/program.c",
        "./build/program.res"
        );
    bcc_cmd_append(&cmd,
        bcc_temp_sprintf("-L./build/raylib/%s", BUILD_TARGET_NAME),
        "-l:libraylib.a");
    bcc_cmd_append(&cmd, "-lwinmm", "-lgdi32");
    bcc_cmd_append(&cmd, "-static");
    if (!bcc_cmd_run_sync(cmd)) bcc_return_defer(false);
#endif // BUILD_HOTRELOAD

defer:
    bcc_cmd_free(cmd);
    bcc_da_free(procs);
    return result;
}

bool build_raylib()
{
    bool result = true;
    BCC_Cmd cmd = {0};
    BCC_File_Paths object_files = {0};

    if (!bcc_mkdir_if_not_exists("./build/raylib")) {
        bcc_return_defer(false);
    }

    BCC_Procs procs = {0};

    const char *build_path = bcc_temp_sprintf("./build/raylib/%s", BUILD_TARGET_NAME);

    if (!bcc_mkdir_if_not_exists(build_path)) {
        bcc_return_defer(false);
    }

    for (size_t i = 0; i < BCC_ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path = bcc_temp_sprintf("./raylib/raylib-"RAYLIB_VERSION"/src/%s.c", raylib_modules[i]);
        const char *output_path = bcc_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
        output_path = bcc_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        bcc_da_append(&object_files, output_path);

        if (bcc_needs_rebuild(output_path, &input_path, 1)) {
            cmd.count = 0;
            bcc_cmd_append(&cmd, "gcc");
            bcc_cmd_append(&cmd, "-ggdb", "-DPLATFORM_DESKTOP", "-fPIC");
            bcc_cmd_append(&cmd, "-DPLATFORM_DESKTOP");
            bcc_cmd_append(&cmd, "-fPIC");
            bcc_cmd_append(&cmd, "-I./raylib/raylib-"RAYLIB_VERSION"/src/external/glfw/include");
            bcc_cmd_append(&cmd, "-I./raylib/raylib-"RAYLIB_VERSION"/src/external/glfw/deps/mingw");
            bcc_cmd_append(&cmd, "-c", input_path);
            bcc_cmd_append(&cmd, "-o", output_path);

            BCC_Proc proc = bcc_cmd_run_async(cmd);
            bcc_da_append(&procs, proc);
        }
    }
    cmd.count = 0;

    if (!bcc_procs_wait(procs)) bcc_return_defer(false);

#ifndef BUILD_HOTRELOAD
    const char *libraylib_path = bcc_temp_sprintf("%s/libraylib.a", build_path);

    if (bcc_needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        bcc_cmd_append(&cmd, "ar", "-crs", libraylib_path);
        for (size_t i = 0; i < BCC_ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path = bcc_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
            bcc_cmd_append(&cmd, input_path);
        }
        if (!bcc_cmd_run_sync(cmd)) bcc_return_defer(false);
    }
#else
#error "TODO: dynamic raylib is not supported for TARGET_WIN64_MINGW"
#endif // BUILD_HOTRELOAD

defer:
    bcc_cmd_free(cmd);
    bcc_da_free(object_files);
    return result;
}

bool build_chain(int argc, char **argv)
{
    build_raylib();
    build_program();

#ifndef BUILD_HOTRELOAD
    BCC_Cmd cmd = {0};
    const char *program_binary = "build/program.exe";
    bcc_cmd_append(&cmd, program_binary);
    if (!bcc_cmd_run_sync(cmd)) return 1;
#endif
}