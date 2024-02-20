#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <math.h>
#include <raylib.h>

#ifndef _WIN32
#include <signal.h> // needed for sigaction()
#endif // _WIN32

// #include "./hotreload.h" TODO

int main(void)
{
#ifndef _WIN32
    // NOTE: This is needed because if the pipe between the program and FFmpeg breaks
    // The program will receive SIGPIPE on trying to write into it. While such behavior
    // makes sense for command line utilities, the program will recover from such situations.
    //struct sigaction act = {0};
    //act.sa_handler = SIG_IGN;
    //sigaction(SIGPIPE, &act, NULL);
#endif // _WIN32

    // if (!reload_libplug()) return 1; TODO ??

    //Image logo = LoadImage("./resources/logo/logo.png"); TODO
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    size_t factor = 80; // This will result in 1280x720 @ 16:9
    InitWindow(factor*16, factor*9, "Basic Window Example");
    // SetWindowIcon(logo);TODO
    //SetTargetFPS(60);
    //SetExitKey(KEY_NULL);
    InitAudioDevice();

    // Here is where you would include a function to render your scene.
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();
        ClearBackground(BLACK); // Clear the background to black
        EndDrawing();
    }

    CloseAudioDevice();
    EndWindow();
    // UnloadImage(logo); TODO

    return 0;
}
