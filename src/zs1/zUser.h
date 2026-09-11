#pragma once

#include "mapedit/legacy_compiler.hpp"

#include "zArgList.h"

namespace zs1 {

#pragma pack(push, 4)
class zUser : public zArgList {
public:
    // Convenience constructor; original allocation sites initialized these fields inline.
    zUser() noexcept;
    bool Load(FILE* file) override;
    const char* GetName() const override;
    // vtable slot 2 introduced here; body 0x00472EF0, scalar deleting dtor 0x00472F20
    virtual ~zUser();

    void SetName(const char* name);

private:
    char* m_name; // +0x14
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

} // namespace zs1
