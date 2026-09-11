#include "zHelpParser.h"
#include "zCommon.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

struct HWND__;
struct HKEY__;
class STREAM;
#include "mapedit/core/string.hpp"

namespace zs1 {

zHelpParser* g_HelpParser = nullptr;

namespace {

void AppendCStringLine(char** destination, const char* line)
{
    const char* oldText = *destination ? *destination : "";
    const char* newLine = line ? line : "";
    const unsigned int oldLen = std::strlen(oldText);
    const unsigned int lineLen = std::strlen(newLine);

    char* merged = static_cast<char*>(::operator new(oldLen + lineLen + 2));
    std::memcpy(merged, oldText, oldLen);
    std::memcpy(merged + oldLen, newLine, lineLen);
    merged[oldLen + lineLen] = '\n';
    merged[oldLen + lineLen + 1] = '\0';

    if (*destination)
        ::operator delete(*destination);
    *destination = merged;
}

} // namespace

zHelpItem::zHelpItem() noexcept
{
    // Retail AppendItem initializes the data fields in +04,+08,+0C,+10,+14 order.
    // Keep the same logical write order; the compiler-generated vptr store is codegen noise.
    m_value1 = 0;
    m_value2 = 0;
    m_text.m_str = STRING::EMPTY;
    m_value3 = 0;
    m_type = 0;
}

// Target scalar deleting destructor folds the zHelpItem string teardown inline.
zHelpItem::~zHelpItem()
{
    // zStringField is layout-compatible with STRING (+0x0C is its sole pointer).
    // Use the real STRING teardown rule: the shared EMPTY sentinel is never freed.
    if (m_text.m_str != STRING::EMPTY)
        ::operator delete(m_text.m_str);
}

zHelpLevel::zHelpLevel(int levelNum) noexcept
    : m_items(nullptr), m_size(0), m_capacity(0), m_levelNum(levelNum), m_loaded(0)
{
    // Retail 0x004752B0 does not write the three padding bytes at +0x15..+0x17.
}

// Compiler-generated scalar deleting destructor around the 0x004752F0 body.
zHelpLevel::~zHelpLevel()
{
    Clear();
}

void zHelpLevel::Clear()
{
    // Retail short path: when the pointer array is null, only m_loaded changes.
    if (!m_items) {
        m_loaded = 0;
        return;
    }

    for (int i = 0; i < m_size; ++i)
        delete m_items[i];

    std::free(m_items);
    m_items = nullptr;
    m_size = 0;
    m_capacity = 0;
    m_loaded = 0;
}

zHelpItem* zHelpLevel::GetItem(int item)
{
    Load();
    return m_items[item];
}

zHelpItem* zHelpLevel::AppendItem()
{

    if (m_size == m_capacity) {
        m_capacity += 8;
        m_items = static_cast<zHelpItem**>(
            std::realloc(m_items, static_cast<unsigned int>(m_capacity) * sizeof(zHelpItem*)));
    }

    zHelpItem* item = new zHelpItem();
    m_items[m_size++] = item;
    return item;
}

bool zHelpLevel::Load()
{
    if (m_loaded)
        return true;


#if defined(_MSC_VER) || defined(__i386__)
    // 0x004754F4..0x0047553A: persistent level tag STRING assigned from Printf.
    STRING levelTag;
    levelTag = Printf("<Level_%02d>", m_levelNum % 100 + 1);

    // Retail reads zHelpParser::m_rootPath directly at +0x10 and copies it into
    // a STRING, then appends a short-lived Printf result.
    STRING fileName(g_HelpParser->m_rootPath);
    {
        STRING shortName = Printf("help_l%02d.txt", m_levelNum / 100 + 1);
        fileName += &shortName;
    }

    FILE* file = std::fopen(fileName.m_buf, "r");
#else
    const int fileIndex = m_levelNum / 100 + 1;
    const int levelInFile = m_levelNum % 100 + 1;
    char shortName[64];
    char levelTagBuffer[64];
    char fullName[1024];
    MAPEDIT_SNPRINTF(shortName, sizeof(shortName), "help_l%02d.txt", fileIndex);
    MAPEDIT_SNPRINTF(levelTagBuffer, sizeof(levelTagBuffer), "<Level_%02d>", levelInFile);
    MAPEDIT_SNPRINTF(fullName, sizeof(fullName), "%s%s", g_HelpParser->GetRootPath(), shortName);
    FILE* file = std::fopen(fullName, "r");
    const char* levelTag = levelTagBuffer;
#endif
    if (!file) {
        m_loaded = 1;
        return true;
    }

    bool foundLevel = false;
    zHelpItem* current = nullptr;
    int currentType = 0;

#if defined(_MSC_VER) && _MSC_VER <= 1200
    while (!(file->_flag & 0x10)) {
#else
    while (!std::feof(file)) {
#endif
        char* line = ReadLine(file);
        if (!line)
            break;

#if defined(_MSC_VER) || defined(__i386__)
        const char* wantedLevel = levelTag.m_buf;
#else
        const char* wantedLevel = levelTag;
#endif
        if (!foundLevel && std::strcmp(line, wantedLevel) == 0)
            foundLevel = true;

        // Retail checks EndLevel before rejecting pre-level lines.
        if (std::strcmp(line, "<EndLevel>") == 0)
            break;
        if (!foundLevel)
            continue;

        if (std::strcmp(line, "<Text>") == 0) {
            current = AppendItem();
            currentType = 1;
            current->m_type = currentType;
            continue;
        }

        if (std::strcmp(line, "<Img>") == 0) {
            current = AppendItem();
            currentType = 2;
            current->m_type = currentType;
            continue;
        }

        if (currentType == 2) {
            char* key = nullptr;
            char* value = nullptr;
            if (!SplitLine(line, &key, &value, '='))
                continue;
            if (std::strcmp(key, "vid") == 0)
                current->m_value1 = std::atoi(value);
            else if (std::strcmp(key, "dir") == 0)
                current->m_value2 = std::atoi(value);
        }
        else if (currentType == 1) {
#if defined(_MSC_VER) || defined(__i386__)
            STRING* text = reinterpret_cast<STRING*>(&current->m_text);
            *text += line;
            *text += "\n";
#else
            AppendCStringLine(&current->m_text.m_str, line);
#endif
        }
    }

    std::fclose(file);
    m_loaded = 1;
    return true;
}

zHelpParser::zHelpParser(const char* rootPath) noexcept
    : m_unknown0(nullptr), m_levels(nullptr), m_levelSize(0), m_capacity(0), m_rootPath(nullptr)
{
    AssignCString(&m_rootPath, rootPath);
}

zHelpParser::~zHelpParser()
{
    for (int i = 0; i < m_levelSize; ++i)
        delete m_levels[i];
    std::free(m_levels);
    AssignCString(&m_rootPath, nullptr);
}

zHelpLevel* zHelpParser::GetLevel(int level, bool create)
{
    for (int i = 0; i < m_levelSize; ++i) {
        // Retail directly dereferences every populated level entry.
        if (m_levels[i]->m_levelNum == level)
            return m_levels[i];
    }

    if (!create)
        return nullptr;

    zHelpLevel* result = new zHelpLevel(level);

    if (m_levelSize == m_capacity) {
        m_capacity += 8;
        m_levels = static_cast<zHelpLevel**>(
            std::realloc(m_levels, static_cast<unsigned int>(m_capacity) * sizeof(zHelpLevel*)));
    }

    m_levels[m_levelSize++] = result;
    return result;
}

int zHelpParser::LoadLevel(int level)
{
    zHelpLevel* helpLevel = GetLevel(level, true);
    if (!helpLevel)
        return 0;
    helpLevel->Clear();
    helpLevel->Load();
    return helpLevel->m_size;
}

int zHelpParser::GetItemType(int level, int item)
{
    zHelpLevel* helpLevel = GetLevel(level, false);
    return helpLevel->GetItem(item)->m_type;
}

int zHelpParser::GetItemMetric(int level, int item)
{
    zHelpLevel* helpLevel = GetLevel(level, false);
    zHelpItem* helpItem = helpLevel->GetItem(item);

    if (helpItem->m_type == 1)
        return helpItem->m_value3;
    if (helpItem->m_type == 2)
        return ResolveHelpVidMetric(helpItem->m_value1);
    return 0;
}

const zStringField* zHelpParser::GetItemText(int level, int item)
{
    zHelpLevel* helpLevel = GetLevel(level, false);
    return &helpLevel->GetItem(item)->m_text;
}

int zHelpParser::GetItemValue1(int level, int item)
{
    zHelpLevel* helpLevel = GetLevel(level, false);
    return helpLevel->GetItem(item)->m_value1;
}

int zHelpParser::GetItemValue2(int level, int item)
{
    zHelpLevel* helpLevel = GetLevel(level, false);
    return helpLevel->GetItem(item)->m_value2;
}

} // namespace zs1
