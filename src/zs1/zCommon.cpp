#include "zCommon.h"

#include <cstdio>
#include <cstring>
#include <new>

// Do not include mapedit/runtime.hpp or windows.h in this standalone ZS1 TU:
// the project umbrella intentionally carries historical x86 Win32/CRT ABI
// declarations which collide with the modern VS2022 SDK/CRT declarations.
#if defined(_MSC_VER) || defined(__i386__)
struct HWND__;
struct HKEY__;
class STREAM;
#include "mapedit/core/string.hpp"
#if defined(_WIN32)
extern "C" int __stdcall MoveFileExA(const char*, const char*, unsigned long);
#endif
#endif

namespace zs1 {

namespace {
// ZS1 retail .data contains an initialized pointer at 0x0049C71C that points
// at the 1000-byte zero-filled line buffer at 0x005F1ADC.  Keep the pointer
// and storage as separate owners: target ReadLine reloads the pointer before
// fgets/strlen instead of addressing the array directly.
char g_readLineBuffer[1000];
char* g_readLine = g_readLineBuffer;
}

void AssignCString(char** destination, const char* source)
{
    if (*destination) {
        ::operator delete(*destination);
        *destination = nullptr;
    }

    if (!source)
        return;

    const unsigned int size = std::strlen(source) + 1;
    char* copy = static_cast<char*>(::operator new(size));
    std::memcpy(copy, source, size);
    *destination = copy;
}

bool SplitLine(char* text, char** left, char** right, char delimiter)
{
    char* split = std::strchr(text, delimiter);
    if (!split)
        return false;

    *split = '\0';
    *left = text;
    *right = split + 1;
    return true;
}

char* ReadLine(FILE* file)
{
    // Direct ZS1 target proof: 0x00474EF0 loads the initialized pointer at
    // 0x0049C71C, calls fgets(pointer,1000,file), tests FILE::_flag & 0x10,
    // then repeatedly rescans the pointed buffer while stripping CR/LF.
    char* result = fgets(g_readLine, 1000, file);

#if defined(_MSC_VER) && _MSC_VER <= 1200
    if (file->_flag & 0x10)
        return nullptr;
#else
    if (feof(file))
        return nullptr;
#endif

    while (strlen(g_readLine) != 0 &&
           (g_readLine[strlen(g_readLine) - 1] == '\n' ||
            g_readLine[strlen(g_readLine) - 1] == '\r')) {
        g_readLine[strlen(g_readLine) - 1] = '\0';
    }
    return result;
}

void BackupFile(const char* fileName)
{
#if defined(_MSC_VER) || defined(__i386__)
    // Retail constructs two engine STRING values before MoveFileExA. In
    // particular, STRING(const char*) maps null/empty input to STRING::EMPTY;
    // BackupFile itself does not early-return for a null input.
    STRING source(fileName);
    STRING backup(source);
    backup += ".bak";
#if defined(_WIN32)
    ::MoveFileExA(source.m_buf, backup.m_buf, 9u);
#endif
#else
    // Host-only syntax-gate equivalent; production target is Win32/x86 above.
    const char* source = fileName ? fileName : "";
    const unsigned int len = std::strlen(source);
    char* backup = static_cast<char*>(::operator new(len + sizeof(".bak")));
    std::memcpy(backup, source, len);
    std::memcpy(backup + len, ".bak", sizeof(".bak"));
    ::operator delete(backup);
#endif
}

unsigned int Adler32(const char* text)
{
    unsigned int a = 1;
    unsigned int b = 0;
    while (*text) {
        a = (a + static_cast<unsigned char>(*text++)) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) + a;
}

} // namespace zs1
