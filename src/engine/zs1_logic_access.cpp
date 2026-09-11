#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"


namespace zs1 { namespace logic {
namespace {


const char* RetailString(const STRING_LAYOUT32& value)
{
    return reinterpret_cast<const char*>(static_cast<uintptr_t>(value.ptr));
}

void FreeRetailString(STRING_LAYOUT32& value)
{
    if (value.ptr && value.ptr != RetailEmptyStringAddress())
        zs1::engine::RetailOperatorDelete(
            reinterpret_cast<void*>(static_cast<uintptr_t>(value.ptr)));
    value.ptr = RetailEmptyStringAddress();
}

void AssignCString(STRING_LAYOUT32* out, const char* text)
{
    if (out->ptr && out->ptr != RetailEmptyStringAddress())
        zs1::engine::RetailOperatorDelete(
            reinterpret_cast<void*>(static_cast<uintptr_t>(out->ptr)));
    if (!text || !*text) {
        out->ptr = RetailEmptyStringAddress();
        return;
    }

    const size_t len = strlen(text);
    const size_t bytes = (len & ~size_t(0x0F)) + 0x10;
    char* dst = static_cast<char*>(zs1::engine::RetailOperatorNew(bytes));
    if (!dst) {
        out->ptr = RetailEmptyStringAddress();
        return;
    }
    memcpy(dst, text, len);
    dst[len] = 0;
    out->ptr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst));
}

void CopyCStringResult(char** out, const char* text)
{
    if (!text || !*text) {
        *out = reinterpret_cast<char*>(static_cast<uintptr_t>(RetailEmptyStringAddress()));
        return;
    }
    const size_t len = strlen(text);
    const size_t bytes = (len & ~size_t(0x0F)) + 0x10;
    char* dst = static_cast<char*>(zs1::engine::RetailOperatorNew(bytes));
    if (!dst) {
        *out = reinterpret_cast<char*>(static_cast<uintptr_t>(RetailEmptyStringAddress()));
        return;
    }
    memcpy(dst, text, len);
    dst[len] = 0;
    *out = dst;
}

int FindVariable(const LOGIC_LAYOUT32* logic, const char* name)
{
    const NAMED_LOGICVAR_ENTRY_LAYOUT32* entries = reinterpret_cast<const NAMED_LOGICVAR_ENTRY_LAYOUT32*>(
        static_cast<uintptr_t>(logic->variables.data));
    for (int i = logic->variables.count - 1; i >= 0; --i) {
        const char* current = RetailString(entries[i].name);
        if (current && strcmp(current, name) == 0)
            return i;
    }
    return -1;
}

void SplitBeforeBracket(STRING_LAYOUT32* out, const char* text)
{
    const char* bracket = text ? strchr(text, '[') : nullptr;
    const size_t len = bracket ? static_cast<size_t>(bracket - text)
                                    : (text ? strlen(text) : 0);
    if (!len) {
        out->ptr = RetailEmptyStringAddress();
        return;
    }
    const size_t bytes = (len & ~size_t(0x0F)) + 0x10;
    char* dst = static_cast<char*>(zs1::engine::RetailOperatorNew(bytes));
    memcpy(dst, text, len);
    dst[len] = 0;
    out->ptr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst));
}

int ParseBracketIndex(const char* text)
{
    const char* bracket = text ? strchr(text, '[') : nullptr;
    if (!bracket)
        return 0;
    const char* value = bracket + 1;
    int index = 0;
    if (value[1] == 'x')
        sscanf(value, "%i", &index);
    else
        index = atoi(value);
    return index;
}

} // namespace

char** LOGIC_LAYOUT32::GetVariableStr(char** out, const STRING_LAYOUT32& name)
{
    STRING_LAYOUT32 baseName = { RetailEmptyStringAddress() };
    const char* fullName = RetailString(name);
    SplitBeforeBracket(&baseName, fullName);

    const int variable = FindVariable(this, RetailString(baseName));
    FreeRetailString(baseName);

    if (variable < 0) {
        MYERROR::Log(::Error,"!!!ERROR!!! SCRIPT Can't find variable '%s' in GetVariableString",fullName);
        CopyCStringResult(out, g_RetailLogicErrorText);
        return out;
    }

    NAMED_LOGICVAR_ENTRY_LAYOUT32* entries = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(
        static_cast<uintptr_t>(variables.data));
    NAMED_LOGICVAR_ENTRY_LAYOUT32& entry = entries[variable];
    if (entry.var.flag != 1) {
        MYERROR::Log(::Error,"!!!ERROR!!! SCRIPT Can't find variable '%s' in GetVariableString",fullName);
        CopyCStringResult(out, g_RetailLogicErrorText);
        return out;
    }

    const int index = ParseBracketIndex(fullName);
    if (index >= entry.var.extra) {
        CopyCStringResult(out, g_RetailLogicErrorText);
        return out;
    }

    LOGICSTACK_LAYOUT32* values = reinterpret_cast<LOGICSTACK_LAYOUT32*>(
        static_cast<uintptr_t>(stack.data));
    LOGICSTACK_LAYOUT32& value = values[entry.var.a + index];

    if (value.type & 2) {
        char decimal[32];
        sprintf(decimal, "%d", value.number);
        STRING_LAYOUT32 temporary = { RetailEmptyStringAddress() };
        AssignCString(&temporary, decimal);
        STRING_LAYOUT32 destination = { value.stringPtr };
        AssignRetailString(&destination, &temporary);
        value.stringPtr = destination.ptr;
        FreeRetailString(temporary);
    }

    CopyCStringResult(out, reinterpret_cast<const char*>(
        static_cast<uintptr_t>(value.stringPtr)));
    return out;
}

} } // namespace zs1::logic
