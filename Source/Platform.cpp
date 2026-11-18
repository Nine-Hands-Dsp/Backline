// Fonctions propres a Windows, isolees ici (windows.h hors de l'espace de noms juce).
#include <string>

#ifdef _WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
 #include <vector>

namespace pmplat
{
    // version du fichier (ressource VERSIONINFO) : "1.2.3", vide si absente
    std::wstring fileVersion (const std::wstring& path)
    {
        DWORD h = 0;
        const DWORD n = GetFileVersionInfoSizeW (path.c_str(), &h);
        if (n == 0) return {};
        std::vector<unsigned char> buf (n);
        if (! GetFileVersionInfoW (path.c_str(), 0, n, buf.data())) return {};
        VS_FIXEDFILEINFO* fi = nullptr; UINT len = 0;
        if (! VerQueryValueW (buf.data(), L"\\", (void**) &fi, &len) || fi == nullptr) return {};
        return std::to_wstring (HIWORD (fi->dwFileVersionMS)) + L"." + std::to_wstring (LOWORD (fi->dwFileVersionMS)) + L"."
             + std::to_wstring (HIWORD (fi->dwFileVersionLS));
    }

    // relance l'application avec les droits administrateur (invite UAC)
    bool relaunchAsAdmin (const std::wstring& exe)
    {
        auto r = (INT_PTR) ShellExecuteW (nullptr, L"runas", exe.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return r > 32;
    }
}
#else
namespace pmplat
{
    std::wstring fileVersion (const std::wstring&) { return {}; }
    bool relaunchAsAdmin (const std::wstring&) { return false; }
}
#endif
