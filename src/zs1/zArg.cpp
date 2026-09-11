#include "zArg.h"
#include "zCommon.h"

namespace zs1 {

zArg::zArg() noexcept
    : m_name(nullptr), m_intValue(0), m_strValue(nullptr), m_type(AT_NONE)
{
}

// Compiler-generated scalar deleting destructor around the 0x00472E00 body.
zArg::~zArg()
{
    AssignCString(&m_name, nullptr);
    AssignCString(&m_strValue, nullptr);
}

// Retail: store value at +0x08 and type AT_INT at +0x10.
void zArg::SetInt(int value)
{
    m_intValue = value;
    m_type = AT_INT;
}

// Return the integer field at +0x08.
int zArg::GetInt() const
{
    return m_intValue;
}

// Retail: AssignCString(+0x0C,value), then type=AT_STR.
void zArg::SetStr(const char* value)
{
    AssignCString(&m_strValue, value);
    m_type = AT_STR;
}

// Return the string field at +0x0C.
const char* zArg::GetStr() const
{
    return m_strValue;
}

void zArg::SetName(const char* name)
{
    AssignCString(&m_name, name);
}

} // namespace zs1
