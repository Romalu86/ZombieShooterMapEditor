#include "zUserMngr.h"
#include "zCommon.h"

// Retail ZS1 user-manager owners materialize the engine STRING type directly.
// Keep the old Win32 types forward-declared so this header stays independent
// from the platform SDK include order in both compiler lanes.
#include <string.h>
#include <new>

struct HWND__;
struct HKEY__;
class STREAM;
#include "mapedit/core/string.hpp"

#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace zs1 {

zUserMngr* g_UserMngr = nullptr;

zUserMngr::zUserMngr(const char* rootPath)
    : zArgList(), m_rootPath(nullptr), m_aux(), m_curUser(-1)
{
    memset(m_users,0,sizeof(m_users));
    AssignCString(&m_rootPath, rootPath);
    Load();
}

// Compiler-generated scalar deleting destructor around 0x004732D0.
zUserMngr::~zUserMngr()
{
    Save();
    DeleteAllUsers();
    // Retail 0x004732D0 intentionally does not release m_rootPath here.
    // m_aux and the zArgList base are destroyed automatically afterwards.
}

int zUserMngr::GetUsersCnt() const
{
    int count = 0;
    for (int i = 0; i < MAX_USER_CNT; ++i) {
        if (m_users[i])
            ++count;
    }
    return count;
}

const char* zUserMngr::GetUserNameByOrdinal(int ordinal) const
{
    const int slot = OrdinalToSlot(ordinal);
    return slot >= 0 ? m_users[slot]->GetName() : nullptr;
}

int zUserMngr::FindUser(const char* name) const
{
    for (int i = 0; i < MAX_USER_CNT; ++i) {
        if (m_users[i] && std::strcmp(m_users[i]->GetName(), name) == 0)
            return i;
    }
    return -1;
}

bool zUserMngr::AddUser(const char* name)
{
    // Preserve the original call order: FindUser is evaluated before the explicit null/empty rejection.
    if (FindUser(name) != -1 || !name || !*name)
        return false;

    int slot = 0;
    while (slot < MAX_USER_CNT && m_users[slot])
        ++slot;
    if (slot == MAX_USER_CNT)
        return false;

    zUser* user = new zUser();
    user->SetName(name);
    m_users[slot] = user;
    SetCurUser(slot);
    SaveUser(slot);
    return true;
}

bool zUserMngr::DeleteUser(int slot)
{

    delete m_users[slot];
    m_users[slot] = nullptr;
    SetCurUserByOrdinal(0);
    SaveUser(slot);
    return true;
}

bool zUserMngr::RenameUser(int slot, const char* name)
{
    m_users[slot]->SetName(name);
    SaveUser(slot);
    return true;
}

bool zUserMngr::SetCurUser(int slot)
{

    // Preserve the original bounds behavior: only slot>=0 and the selected pointer are tested here.
    if (slot >= 0 && !m_users[slot])
        slot = -1;

    m_curUser = slot;
    zArgList::SetInt("m_iCurUser", slot);
    Save();
    return true;
}

void zUserMngr::SetInt(bool global, const char* name, int value)
{
    if (!global && m_curUser >= 0 && m_curUser < MAX_USER_CNT)
        m_users[m_curUser]->SetInt(name, value);
    else
        zArgList::SetInt(name, value);
}

int zUserMngr::GetInt(bool global, const char* name, int defaultValue)
{
    int result = defaultValue;
    if (!global && m_curUser >= 0 && m_curUser < MAX_USER_CNT)
        m_users[m_curUser]->GetInt(name, &result);
    else
        zArgList::GetInt(name, &result);
    return result;
}

void zUserMngr::SetStr(bool global, const char* name, const char* value)
{
    if (!global && m_curUser >= 0 && m_curUser < MAX_USER_CNT)
        m_users[m_curUser]->SetStr(name, value);
    else
        zArgList::SetStr(name, value);
}

const char* zUserMngr::GetStr(bool global, const char* name, const char* defaultValue)
{
    const char* result = defaultValue;
    if (!global && m_curUser >= 0 && m_curUser < MAX_USER_CNT)
        m_users[m_curUser]->GetStr(name, &result);
    else
        zArgList::GetStr(name, &result);
    return result;
}

void zUserMngr::DeleteAllUsers()
{
    for (int i = 0; i < MAX_USER_CNT; ++i) {
        delete m_users[i];
        m_users[i] = nullptr;
    }
}

void zUserMngr::Load()
{

#if defined(_MSC_VER) || defined(__i386__)
    // Retail keeps one STRING local and assigns each BuildUserFileName temporary
    // into it on every slot iteration.
    STRING fileName;
    for (int slot = 0; slot < MAX_USER_CNT; ++slot) {
        fileName = BuildUserFileName(slot, false, -1);
        FILE* file = std::fopen(fileName.m_buf, "r");
        if (!file)
            continue;

        zUser* user = new zUser();
        if (!user->Load(file)) {
            delete user;
            user = nullptr;
        }
        m_users[slot] = user;
        std::fclose(file);
    }

    // 0x0047392A..0x0047397F: BuildUserDataPath temporary -> Printf temporary
    // -> assignment into the same STRING local, then fopen.
    fileName = Printf("%s/_global.dat", BuildUserDataPath().m_buf);
    FILE* globalFile = std::fopen(fileName.m_buf, "r");
    if (globalFile) {
        // Retail dispatches slot 0 through the zArgList vtable here.
        static_cast<zArgList*>(this)->Load(globalFile);
        std::fclose(globalFile);
    }
#else
    // Non-target host syntax fallback.  Production Win32/x86 takes the STRING path above.
    char fileName[1024];
    for (int slot = 0; slot < MAX_USER_CNT; ++slot) {
        BuildUserFileName(fileName, sizeof(fileName), slot, false, -1);
        FILE* file = std::fopen(fileName, "r");
        if (!file)
            continue;
        zUser* user = new zUser();
        if (!user->Load(file)) { delete user; user = nullptr; }
        m_users[slot] = user;
        std::fclose(file);
    }
    char dataPath[1024];
    BuildUserDataPath(dataPath, sizeof(dataPath));
    MAPEDIT_SNPRINTF(fileName, sizeof(fileName), "%s/_global.dat", dataPath);
    FILE* globalFile = std::fopen(fileName, "r");
    if (globalFile) { static_cast<zArgList*>(this)->Load(globalFile); std::fclose(globalFile); }
#endif

    zArgList::GetInt("m_iCurUser", &m_curUser);
    if (m_curUser < 0 || m_curUser >= MAX_USER_CNT || !m_users[m_curUser])
        m_curUser = -1;
}

void zUserMngr::Save()
{
    for (int slot = 0; slot < MAX_USER_CNT; ++slot) {
        if (m_users[slot] && m_users[slot]->IsDirty())
            SaveUser(slot);
    }

    if (IsDirty()) {
#if defined(_MSC_VER) || defined(__i386__)
        STRING fileName;
        fileName = Printf("%s/_global.dat", BuildUserDataPath().m_buf);
        zArgList::Save(fileName.m_buf);
#else
        char dataPath[1024];
        char fileName[1024];
        BuildUserDataPath(dataPath, sizeof(dataPath));
        MAPEDIT_SNPRINTF(fileName, sizeof(fileName), "%s/_global.dat", dataPath);
        zArgList::Save(fileName);
#endif
    }
}

void zUserMngr::SaveUser(int slot)
{
#if defined(_MSC_VER) || defined(__i386__)
    // Retail owns two persistent STRING locals and assigns the two filename
    // temporaries into them before testing the user slot.
    STRING normal;
    STRING hash;
    normal = BuildUserFileName(slot, false, -1);
    hash = BuildUserFileName(slot, true, -1);

    if (!m_users[slot]) {
        BackupFile(normal.m_buf);
        BackupFile(hash.m_buf);
        return;
    }
    m_users[slot]->Save(normal.m_buf);
#else
    char normal[1024];
    char hash[1024];
    BuildUserFileName(normal, sizeof(normal), slot, false, -1);
    BuildUserFileName(hash, sizeof(hash), slot, true, -1);
    if (!m_users[slot]) { BackupFile(normal); BackupFile(hash); return; }
    m_users[slot]->Save(normal);
#endif
}

int zUserMngr::OrdinalToSlot(int ordinal) const
{
    for (int slot = 0; slot < MAX_USER_CNT; ++slot) {
        if (!m_users[slot])
            continue;
        if (ordinal-- == 0)
            return slot;
    }
    return -1;
}

int zUserMngr::SlotToOrdinal(int slot) const
{

    int ordinal = 0;
    for (int i = 0; i < slot; ++i) {
        if (m_users[i])
            ++ordinal;
    }
    return ordinal;
}

bool zUserMngr::DeleteUserByOrdinal(int ordinal)
{
    return DeleteUser(OrdinalToSlot(ordinal));
}

bool zUserMngr::RenameUserByOrdinal(int ordinal, const char* name)
{
    return RenameUser(OrdinalToSlot(ordinal), name);
}

bool zUserMngr::SetCurUserByOrdinal(int ordinal)
{
    if (ordinal == -1)
        return SetCurUser(-1);
    return SetCurUser(OrdinalToSlot(ordinal));
}

bool zUserMngr::SelectOrAddUserByName(const char* name)
{
    const int slot = FindUser(name);
    if (slot == -1)
        return AddUser(name);
    return SetCurUser(slot);
}

int zUserMngr::GetCurUserOrdinal() const
{
    if (m_curUser < 0)
        return -1;
    return SlotToOrdinal(m_curUser);
}

void zUserMngr::BuildUserFileName(char* out, unsigned int outSize, int slot, bool hash, int suffixIndex) const
{
    BuildUserFileNameBuffer(this,out,outSize,slot,hash,suffixIndex);
}

void zUserMngr::BuildUserDataPath(char* out, unsigned int outSize) const
{
    BuildUserDataPathBuffer(this,out,outSize);
}

void zUserMngr::SetAuxInt(const char* name, int value)
{
    m_aux.SetInt(name, value);
}

int zUserMngr::GetAuxInt(const char* name, int defaultValue)
{
    int result = defaultValue;
    m_aux.GetInt(name, &result);
    return result;
}

void zUserMngr::SetAuxStr(const char* name, const char* value)
{
    m_aux.SetStr(name, value);
}

const char* zUserMngr::GetAuxStr(const char* name, const char* defaultValue)
{
    const char* result = defaultValue;
    m_aux.GetStr(name, &result);
    return result;
}

void zUserMngr::LoadAux(int suffixIndex)
{
    m_aux.Clear();
    if (m_curUser < 0)
        return;

#if defined(_MSC_VER) || defined(__i386__)
    STRING fileName;
    fileName = BuildUserFileName(m_curUser, false, suffixIndex);
    FILE* file = std::fopen(fileName.m_buf, "r");
    if (file) {
        m_aux.Load(file);
        std::fclose(file);
    }
#else
    char fileName[1024];
    BuildUserFileName(fileName, sizeof(fileName), m_curUser, false, suffixIndex);
    FILE* file = std::fopen(fileName, "r");
    if (file) { m_aux.Load(file); std::fclose(file); }
#endif
}

void zUserMngr::SaveAux(int suffixIndex)
{
    if (!m_aux.IsDirty() || m_curUser < 0)
        return;
#if defined(_MSC_VER) || defined(__i386__)
    STRING fileName;
    fileName = BuildUserFileName(m_curUser, false, suffixIndex);
    m_aux.Save(fileName.m_buf);
#else
    char fileName[1024];
    BuildUserFileName(fileName, sizeof(fileName), m_curUser, false, suffixIndex);
    m_aux.Save(fileName);
#endif
}

void zUserMngr::ClearAux()
{
    m_aux.Clear();
}

} // namespace zs1
