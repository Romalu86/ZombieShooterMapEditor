#pragma once

#include "mapedit/legacy_compiler.hpp"

#include "zArgList.h"
#include "zUser.h"

class STRING;


namespace zs1 {

#pragma pack(push, 4)
class zUserMngr : public zArgList {
public:
    static constexpr int MAX_USER_CNT = 100;

    explicit zUserMngr(const char* rootPath);
    // vtable slot 2; body 0x004732D0, scalar deleting dtor 0x004732B0
    virtual ~zUserMngr();

    int GetUsersCnt() const;
    const char* GetUserNameByOrdinal(int ordinal) const;
    int FindUser(const char* name) const;
    bool AddUser(const char* name);
    bool DeleteUser(int slot);
    bool RenameUser(int slot, const char* name);
    bool SetCurUser(int slot);

    void SetInt(bool global, const char* name, int value);
    int GetInt(bool global, const char* name, int defaultValue);
    void SetStr(bool global, const char* name, const char* value);
    const char* GetStr(bool global, const char* name, const char* defaultValue);

    void DeleteAllUsers();
    using zArgList::Load; // keep base virtual overload visible; no ABI/layout change

    void Load();
    void Save();
    void SaveUser(int slot);

    int OrdinalToSlot(int ordinal) const;
    int SlotToOrdinal(int slot) const;
    bool DeleteUserByOrdinal(int ordinal);
    bool RenameUserByOrdinal(int ordinal, const char* name);
    bool SetCurUserByOrdinal(int ordinal);
    bool SelectOrAddUserByName(const char* name);
    int GetCurUserOrdinal() const;

    // Target STRING-returning owners plus ABI-neutral buffer adapters.
    STRING BuildUserFileName(int slot, bool hash, int suffixIndex) const;
    void BuildUserFileName(char* out, unsigned int outSize, int slot, bool hash, int suffixIndex) const;
    STRING BuildUserDataPath() const;
    void BuildUserDataPath(char* out, unsigned int outSize) const;

    // Embedded +0x1A8 arg store wrappers.
    void SetAuxInt(const char* name, int value);
    int GetAuxInt(const char* name, int defaultValue);
    void SetAuxStr(const char* name, const char* value);
    const char* GetAuxStr(const char* name, const char* defaultValue);
    void LoadAux(int suffixIndex);
    void SaveAux(int suffixIndex);
    void ClearAux();

private:
    char* m_rootPath;                  // +0x14
    zUser* m_users[MAX_USER_CNT];      // +0x18
    zArgList m_aux;                    // +0x1A8
    int m_curUser;                     // +0x1BC, physical slot
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

void BuildUserFileNameBuffer(const zUserMngr* self,char* out,unsigned int outSize,int slot,bool hash,int suffixIndex);
void BuildUserDataPathBuffer(const zUserMngr* self,char* out,unsigned int outSize);

extern zUserMngr* g_UserMngr; // GLOBAL: ZS1 0x005F0AD0

} // namespace zs1
