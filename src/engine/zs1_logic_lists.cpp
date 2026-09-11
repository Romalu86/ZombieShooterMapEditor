#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"


namespace zs1 { namespace logic {
namespace {


inline void DestroyOwnedString(uint32_t& value)
{
    if (value && value != RetailEmptyStringAddress())
        zs1::engine::RetailOperatorDelete(
            reinterpret_cast<void*>(static_cast<uintptr_t>(value)));
    value = RetailEmptyStringAddress();
}

inline LOGICSTACK_LAYOUT32* AllocateStackArray(int count)
{
    const size_t bytes = 4u + static_cast<size_t>(count) * sizeof(LOGICSTACK_LAYOUT32);
    uint8_t* raw = static_cast<uint8_t*>(zs1::engine::RetailOperatorNew(bytes));
    if (!raw)
        return nullptr;
    *reinterpret_cast<int*>(raw) = count;
    LOGICSTACK_LAYOUT32* values = reinterpret_cast<LOGICSTACK_LAYOUT32*>(raw + 4);
    for (int i = 0; i < count; ++i)
        new (values + i) LOGICSTACK_LAYOUT32();
    return values;
}

inline void DestroyStackArray(LOGICSTACK_LAYOUT32* values)
{
    if (!values)
        return;
    uint8_t* raw = reinterpret_cast<uint8_t*>(values) - 4;
    const int count = *reinterpret_cast<int*>(raw);
    for (int i = count - 1; i >= 0; --i)
        DestroyOwnedString(values[i].stringPtr);
    zs1::engine::RetailOperatorDelete(raw);
}

inline NAMED_LOGICVAR_ENTRY_LAYOUT32* AllocateVariableArray(int count)
{
    const size_t bytes = 4u + static_cast<size_t>(count) * sizeof(NAMED_LOGICVAR_ENTRY_LAYOUT32);
    uint8_t* raw = static_cast<uint8_t*>(zs1::engine::RetailOperatorNew(bytes));
    if (!raw)
        return nullptr;
    *reinterpret_cast<int*>(raw) = count;
    NAMED_LOGICVAR_ENTRY_LAYOUT32* values = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(raw + 4);
    for (int i = 0; i < count; ++i)
        new (values + i) NAMED_LOGICVAR_ENTRY_LAYOUT32();
    return values;
}

inline void DestroyVariableArray(NAMED_LOGICVAR_ENTRY_LAYOUT32* values)
{
    if (!values)
        return;
    uint8_t* raw = reinterpret_cast<uint8_t*>(values) - 4;
    const int count = *reinterpret_cast<int*>(raw);
    for (int i = count - 1; i >= 0; --i) {
        DestroyOwnedString(values[i].var.value.ptr);
        DestroyOwnedString(values[i].name.ptr);
    }
    zs1::engine::RetailOperatorDelete(raw);
}

inline NAMED_STRING_ENTRY_LAYOUT32* AllocateNamedStringArray(int count)
{
    const size_t bytes = 4u + static_cast<size_t>(count) * sizeof(NAMED_STRING_ENTRY_LAYOUT32);
    uint8_t* raw = static_cast<uint8_t*>(zs1::engine::RetailOperatorNew(bytes));
    if (!raw)
        return nullptr;
    *reinterpret_cast<int*>(raw) = count;
    NAMED_STRING_ENTRY_LAYOUT32* values = reinterpret_cast<NAMED_STRING_ENTRY_LAYOUT32*>(raw + 4);
    for (int i = 0; i < count; ++i) {
        values[i].name.ptr = RetailEmptyStringAddress();
        values[i].value.ptr = RetailEmptyStringAddress();
    }
    return values;
}

inline void DestroyNamedStringArray(NAMED_STRING_ENTRY_LAYOUT32* values)
{
    if (!values)
        return;
    uint8_t* raw = reinterpret_cast<uint8_t*>(values) - 4;
    const int count = *reinterpret_cast<int*>(raw);
    for (int i = count - 1; i >= 0; --i) {
        DestroyOwnedString(values[i].value.ptr);
        DestroyOwnedString(values[i].name.ptr);
    }
    zs1::engine::RetailOperatorDelete(raw);
}

} // namespace

int LIST_LOGICSTACK_LAYOUT32::GetNo() const
{
    return count;
}

void NAMED_LIST_LOGICVAR_LAYOUT32::Insert(STRING_LAYOUT32 name, LOGICVAR_LAYOUT32 value)
{
    if (count >= capacity) {
        const int oldCapacity = capacity;
        const int newCapacity = oldCapacity * 2 + 4;
        if (newCapacity > oldCapacity) {
            NAMED_LOGICVAR_ENTRY_LAYOUT32* oldData = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(
                static_cast<uintptr_t>(data));
            NAMED_LOGICVAR_ENTRY_LAYOUT32* newData = AllocateVariableArray(newCapacity);
            data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(newData));
            if (!newData) {
                zs1::engine::ReportListOutOfMemory(newCapacity);
            }
            else {
                if (oldData) {
                    // Retail copies all constructed capacity slots, not merely count.
                    for (int i = 0; i < oldCapacity; ++i)
                        newData[i] = oldData[i];
                    DestroyVariableArray(oldData);
                }
                capacity = newCapacity;
            }
        }
    }

    NAMED_LOGICVAR_ENTRY_LAYOUT32* entries = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(
        static_cast<uintptr_t>(data));
    NAMED_LOGICVAR_ENTRY_LAYOUT32& dst = entries[count];
    AssignRetailString(&dst.name, &name);
    ++count;
    dst.var.flag = value.flag;
    dst.var.auxFlags = value.auxFlags;
    AssignRetailString(&dst.var.value, &value.value);
    dst.var.a = value.a;
    dst.var.type = value.type;
    dst.var.extra = value.extra;
}

void NAMED_LIST_STRING_LAYOUT32::Insert(STRING_LAYOUT32 name, STRING_LAYOUT32 value)
{
    if (count >= capacity) {
        const int oldCapacity = capacity;
        const int newCapacity = oldCapacity * 2 + 4;
        if (newCapacity > oldCapacity) {
            NAMED_STRING_ENTRY_LAYOUT32* oldData = reinterpret_cast<NAMED_STRING_ENTRY_LAYOUT32*>(
                static_cast<uintptr_t>(data));
            NAMED_STRING_ENTRY_LAYOUT32* newData = AllocateNamedStringArray(newCapacity);
            data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(newData));
            if (!newData) {
                zs1::engine::ReportListOutOfMemory(newCapacity);
            }
            else {
                if (oldData) {
                    for (int i = 0; i < oldCapacity; ++i) {
                        AssignRetailString(&newData[i].name, &oldData[i].name);
                        AssignRetailString(&newData[i].value, &oldData[i].value);
                    }
                    DestroyNamedStringArray(oldData);
                }
                capacity = newCapacity;
            }
        }
    }

    NAMED_STRING_ENTRY_LAYOUT32* entries = reinterpret_cast<NAMED_STRING_ENTRY_LAYOUT32*>(
        static_cast<uintptr_t>(data));
    AssignRetailString(&entries[count].name, &name);
    AssignRetailString(&entries[count].value, &value);
    ++count;
}

void NAMED_LIST_STRING_LAYOUT32::Release()
{
    capacity = 0;
    count = 0;
    NAMED_STRING_ENTRY_LAYOUT32* values = reinterpret_cast<NAMED_STRING_ENTRY_LAYOUT32*>(
        static_cast<uintptr_t>(data));
    if (values)
        DestroyNamedStringArray(values);
    data = 0;
}

int LIST_LOGICSTACK_LAYOUT32::DeletePointerToObject(void* object)
{
    int result = 0;
    LOGICSTACK_LAYOUT32* values = reinterpret_cast<LOGICSTACK_LAYOUT32*>(static_cast<uintptr_t>(data));
    const int32_t target = static_cast<int32_t>(reinterpret_cast<uintptr_t>(object));
    for (int i = 0; i < count; ++i) {
        if ((values[i].type & 0x10) && values[i].number == target) {
            values[i].number = 0;
            values[i].type = static_cast<uint8_t>(values[i].type & ~0x10u);
            ++result;
        }
    }
    return result;
}

void LIST_LOGICSTACK_LAYOUT32::Push(const LOGICSTACK_LAYOUT32& value)
{
    // Retail wrapper constructs a deep-copy temporary then enters the by-value Insert owner.
    LOGICSTACK_LAYOUT32 copy(value);
    Insert(copy);
}

void LIST_LOGICSTACK_LAYOUT32::Insert(LOGICSTACK_LAYOUT32 value)
{
    if (count >= capacity) {
        const int oldCapacity = capacity;
        const int newCapacity = oldCapacity * 2 + 4;
        if (newCapacity > oldCapacity) {
            LOGICSTACK_LAYOUT32* oldData = reinterpret_cast<LOGICSTACK_LAYOUT32*>(
                static_cast<uintptr_t>(data));
            LOGICSTACK_LAYOUT32* newData = AllocateStackArray(newCapacity);
            data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(newData));
            if (!newData) {
                zs1::engine::ReportListOutOfMemory(newCapacity);
            }
            else {
                if (oldData) {
                    // Retail copies all constructed capacity entries.
                    for (int i = 0; i < oldCapacity; ++i)
                        newData[i] = oldData[i];
                    DestroyStackArray(oldData);
                }
                capacity = newCapacity;
            }
        }
    }

    LOGICSTACK_LAYOUT32* values = reinterpret_cast<LOGICSTACK_LAYOUT32*>(static_cast<uintptr_t>(data));
    LOGICSTACK_LAYOUT32& dst = values[count++];
    dst.type = static_cast<uint8_t>(value.type & 0x7Fu);
    dst.number = value.number;
    STRING_LAYOUT32 src = { value.stringPtr };
    STRING_LAYOUT32 out = { dst.stringPtr };
    AssignRetailString(&out, &src);
    dst.stringPtr = out.ptr;
}

void LIST_LOGICSTACK_LAYOUT32::Expand(int newCapacity)
{
    if (newCapacity <= capacity)
        return;

    LOGICSTACK_LAYOUT32* oldData = reinterpret_cast<LOGICSTACK_LAYOUT32*>(static_cast<uintptr_t>(data));
    LOGICSTACK_LAYOUT32* newData = AllocateStackArray(newCapacity);
    data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(newData));
    if (!newData)
        zs1::engine::ReportListOutOfMemory(newCapacity);

    if (oldData) {
        for (int i = 0; i < capacity; ++i)
            newData[i] = oldData[i];
        DestroyStackArray(oldData);
    }
    capacity = newCapacity;
}

int LIST_LOGICSTACK_LAYOUT32::IsLastString() const
{
    const LOGICSTACK_LAYOUT32* values = reinterpret_cast<const LOGICSTACK_LAYOUT32*>(
        static_cast<uintptr_t>(data));
    // Retail intentionally has no empty-list guard here.
    return values[count - 1].type & 1;
}

} } // namespace zs1::logic
