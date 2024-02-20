/*
    Build Config Compile (BCC) Version 0.0.1

    The Build Config Compile project is designed to revolutionize the C project compilation process
    by eliminating the dependency on traditional build systems and scripting environments.
    Our goal is to create a self-contained, efficient, and streamlined build system
    that requires nothing more than a C compiler to manage the entire build process for C projects.
    This approach simplifies the build process, enhances portability across different platforms.
*/

#ifndef BCC_H_
#define BCC_H_

#define BCC_ASSERT assert
#define BCC_REALLOC realloc
#define BCC_FREE free

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define _WINUSER_
#    define _WINGDI_
#    define _IMM_
#    define _WINCON_
#    include <windows.h>
#    include <direct.h>
#    include <shellapi.h>
#else
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    include <fcntl.h>
#endif

#ifdef _WIN32
#    define BCC_LINE_END "\r\n"
#else
#    define BCC_LINE_END "\n"
#endif

#define BCC_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define BCC_ARRAY_GET(array, index) \
    (BCC_ASSERT(index >= 0), BCC_ASSERT(index < BCC_ARRAY_LEN(array)), array[index])

typedef enum {
    BCC_INFO,
    BCC_WARNING,
    BCC_ERROR,
} BCC_Log_Level;

void bcc_log(BCC_Log_Level level, const char *fmt, ...);

// It is an equivalent of shift command from bash. It basically pops a command line
// argument from the beginning.
char *bcc_shift_args(int *argc, char ***argv);

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} BCC_File_Paths;

typedef enum {
    BCC_FILE_REGULAR = 0,
    BCC_FILE_DIRECTORY,
    BCC_FILE_SYMLINK,
    BCC_FILE_OTHER,
} BCC_File_Type;

bool bcc_mkdir_if_not_exists(const char *path);
bool bcc_copy_file(const char *src_path, const char *dst_path);
bool bcc_copy_directory_recursively(const char *src_path, const char *dst_path);
bool bcc_read_entire_dir(const char *parent, BCC_File_Paths *children);
bool bcc_write_entire_file(const char *path, const void *data, size_t size);
BCC_File_Type bcc_get_file_type(const char *path);

#define bcc_return_defer(value) do { result = (value); goto defer; } while(0)

// Initial capacity of a dynamic array
#define BCC_DA_INIT_CAP 256

// Append an item to a dynamic array
#define bcc_da_append(da, item)                                                          \
    do {                                                                                 \
        if ((da)->count >= (da)->capacity) {                                             \
            (da)->capacity = (da)->capacity == 0 ? BCC_DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = BCC_REALLOC((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            BCC_ASSERT((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                                \
                                                                                         \
        (da)->items[(da)->count++] = (item);                                             \
    } while (0)

#define bcc_da_free(da) BCC_FREE((da).items)

// Append several items to a dynamic array
#define bcc_da_append_many(da, new_items, new_items_count)                                  \
    do {                                                                                    \
        if ((da)->count + new_items_count > (da)->capacity) {                               \
            if ((da)->capacity == 0) {                                                      \
                (da)->capacity = BCC_DA_INIT_CAP;                                           \
            }                                                                               \
            while ((da)->count + new_items_count > (da)->capacity) {                        \
                (da)->capacity *= 2;                                                        \
            }                                                                               \
            (da)->items = BCC_REALLOC((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            BCC_ASSERT((da)->items != NULL && "Buy more RAM lol");                          \
        }                                                                                   \
        memcpy((da)->items + (da)->count, new_items, new_items_count*sizeof(*(da)->items)); \
        (da)->count += new_items_count;                                                     \
    } while (0)

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} BCC_String_Builder;

bool bcc_read_entire_file(const char *path, BCC_String_Builder *sb);

// Append a sized buffer to a string builder
#define bcc_sb_append_buf(sb, buf, size) bcc_da_append_many(sb, buf, size)

// Append a NULL-terminated string to a string builder
#define bcc_sb_append_cstr(sb, cstr)  \
    do {                              \
        const char *s = (cstr);       \
        size_t n = strlen(s);         \
        bcc_da_append_many(sb, s, n); \
    } while (0)

// Append a single NULL character at the end of a string builder. So then you can
// use it a NULL-terminated C string
#define bcc_sb_append_null(sb) bcc_da_append_many(sb, "", 1)

// Free the memory allocated by a string builder
#define bcc_sb_free(sb) BCC_FREE((sb).items)

// Process handle
#ifdef _WIN32
typedef HANDLE BCC_Proc;
#define BCC_INVALID_PROC INVALID_HANDLE_VALUE
#else
typedef int BCC_Proc;
#define BCC_INVALID_PROC (-1)
#endif // _WIN32

typedef struct {
    BCC_Proc *items;
    size_t count;
    size_t capacity;
} BCC_Procs;

bool bcc_procs_wait(BCC_Procs procs);

// Wait until the process has finished
bool bcc_proc_wait(BCC_Proc proc);

// A command - the main workhorse of BCC. BCC is all about building commands an running them
typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} BCC_Cmd;

// Render a string representation of a command into a string builder. Keep in mind the the
// string builder is not NULL-terminated by default. Use bcc_sb_append_null if you plan to
// use it as a C string.
void bcc_cmd_render(BCC_Cmd cmd, BCC_String_Builder *render);

#define bcc_cmd_append(cmd, ...) \
    bcc_da_append_many(cmd, ((const char*[]){__VA_ARGS__}), (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

// Free all the memory allocated by command arguments
#define bcc_cmd_free(cmd) BCC_FREE(cmd.items)

// Run command asynchronously
BCC_Proc bcc_cmd_run_async(BCC_Cmd cmd);

// Run command synchronously
bool bcc_cmd_run_sync(BCC_Cmd cmd);

#ifndef BCC_TEMP_CAPACITY
#define BCC_TEMP_CAPACITY (8*1024*1024)
#endif // BCC_TEMP_CAPACITY
char *bcc_temp_strdup(const char *cstr);
void *bcc_temp_alloc(size_t size);
char *bcc_temp_sprintf(const char *format, ...);
void bcc_temp_reset(void);
size_t bcc_temp_save(void);
void bcc_temp_rewind(size_t checkpoint);

int is_path1_modified_after_path2(const char *path1, const char *path2);
bool bcc_rename(const char *old_path, const char *new_path);
int bcc_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
int bcc_needs_rebuild1(const char *output_path, const char *input_path);
int bcc_file_exists(const char *file_path);

// TODO: add MinGW support for Go Rebuild Urself™ Technology
#ifndef BCC_REBUILD_URSELF
#  if _WIN32
#    if defined(__GNUC__)
#       define BCC_REBUILD_URSELF(binary_path, source_path) "gcc", "-o", binary_path, source_path
#    elif defined(__clang__)
#       define BCC_REBUILD_URSELF(binary_path, source_path) "clang", "-o", binary_path, source_path
#    elif defined(_MSC_VER)
#       define BCC_REBUILD_URSELF(binary_path, source_path) "cl.exe", bcc_temp_sprintf("/Fe:%s", (binary_path)), source_path
#    endif
#  else
#    define BCC_REBUILD_URSELF(binary_path, source_path) "cc", "-o", binary_path, source_path
#  endif
#endif

// Go Rebuild Urself™ Technology
//
//   How to use it:
//     int main(int argc, char** argv) {
//         GO_REBUILD_URSELF(argc, argv);
//         // actual work
//         return 0;
//     }
//
//   After your added this macro every time you run ./bcc it will detect
//   that you modified its original source code and will try to rebuild itself
//   before doing any actual work. So you only need to bootstrap your build system
//   once.
//
//   The modification is detected by comparing the last modified times of the executable
//   and its source code. The same way the make utility usually does it.
//
//   The rebuilding is done by using the REBUILD_URSELF macro which you can redefine
//   if you need a special way of bootstraping your build system. (which I personally
//   do not recommend since the whole idea of bcc is to keep the process of bootstrapping
//   as simple as possible and doing all of the actual work inside of the bcc)
//
#define BCC_GO_REBUILD_URSELF(argc, argv)                                                    \
    do {                                                                                     \
        const char *source_path = __FILE__;                                                  \
        assert(argc >= 1);                                                                   \
        const char *binary_path = argv[0];                                                   \
                                                                                             \
        int rebuild_is_needed = bcc_needs_rebuild(binary_path, &source_path, 1);             \
        if (rebuild_is_needed < 0) exit(1);                                                  \
        if (rebuild_is_needed) {                                                             \
            BCC_String_Builder sb = {0};                                                     \
            bcc_sb_append_cstr(&sb, binary_path);                                            \
            bcc_sb_append_cstr(&sb, ".old");                                                 \
            bcc_sb_append_null(&sb);                                                         \
                                                                                             \
            if (!bcc_rename(binary_path, sb.items)) exit(1);                                 \
            BCC_Cmd rebuild = {0};                                                           \
            bcc_cmd_append(&rebuild, BCC_REBUILD_URSELF(binary_path, source_path));          \
            bool rebuild_succeeded = bcc_cmd_run_sync(rebuild);                              \
            bcc_cmd_free(rebuild);                                                           \
            if (!rebuild_succeeded) {                                                        \
                bcc_rename(sb.items, binary_path);                                           \
                exit(1);                                                                     \
            }                                                                                \
                                                                                             \
            BCC_Cmd cmd = {0};                                                               \
            bcc_da_append_many(&cmd, argv, argc);                                            \
            if (!bcc_cmd_run_sync(cmd)) exit(1);                                             \
            exit(0);                                                                         \
        }                                                                                    \
    } while(0)
// The implementation idea is stolen from https://github.com/zhiayang/nabs

typedef struct {
    size_t count;
    const char *data;
} BCC_String_View;

const char *bcc_temp_sv_to_cstr(BCC_String_View sv);

BCC_String_View bcc_sv_chop_by_delim(BCC_String_View *sv, char delim);
BCC_String_View bcc_sv_trim(BCC_String_View sv);
bool bcc_sv_eq(BCC_String_View a, BCC_String_View b);
BCC_String_View bcc_sv_from_cstr(const char *cstr);
BCC_String_View bcc_sv_from_parts(const char *data, size_t count);

// printf macros for String_View
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif // SV_Fmt
#ifndef SV_Arg
#define SV_Arg(sv) (int) (sv).count, (sv).data
#endif // SV_Arg
// USAGE:
//   String_View name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));


// minirent.h HEADER BEGIN ////////////////////////////////////////
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================
//
// minirent — 0.0.1 — A subset of dirent interface for Windows.
//
// https://github.com/tsoding/minirent
//
// ============================================================
//
// ChangeLog (https://semver.org/ is implied)
//
//    0.0.2 Automatically include dirent.h on non-Windows
//          platforms
//    0.0.1 First Official Release

#ifndef _WIN32
#include <dirent.h>
#else // _WIN32

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

struct dirent
{
    char d_name[MAX_PATH+1];
};

typedef struct DIR DIR;

static DIR *opendir(const char *dirpath);
static struct dirent *readdir(DIR *dirp);
static int closedir(DIR *dirp);
#endif // _WIN32
// minirent.h HEADER END ////////////////////////////////////////

#endif // BCC_H_

#ifdef BCC_VERSION

static size_t bcc_temp_size = 0;
static char bcc_temp[BCC_TEMP_CAPACITY] = {0};

bool bcc_mkdir_if_not_exists(const char *path)
{
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        if (errno == EEXIST) {
            bcc_log(BCC_INFO, "directory `%s` already exists", path);
            return true;
        }
        bcc_log(BCC_ERROR, "could not create directory `%s`: %s", path, strerror(errno));
        return false;
    }

    bcc_log(BCC_INFO, "created directory `%s`", path);
    return true;
}

bool bcc_copy_file(const char *src_path, const char *dst_path)
{
    bcc_log(BCC_INFO, "copying %s -> %s", src_path, dst_path);
#ifdef _WIN32
    if (!CopyFile(src_path, dst_path, FALSE)) {
        bcc_log(BCC_ERROR, "Could not copy file: %lu", GetLastError());
        return false;
    }
    return true;
#else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32*1024;
    char *buf = BCC_REALLOC(NULL, buf_size);
    BCC_ASSERT(buf != NULL && "Buy more RAM lol!!");
    bool result = true;

    src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        bcc_log(BCC_ERROR, "Could not open file %s: %s", src_path, strerror(errno));
        bcc_return_defer(false);
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        bcc_log(BCC_ERROR, "Could not get mode of file %s: %s", src_path, strerror(errno));
        bcc_return_defer(false);
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        bcc_log(BCC_ERROR, "Could not create file %s: %s", dst_path, strerror(errno));
        bcc_return_defer(false);
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) break;
        if (n < 0) {
            bcc_log(BCC_ERROR, "Could not read from file %s: %s", src_path, strerror(errno));
            bcc_return_defer(false);
        }
        char *buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                bcc_log(BCC_ERROR, "Could not write to file %s: %s", dst_path, strerror(errno));
                bcc_return_defer(false);
            }
            n    -= m;
            buf2 += m;
        }
    }

defer:
    free(buf);
    close(src_fd);
    close(dst_fd);
    return result;
#endif
}

void bcc_cmd_render(BCC_Cmd cmd, BCC_String_Builder *render)
{
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.items[i];
        if (arg == NULL) break;
        if (i > 0) bcc_sb_append_cstr(render, " ");
        if (!strchr(arg, ' ')) {
            bcc_sb_append_cstr(render, arg);
        } else {
            bcc_da_append(render, '\'');
            bcc_sb_append_cstr(render, arg);
            bcc_da_append(render, '\'');
        }
    }
}

BCC_Proc bcc_cmd_run_async(BCC_Cmd cmd)
{
    if (cmd.count < 1) {
        bcc_log(BCC_ERROR, "Could not run empty command");
        return BCC_INVALID_PROC;
    }

    BCC_String_Builder sb = {0};
    bcc_cmd_render(cmd, &sb);
    bcc_sb_append_null(&sb);
    bcc_log(BCC_INFO, "CMD: %s", sb.items);
    bcc_sb_free(sb);
    memset(&sb, 0, sizeof(sb));

#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // TODO: use a more reliable rendering of the command instead of cmd_render
    // cmd_render is for logging primarily
    bcc_cmd_render(cmd, &sb);
    bcc_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    bcc_sb_free(sb);

    if (!bSuccess) {
        bcc_log(BCC_ERROR, "Could not create child process: %lu", GetLastError());
        return BCC_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        bcc_log(BCC_ERROR, "Could not fork child process: %s", strerror(errno));
        return BCC_INVALID_PROC;
    }

    if (cpid == 0) {
        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        BCC_Cmd cmd_null = {0};
        bcc_da_append_many(&cmd_null, cmd.items, cmd.count);
        bcc_cmd_append(&cmd_null, NULL);

        if (execvp(cmd.items[0], (char * const*) cmd_null.items) < 0) {
            bcc_log(BCC_ERROR, "Could not exec child process: %s", strerror(errno));
            exit(1);
        }
        BCC_ASSERT(0 && "unreachable");
    }

    return cpid;
#endif
}

bool bcc_procs_wait(BCC_Procs procs)
{
    bool success = true;
    for (size_t i = 0; i < procs.count; ++i) {
        success = bcc_proc_wait(procs.items[i]) && success;
    }
    return success;
}

bool bcc_proc_wait(BCC_Proc proc)
{
    if (proc == BCC_INVALID_PROC) return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(
                       proc,    // HANDLE hHandle,
                       INFINITE // DWORD  dwMilliseconds
                   );

    if (result == WAIT_FAILED) {
        bcc_log(BCC_ERROR, "could not wait on child process: %lu", GetLastError());
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        bcc_log(BCC_ERROR, "could not get process exit code: %lu", GetLastError());
        return false;
    }

    if (exit_status != 0) {
        bcc_log(BCC_ERROR, "command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(proc);

    return true;
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            bcc_log(BCC_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                bcc_log(BCC_ERROR, "command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            bcc_log(BCC_ERROR, "command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }

    return true;
#endif
}

bool bcc_cmd_run_sync(BCC_Cmd cmd)
{
    BCC_Proc p = bcc_cmd_run_async(cmd);
    if (p == BCC_INVALID_PROC) return false;
    return bcc_proc_wait(p);
}
char *bcc_shift_args(int *argc, char ***argv)
{
    BCC_ASSERT(*argc > 0);
    char *result = **argv;
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}


void bcc_log(BCC_Log_Level level, const char *fmt, ...)
{
    switch (level) {
    case BCC_INFO:
        fprintf(stderr, "[INFO] ");
        break;
    case BCC_WARNING:
        fprintf(stderr, "[WARNING] ");
        break;
    case BCC_ERROR:
        fprintf(stderr, "[ERROR] ");
        break;
    default:
        BCC_ASSERT(0 && "unreachable");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

bool bcc_read_entire_dir(const char *parent, BCC_File_Paths *children)
{
    bool result = true;
    DIR *dir = NULL;

    dir = opendir(parent);
    if (dir == NULL) {
        bcc_log(BCC_ERROR, "Could not open directory %s: %s", parent, strerror(errno));
        bcc_return_defer(false);
    }

    errno = 0;
    struct dirent *ent = readdir(dir);
    while (ent != NULL) {
        bcc_da_append(children, bcc_temp_strdup(ent->d_name));
        ent = readdir(dir);
    }

    if (errno != 0) {
        bcc_log(BCC_ERROR, "Could not read directory %s: %s", parent, strerror(errno));
        bcc_return_defer(false);
    }

defer:
    if (dir) closedir(dir);
    return result;
}

bool bcc_write_entire_file(const char *path, const void *data, size_t size)
{
    bool result = true;

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        bcc_log(BCC_ERROR, "Could not open file %s for writing: %s\n", path, strerror(errno));
        bcc_return_defer(false);
    }

    //           len
    //           v
    // aaaaaaaaaa
    //     ^
    //     data

    const char *buf = data;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (ferror(f)) {
            bcc_log(BCC_ERROR, "Could not write into file %s: %s\n", path, strerror(errno));
            bcc_return_defer(false);
        }
        size -= n;
        buf  += n;
    }

defer:
    if (f) fclose(f);
    return result;
}

BCC_File_Type bcc_get_file_type(const char *path)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        bcc_log(BCC_ERROR, "Could not get file attributes of %s: %lu", path, GetLastError());
        return -1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) return BCC_FILE_DIRECTORY;
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return BCC_FILE_REGULAR;
#else // _WIN32
    struct stat statbuf;
    if (stat(path, &statbuf) < 0) {
        bcc_log(BCC_ERROR, "Could not get stat of %s: %s", path, strerror(errno));
        return -1;
    }

    switch (statbuf.st_mode & S_IFMT) {
        case S_IFDIR:  return BCC_FILE_DIRECTORY;
        case S_IFREG:  return BCC_FILE_REGULAR;
        case S_IFLNK:  return BCC_FILE_SYMLINK;
        default:       return BCC_FILE_OTHER;
    }
#endif // _WIN32
}

bool bcc_copy_directory_recursively(const char *src_path, const char *dst_path)
{
    bool result = true;
    BCC_File_Paths children = {0};
    BCC_String_Builder src_sb = {0};
    BCC_String_Builder dst_sb = {0};
    size_t temp_checkpoint = bcc_temp_save();

    BCC_File_Type type = bcc_get_file_type(src_path);
    if (type < 0) return false;

    switch (type) {
        case BCC_FILE_DIRECTORY: {
            if (!bcc_mkdir_if_not_exists(dst_path)) bcc_return_defer(false);
            if (!bcc_read_entire_dir(src_path, &children)) bcc_return_defer(false);

            for (size_t i = 0; i < children.count; ++i) {
                if (strcmp(children.items[i], ".") == 0) continue;
                if (strcmp(children.items[i], "..") == 0) continue;

                src_sb.count = 0;
                bcc_sb_append_cstr(&src_sb, src_path);
                bcc_sb_append_cstr(&src_sb, "/");
                bcc_sb_append_cstr(&src_sb, children.items[i]);
                bcc_sb_append_null(&src_sb);

                dst_sb.count = 0;
                bcc_sb_append_cstr(&dst_sb, dst_path);
                bcc_sb_append_cstr(&dst_sb, "/");
                bcc_sb_append_cstr(&dst_sb, children.items[i]);
                bcc_sb_append_null(&dst_sb);

                if (!bcc_copy_directory_recursively(src_sb.items, dst_sb.items)) {
                    bcc_return_defer(false);
                }
            }
        } break;

        case BCC_FILE_REGULAR: {
            if (!bcc_copy_file(src_path, dst_path)) {
                bcc_return_defer(false);
            }
        } break;

        case BCC_FILE_SYMLINK: {
            bcc_log(BCC_WARNING, "TODO: Copying symlinks is not supported yet");
        } break;

        case BCC_FILE_OTHER: {
            bcc_log(BCC_ERROR, "Unsupported type of file %s", src_path);
            bcc_return_defer(false);
        } break;

        default: BCC_ASSERT(0 && "unreachable");
    }

defer:
    bcc_temp_rewind(temp_checkpoint);
    bcc_da_free(src_sb);
    bcc_da_free(dst_sb);
    bcc_da_free(children);
    return result;
}

char *bcc_temp_strdup(const char *cstr)
{
    size_t n = strlen(cstr);
    char *result = bcc_temp_alloc(n + 1);
    BCC_ASSERT(result != NULL && "Increase BCC_TEMP_CAPACITY");
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

void *bcc_temp_alloc(size_t size)
{
    if (bcc_temp_size + size > BCC_TEMP_CAPACITY) return NULL;
    void *result = &bcc_temp[bcc_temp_size];
    bcc_temp_size += size;
    return result;
}

char *bcc_temp_sprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);
    BCC_ASSERT(n >= 0);
    char *result = bcc_temp_alloc(n + 1);
    BCC_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    // TODO: use proper arenas for the temporary allocator;
    va_start(args, format);
    vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

void bcc_temp_reset(void)
{
    bcc_temp_size = 0;
}

size_t bcc_temp_save(void)
{
    return bcc_temp_size;
}

void bcc_temp_rewind(size_t checkpoint)
{
    bcc_temp_size = checkpoint;
}

const char *bcc_temp_sv_to_cstr(BCC_String_View sv)
{
    char *result = bcc_temp_alloc(sv.count + 1);
    BCC_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

int bcc_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count)
{
#ifdef _WIN32
    BOOL bSuccess;

    HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        bcc_log(BCC_ERROR, "Could not open file %s: %lu", output_path, GetLastError());
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) {
        bcc_log(BCC_ERROR, "Could not get time of %s: %lu", output_path, GetLastError());
        return -1;
    }

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            bcc_log(BCC_ERROR, "Could not open file %s: %lu", input_path, GetLastError());
            return -1;
        }
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) {
            bcc_log(BCC_ERROR, "Could not get time of %s: %lu", input_path, GetLastError());
            return -1;
        }

        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT) return 1;
        bcc_log(BCC_ERROR, "could not stat %s: %s", output_path, strerror(errno));
        return -1;
    }
    int output_path_time = statbuf.st_mtime;

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        if (stat(input_path, &statbuf) < 0) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            bcc_log(BCC_ERROR, "could not stat %s: %s", input_path, strerror(errno));
            return -1;
        }
        int input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (input_path_time > output_path_time) return 1;
    }

    return 0;
#endif
}

int bcc_needs_rebuild1(const char *output_path, const char *input_path)
{
    return bcc_needs_rebuild(output_path, &input_path, 1);
}

bool bcc_rename(const char *old_path, const char *new_path)
{
    bcc_log(BCC_INFO, "renaming %s -> %s", old_path, new_path);
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        bcc_log(BCC_ERROR, "could not rename %s to %s: %lu", old_path, new_path, GetLastError());
        return false;
    }
#else
    if (rename(old_path, new_path) < 0) {
        bcc_log(BCC_ERROR, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
        return false;
    }
#endif // _WIN32
    return true;
}

bool bcc_read_entire_file(const char *path, BCC_String_Builder *sb)
{
    bool result = true;

    FILE *f = fopen(path, "rb");
    if (f == NULL)                 bcc_return_defer(false);
    if (fseek(f, 0, SEEK_END) < 0) bcc_return_defer(false);
    long m = ftell(f);
    if (m < 0)                     bcc_return_defer(false);
    if (fseek(f, 0, SEEK_SET) < 0) bcc_return_defer(false);

    size_t new_count = sb->count + m;
    if (new_count > sb->capacity) {
        sb->items = realloc(sb->items, new_count);
        BCC_ASSERT(sb->items != NULL && "Buy more RAM lool!!");
        sb->capacity = new_count;
    }

    fread(sb->items + sb->count, m, 1, f);
    if (ferror(f)) {
        // TODO: Afaik, ferror does not set errno. So the error reporting in defer is not correct in this case.
        bcc_return_defer(false);
    }
    sb->count = new_count;

defer:
    if (!result) bcc_log(BCC_ERROR, "Could not read file %s: %s", path, strerror(errno));
    if (f) fclose(f);
    return result;
}

BCC_String_View bcc_sv_chop_by_delim(BCC_String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    BCC_String_View result = bcc_sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

BCC_String_View bcc_sv_from_parts(const char *data, size_t count)
{
    BCC_String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

BCC_String_View bcc_sv_trim_left(BCC_String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return bcc_sv_from_parts(sv.data + i, sv.count - i);
}

BCC_String_View bcc_sv_trim_right(BCC_String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return bcc_sv_from_parts(sv.data, sv.count - i);
}

BCC_String_View bcc_sv_trim(BCC_String_View sv)
{
    return bcc_sv_trim_right(bcc_sv_trim_left(sv));
}

BCC_String_View bcc_sv_from_cstr(const char *cstr)
{
    return bcc_sv_from_parts(cstr, strlen(cstr));
}

bool bcc_sv_eq(BCC_String_View a, BCC_String_View b)
{
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

// RETURNS:
//  0 - file does not exists
//  1 - file exists
// -1 - error while checking if file exists. The error is logged
int bcc_file_exists(const char *file_path)
{
#if _WIN32
    // TODO: distinguish between "does not exists" and other errors
    DWORD dwAttrib = GetFileAttributesA(file_path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
#else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT) return 0;
        bcc_log(BCC_ERROR, "Could not check if file %s exists: %s", file_path, strerror(errno));
        return -1;
    }
    return 1;
#endif
}

// minirent.h SOURCE BEGIN ////////////////////////////////////////
#ifdef _WIN32
struct DIR
{
    HANDLE hFind;
    WIN32_FIND_DATA data;
    struct dirent *dirent;
};

DIR *opendir(const char *dirpath)
{
    assert(dirpath);

    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

    DIR *dir = (DIR*)calloc(1, sizeof(DIR));

    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) {
        free(dir);
    }

    return NULL;
}

struct dirent *readdir(DIR *dirp)
{
    assert(dirp);

    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent*)calloc(1, sizeof(struct dirent));
    } else {
        if(!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(
        dirp->dirent->d_name,
        dirp->data.cFileName,
        sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int closedir(DIR *dirp)
{
    assert(dirp);

    if(!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) {
        free(dirp->dirent);
    }
    free(dirp);

    return 0;
}
#endif // _WIN32
// minirent.h SOURCE END ////////////////////////////////////////

#endif