#include "zArgList.h"
#include "zArg.h"
#include "zCommon.h"

#include <string.h>
#include <new>

#if defined(_MSC_VER) || defined(__i386__)
struct HWND__;
struct HKEY__;
class STREAM;
#include "mapedit/core/string.hpp"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace zs1 {

zArgList::zArgList() noexcept
    : m_items(nullptr), m_size(0), m_capacity(0), m_dirty(0)
{
    m_pad[0]=m_pad[1]=m_pad[2]=0;
}

zArgList::~zArgList()
{
    Clear();
}

const char* zArgList::GetName() const
{
    return nullptr;
}

// Retail intentionally returns immediately when m_items is null; in that path
// it does not normalize size/capacity.
void zArgList::Clear()
{
    if (!m_items)
        return;

    for (int i = 0; i < m_size; ++i)
        delete m_items[i];

    std::free(m_items);
    m_items = nullptr;
    m_capacity = 0;
    m_size = 0;
}

void zArgList::SetInt(const char* name, int value)
{
    zArg* arg = FindArg(name);
    if (!arg)
        arg = AddArg(name);

    if (arg->GetType() != AT_INT || arg->GetInt() != value)
        m_dirty = 1;
    arg->SetInt(value);
}

bool zArgList::GetInt(const char* name, int* outValue)
{
    zArg* arg = FindArg(name);
    if (!arg)
        return false;
    *outValue = arg->GetInt();
    return true;
}

void zArgList::SetStr(const char* name, const char* value)
{
    zArg* arg = FindArg(name);
    if (!arg)
        arg = AddArg(name);

    if (arg->GetType() != AT_STR || std::strcmp(arg->GetStr(), value) != 0)
        m_dirty = 1;
    arg->SetStr(value);
}

bool zArgList::GetStr(const char* name, const char** outValue)
{
    zArg* arg = FindArg(name);
    if (!arg)
        return false;
    *outValue = arg->GetStr();
    return true;
}

zArg* zArgList::FindArg(const char* name)
{

    for (int i = 0; i < m_size; ++i) {
        zArg* arg = m_items[i];
        // Retail dereferences every populated entry directly; AddArg guarantees
        // non-null list entries, so do not add a reconstruction-only null guard.
        if (std::strcmp(arg->GetName(), name) == 0)
            return arg;
    }
    return nullptr;
}

zArg* zArgList::AddArg(const char* name)
{
    zArg* arg = new zArg();
    arg->SetName(name);

    if (m_size == m_capacity) {
        m_capacity += 8;
        m_items = static_cast<zArg**>(std::realloc(m_items, static_cast<unsigned int>(m_capacity) * sizeof(zArg*)));
    }
    m_items[m_size++] = arg;
    return arg;
}

bool zArgList::Load(FILE* file)
{
#if defined(_MSC_VER) && _MSC_VER <= 1200
    // Retail 0x00473778/0x00473820 reads FILE::_flag bit 0x10 directly.
    while (!(file->_flag & 0x10)) {
#else
    while (!std::feof(file)) {
#endif
        char* line = ReadLine(file);
        if (!line)
            break;

        char* left = nullptr;
        char* right = nullptr;
        if (!SplitLine(line, &left, &right, '='))
            continue;

        zArgType type = AT_NONE;
        if (left[0] == 'i')
            type = AT_INT;
        else if (left[0] == 's')
            type = AT_STR;
        else

        ++left;
        if (type == AT_INT)
            SetInt(left, std::atoi(right));
        else if (type == AT_STR)
            SetStr(left, right);
    }
    return true;
}

void zArgList::Save(const char* fileName)
{
#if defined(_MSC_VER) || defined(__i386__)
    STRING path;
    path = fileName;
    BackupFile(path.m_buf);
    FILE* file = std::fopen(path.m_buf, "w");
#else
    BackupFile(fileName);
    FILE* file = std::fopen(fileName, "w");
#endif
    if (file) {
        // Retail performs the virtual GetName call twice on the non-null path.
        if (GetName())
            std::fprintf(file, "%s\n", GetName());

        for (int i = 0; i < m_size; ++i) {
            zArg* arg = m_items[i];
            if (arg->GetType() == AT_INT)
                std::fprintf(file, "i%s=%d\n", arg->GetName(), arg->GetInt());
            else if (arg->GetType() == AT_STR)
                std::fprintf(file, "s%s=%s\n", arg->GetName(), arg->GetStr());
        }
        std::fclose(file);
    }
    m_dirty = 0;
}

} // namespace zs1
