#pragma once

#include "mapedit/legacy_compiler.hpp"


namespace zs1 {

enum zArgType {
    AT_NONE = 0,
    AT_INT = 1,
    AT_STR = 2,
};

#pragma pack(push, 4)
class zArg {
public:
    // Convenience constructor; original allocation sites initialized these fields inline.
    zArg() noexcept;

    virtual ~zArg();

    void SetInt(int value);
    int GetInt() const;
    void SetStr(const char* value);
    const char* GetStr() const;

    const char* GetName() const noexcept { return m_name; }
    zArgType GetType() const noexcept { return m_type; }
    void SetName(const char* name);

private:
    char* m_name;              // +0x04
    int m_intValue;            // +0x08
    char* m_strValue;          // +0x0C
    zArgType m_type;           // +0x10
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

} // namespace zs1
