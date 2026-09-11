#include "mapedit/runtime.hpp"

namespace {
char g_registryStringBuffer[512];
}

REGISTRY::REGISTRY(const STRING& registryPath) : path(registryPath) {}

// REGISTRY destructor is header-visible; STRING teardown folds at each delete site.

const STRING* REGISTRY::Path()
{
    return &path;
}

// ZS1 retail 0x00442960..0x00442A12.
int REGISTRY::GetInt(const STRING& name,int defaultInt)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    int result = defaultInt;
    if (RegOpenKeyExA(root, subKey.CharPtr(), 0, 1, &key) == 0) {
        unsigned long size = 0x1FF;
        unsigned long type = 0;
        RegQueryValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, &type,
                         reinterpret_cast<unsigned char*>(g_registryStringBuffer), &size);
        if (type == 1)
            result = atoi(g_registryStringBuffer);
        else if (type == 4 || type == 3)
            result = *reinterpret_cast<int*>(g_registryStringBuffer);
        RegCloseKey(key);
    }
    return result;
}

// ZS1 retail 0x00442A20..0x00442AAE.
unsigned long REGISTRY::GetData(const STRING& name,void* data,unsigned long size)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    if (RegOpenKeyExA(root, subKey.CharPtr(), 0, 1, &key) != 0)
        return 0;

    unsigned long type = 3;
    RegQueryValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, &type,
                     static_cast<unsigned char*>(data), &size);
    RegCloseKey(key);
    return size;
}

// ZS1 retail 0x00442B50..0x00442BCE.
void REGISTRY::SetInt(const STRING& name,int value)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    unsigned long disposition = 0;
    if (RegCreateKeyExA(root, subKey.CharPtr(), 0, 0, 0, 0x000F003Fu, 0, &key, &disposition) == 0) {
        RegSetValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, 4,
                       reinterpret_cast<const unsigned char*>(&value), 4);
        RegCloseKey(key);
    }
}

// ZS1 retail 0x00442BE0..0x00442C61.
void REGISTRY::SetData(const STRING& name,const void* data,unsigned long size)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    unsigned long disposition = 0;
    if (RegCreateKeyExA(root, subKey.CharPtr(), 0, 0, 0, 0x000F003Fu, 0, &key, &disposition) == 0) {
        RegSetValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, 3,
                       static_cast<const unsigned char*>(data), size);
        RegCloseKey(key);
    }
}

// ZS1 retail 0x004427A0..0x00442958.
STRING REGISTRY::GetString(const STRING& name,const STRING& defaultString)
{
    HKEY__* root=0;
    STRING subKey=path.SplitRegPath(&root);
    HKEY__* key=0;
    if (RegOpenKeyExA(root,subKey.CharPtr(),0,1,&key)!=0)
        return defaultString;
    unsigned long size=0x1FF;
    unsigned long type=0;
    RegQueryValueExA(key,const_cast<STRING&>(name).CharPtr(),0,&type,
                     reinterpret_cast<unsigned char*>(g_registryStringBuffer),&size);
    RegCloseKey(key);

    // ZS1 accepts string values directly and converts DWORD/binary values to decimal text.
    if (type==4 || type==3)
        _itoa(*reinterpret_cast<int*>(g_registryStringBuffer),g_registryStringBuffer,10);
    if ((type==1 || type==4 || type==3) && g_registryStringBuffer[0])
        return STRING(g_registryStringBuffer);
    return defaultString;
}

// ZS1 retail 0x00442AC0..0x00442B4D.
void REGISTRY::SetString(const STRING& name,const STRING& value)
{
    HKEY__* root=0;
    STRING subKey=path.SplitRegPath(&root);
    HKEY__* key=0;
    unsigned long disposition=0;
    if (RegCreateKeyExA(root,subKey.CharPtr(),0,0,0,0x000F003Fu,0,&key,&disposition)==0) {
        const unsigned long size=static_cast<unsigned long>(strlen(value.m_buf)+1u);
        RegSetValueExA(key,const_cast<STRING&>(name).CharPtr(),0,1,
                       reinterpret_cast<const unsigned char*>(value.m_buf),size);
        RegCloseKey(key);
    }
}

// ZS1 retail 0x00442C70..0x00442CD9.
void REGISTRY::Delete(const STRING& name)
{
    HKEY__* root=0;
    STRING subKey=path.SplitRegPath(&root);
    HKEY__* key=0;
    if (RegOpenKeyExA(root,subKey.CharPtr(),0,0x000F003Fu,&key)==0) {
        RegDeleteValueA(key,const_cast<STRING&>(name).CharPtr());
        RegCloseKey(key);
    }
}
