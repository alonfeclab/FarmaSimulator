// Lanzador de FarmaciaSim: permite que el .exe visible esté "solo" en la
// carpeta, con el programa real y todas sus DLL escondidos en el subdirectorio
// oculto "bin". Sin consola, sin dependencias (enlazado estático).
//
// Compilación (MinGW):
//   g++ -O2 -static -mwindows launcher.cpp -o FarmaciaSim.exe

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // Carpeta donde está este lanzador
    wchar_t ruta[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, ruta, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return 1;
    for (int i = int(n) - 1; i >= 0; --i) {
        if (ruta[i] == L'\\' || ruta[i] == L'/') { ruta[i] = L'\0'; break; }
    }

    wchar_t binDir[MAX_PATH], exeReal[MAX_PATH];
    lstrcpyW(binDir, ruta);
    lstrcatW(binDir, L"\\bin");
    lstrcpyW(exeReal, binDir);
    lstrcatW(exeReal, L"\\FarmaciaSim.exe");

    // Lanzar el programa real con "bin" como directorio de trabajo:
    // así encuentra sus DLL y guarda el JSON de datos ahí dentro.
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exeReal, nullptr, nullptr, nullptr, FALSE,
                        0, nullptr, binDir, &si, &pi)) {
        MessageBoxW(nullptr,
                    L"No se encuentra bin\\FarmaciaSim.exe.\n"
                    L"La carpeta 'bin' debe estar junto a este ejecutable.",
                    L"Simulación Farmacia", MB_ICONERROR);
        return 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
