#pragma once

#include "mapedit/legacy_compiler.hpp"


namespace zs1 {

struct zStringField {
    char* m_str;
};

#pragma pack(push, 4)
class zHelpItem {
public:
    zHelpItem() noexcept;
    // vtable 0x00492CA0, scalar deleting destructor 0x00475440
    virtual ~zHelpItem();

    int m_value1;          // +0x04, IMG: vid
    int m_value2;          // +0x08, IMG: dir
    zStringField m_text;   // +0x0C
    int m_value3;          // +0x10, TEXT metric/line-height related value
    int m_type;            // +0x14: 1=Text, 2=Img
};

class zHelpLevel {
public:
    explicit zHelpLevel(int levelNum) noexcept;
    // vtable 0x00492C9C, body 0x004752F0, scalar deleting destructor 0x004752D0
    virtual ~zHelpLevel();

    void Clear();
    zHelpItem* GetItem(int item);
    zHelpItem* AppendItem();
    bool Load();

    zHelpItem** m_items;    // +0x04
    int m_size;             // +0x08
    int m_capacity;         // +0x0C
    int m_levelNum;         // +0x10
    unsigned char m_loaded;  // +0x14
    unsigned char m_pad[3];
};

class zHelpParser {
public:
    static constexpr int MAX_LEVEL_CNT = 200;

    // Constructor address is not isolated yet. This synthetic ctor preserves observed layout.
    explicit zHelpParser(const char* rootPath = nullptr) noexcept;
    ~zHelpParser();

    zHelpLevel* GetLevel(int level, bool create);
    int LoadLevel(int level);
    int GetItemType(int level, int item);
    int GetItemMetric(int level, int item);
    // Original returns the address of the +0x0C string field, not the char* itself.
    const zStringField* GetItemText(int level, int item);
    int GetItemValue1(int level, int item);
    int GetItemValue2(int level, int item);

    const char* GetRootPath() const noexcept { return m_rootPath ? m_rootPath : ""; }

private:
    friend class zHelpLevel;

    void* m_unknown0;          // +0x00
    zHelpLevel** m_levels;     // +0x04
    int m_levelSize;           // +0x08
    int m_capacity;            // +0x0C
    char* m_rootPath;          // +0x10, observed in zHelpLevel::Load
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

// GLOBAL: ZS1 0x005F1EC4
extern zHelpParser* g_HelpParser;

// Engine-owner boundary. 0x00475170 resolves Map->VID[vidIndex] and reads signed short +0x300.
// Left external until MAP/VID owners are integrated into this source tree.
int ResolveHelpVidMetric(int vidIndex);

} // namespace zs1
