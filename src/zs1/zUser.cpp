#include "zUser.h"
#include "zCommon.h"

namespace zs1 {

zUser::zUser() noexcept : zArgList(), m_name(nullptr)
{
}

// Compiler-generated scalar deleting destructor around the 0x00472EF0 body.
zUser::~zUser()
{
    AssignCString(&m_name, nullptr);
}

bool zUser::Load(FILE* file)
{
    char* name = ReadLine(file);
    if (!name)
        return false;
    SetName(name);
    return zArgList::Load(file);
}

const char* zUser::GetName() const
{
    return m_name;
}

// Assign the +0x14 C-string field.
void zUser::SetName(const char* name)
{
    AssignCString(&m_name, name);
}

} // namespace zs1
