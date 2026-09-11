#pragma once

#include "mapedit/legacy_compiler.hpp"

#if defined(_MSC_VER) || defined(__i386__)
struct _iobuf; typedef _iobuf FILE;
#else
#include <cstdio>
#endif


namespace zs1 {
class zArg;

#pragma pack(push, 4)
class zArgList {
public:
    // Convenience constructor; original allocation routes initialized this state inline.
    zArgList() noexcept;
    ~zArgList();
    virtual bool Load(FILE* file);
    virtual const char* GetName() const;

    void Clear();
    void SetInt(const char* name, int value);
    bool GetInt(const char* name, int* outValue);
    void SetStr(const char* name, const char* value);
    bool GetStr(const char* name, const char** outValue);
    zArg* FindArg(const char* name);
    zArg* AddArg(const char* name);
    void Save(const char* fileName);

    bool IsDirty() const noexcept { return m_dirty != 0; }
    int GetArgCount() const noexcept { return m_size; }

protected:
    zArg** m_items;              // +0x04
    int m_size;                  // +0x08
    int m_capacity;              // +0x0C
    unsigned char m_dirty;        // +0x10
    unsigned char m_pad[3];
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

} // namespace zs1
