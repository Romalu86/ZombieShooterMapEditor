#pragma once

#include "mapedit/legacy_compiler.hpp"

#if defined(_MSC_VER) || defined(__i386__)
struct _iobuf; typedef _iobuf FILE;
#else
#include <cstdio>
#endif


namespace zs1 {

#pragma pack(push, 4)
class zDebugLog { // recovery class name; historical spelling unresolved
public:
    zDebugLog();
    virtual ~zDebugLog();

    void Open();
    void Write(const char* text);

    const char* GetFileName() const noexcept { return m_fileName; }

private:
    FILE* m_file;          // +0x04
    char* m_fileName;           // +0x08
    unsigned char m_openTried;   // +0x0C
    unsigned char m_pad[3];
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

// GLOBAL: ZS1 0x005F1AD4
extern zDebugLog* g_DebugLog;

} // namespace zs1
