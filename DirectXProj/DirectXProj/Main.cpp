#include <Windows.h>
#include "Core/Game.h"

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Game game(hInstance);

    if (game.Initialize(L"DirectX 11 Game Framework", 1280, 720))
    {
        game.Run();
    }

    game.Shutdown();

    CoUninitialize();

    return 0;
}
