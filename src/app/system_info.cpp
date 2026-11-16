// The release log's opening lines: what the game runs on and with which settings
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <intrin.h>
#pragma comment(lib, "advapi32.lib")
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif

#include <algorithm>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "app/app.h"
#include "build_info.h"
#include "core/log.h"
#include "image_symbols.h"

namespace app {

namespace {

#ifdef _WIN32
std::string RegString(HKEY key, const char* name) {
    char text[256] = {};
    DWORD size = sizeof(text) - 1;
    if (RegQueryValueExA(key, name, nullptr, nullptr, reinterpret_cast<LPBYTE>(text), &size) != ERROR_SUCCESS) return {};
    return text;
}

std::string OsVersion() {
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ | KEY_WOW64_64KEY,
                      &key) != ERROR_SUCCESS)
        return "unknown";
    std::string name = RegString(key, "ProductName"), display = RegString(key, "DisplayVersion"),
                build = RegString(key, "CurrentBuildNumber");
    DWORD ubr = 0, size = sizeof(ubr);
    RegQueryValueExA(key, "UBR", nullptr, nullptr, reinterpret_cast<LPBYTE>(&ubr), &size);
    RegCloseKey(key);
    // Windows 11 still names itself Windows 10 here, its build number tells
    if (auto ten = name.find("Windows 10"); ten != std::string::npos && std::atoi(build.c_str()) >= 22000)
        name.replace(ten, 10, "Windows 11");
    if (!display.empty()) name += " " + display;
    if (!build.empty()) name += " (build " + build + "." + std::to_string(ubr) + ")";
    using WineVersion = const char*(__cdecl*)();
    if (auto wine = reinterpret_cast<WineVersion>(reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version"))))
        name += std::string(" - Wine ") + wine();
    return name;
}

std::string CpuName() {
    int r[4] = {};
    __cpuid(r, int(0x80000000));
    if (uint32_t(r[0]) < 0x80000004) return "unknown";
    char brand[49] = {};
    for (int i = 0; i < 3; i++) {
        __cpuid(r, int(0x80000002 + i));
        std::memcpy(brand + i * 16, r, 16);
    }
    std::string s = brand;
    s.erase(0, s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}
#elif defined(__APPLE__)
std::string OsVersion() {
    char osversion[64] = {};
    size_t size = sizeof(osversion);
    if (sysctlbyname("kern.osproductversion", osversion, &size, nullptr, 0) == 0) {
        return std::string("macOS ") + osversion;
    }
    return "macOS";
}

std::string CpuName() {
    char brand[128] = {};
    size_t size = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", brand, &size, nullptr, 0) == 0) {
        return brand;
    }
    return "Apple Silicon";
}
#else
std::string OsVersion() { return "POSIX"; }
std::string CpuName() { return "Generic CPU"; }
#endif

}  // namespace

void LogSystem() {
    std::time_t now = std::time(nullptr);
    char date[32] = {};
    std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    constexpr const char* kBuild = logx::kVerbose ? " (" SCDA_COMMIT ", debug)" : "";

#ifdef _WIN32
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    LOGR("Splinter Cell Double Agent XBE Recomp V%d%s, %ls (%s), %s", kVersion, kBuild,
         std::filesystem::path(exe).filename().c_str(), IMAGE_XBE, date);
    LOGR("Path: %ls", ExeDir().c_str());
    LOGR("OS: %s", OsVersion().c_str());
    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);
    LOGR("CPU: %s (%lu threads)", CpuName().c_str(), si.dwNumberOfProcessors);
    MEMORYSTATUSEX mem{sizeof(mem)};
    GlobalMemoryStatusEx(&mem);
    LOGR("RAM: %.1f GB", mem.ullTotalPhys / 1073741824.0);
    std::vector<std::string> seen;
    for (DWORD i = 0;; i++) {
        DISPLAY_DEVICEA dd{sizeof(dd)};
        if (!EnumDisplayDevicesA(nullptr, i, &dd, 0)) break;
        if (std::find(seen.begin(), seen.end(), dd.DeviceString) != seen.end()) continue;
        seen.emplace_back(dd.DeviceString);
        LOGR("GPU %zu: %s%s", seen.size(), dd.DeviceString, (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? " (primary)" : "");
    }
#elif defined(__APPLE__)
    LOGR("Splinter Cell Double Agent XBE Recomp V%d%s, %s, %s", kVersion, kBuild, IMAGE_XBE, date);
    LOGR("Path: %s", ExeDir().string().c_str());
    LOGR("OS: %s", OsVersion().c_str());
    int ncpu = 1;
    size_t ncpu_len = sizeof(ncpu);
    sysctlbyname("hw.ncpu", &ncpu, &ncpu_len, nullptr, 0);
    LOGR("CPU: %s (%d threads)", CpuName().c_str(), ncpu);
    uint64_t mem = 0;
    size_t mem_len = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &mem_len, nullptr, 0);
    LOGR("RAM: %.1f GB", mem / 1073741824.0);
    LOGR("GPU: Apple Silicon Metal GPU");
#endif
    Mode desk = Desktop();
    LOGR("Desktop: %ux%u", desk.width, desk.height);
    LogSettings();
}

}  // namespace app
