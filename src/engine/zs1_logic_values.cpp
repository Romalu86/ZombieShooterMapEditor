#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"


namespace zs1 { namespace logic {
namespace {

inline const char* Chars(uint32_t p)
{
    return reinterpret_cast<const char*>(static_cast<uintptr_t>(p));
}
inline uint32_t CopyCString(const char* text)
{
    if (!text || !*text)
        return RetailEmptyStringAddress();
    const size_t len = strlen(text);
    const size_t bytes = (len & ~size_t(0x0F)) + 0x10;
    char* out = static_cast<char*>(zs1::engine::RetailOperatorNew(bytes));
    if (!out)
        return 0;
    memcpy(out, text, len);
    out[len] = 0;
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(out));
}

inline void ReplaceString(uint32_t& dst, const char* text)
{
    STRING_LAYOUT32 source = { CopyCString(text) };
    STRING_LAYOUT32 target = { dst };
    AssignRetailString(&target, &source);
    if (source.ptr && source.ptr != RetailEmptyStringAddress())
        zs1::engine::RetailOperatorDelete(reinterpret_cast<void*>(static_cast<uintptr_t>(source.ptr)));
    dst = target.ptr;
}

inline int ParseRetailInt(const char* text)
{
    if (text[1] != 'x')
        return atoi(text);
    int value = 0;
    sscanf(text, "%i", &value);
    return value;
}

inline int NumericValueNoValidation(const LOGICSTACK_LAYOUT32& value)
{
    return (value.type & 1) ? ParseRetailInt(Chars(value.stringPtr)) : value.number;
}
} // namespace

STRING_LAYOUT32* RetailInt2Str(STRING_LAYOUT32* out, int value)
{
    char buffer[128];
    sprintf(buffer, "%d", value);
    out->ptr = CopyCString(buffer);
    return out;
}

LOGICSTACK_LAYOUT32::LOGICSTACK_LAYOUT32()
{
    type = 0;
    number = 0;
    stringPtr = RetailEmptyStringAddress();
}

LOGICSTACK_LAYOUT32::LOGICSTACK_LAYOUT32(int value)
{
    type = 2;
    number = value;
    stringPtr = RetailEmptyStringAddress();
}

LOGICSTACK_LAYOUT32::LOGICSTACK_LAYOUT32(const void* object, const STRING_LAYOUT32& context)
{
    (void)context; // proven x86 argument; current retail body does not read it
    type = object ? 0x12 : 0x02;
    number = static_cast<int32_t>(reinterpret_cast<uintptr_t>(object));
    stringPtr = RetailEmptyStringAddress();
}

LOGICSTACK_LAYOUT32::LOGICSTACK_LAYOUT32(const STRING_LAYOUT32& value)
{
    type = 1;
    // Retail string-value construction leaves the numeric payload unspecified.
    // Do not normalize it: type bit 1 makes stringPtr the active value.
    stringPtr = RetailEmptyStringAddress();
    STRING_LAYOUT32 target = { stringPtr };
    AssignRetailString(&target, &value);
    stringPtr = target.ptr;
}

LOGICSTACK_LAYOUT32::LOGICSTACK_LAYOUT32(const LOGICSTACK_LAYOUT32& other)
{
    type = other.type;
    number = other.number;
    stringPtr = CopyCString(Chars(other.stringPtr));
}

LOGICSTACK_LAYOUT32& LOGICSTACK_LAYOUT32::operator=(const LOGICSTACK_LAYOUT32& other)
{
    if (this != &other) {
        type = static_cast<uint8_t>(other.type & 0x7Fu);
        number = other.number;
        STRING_LAYOUT32 source = { other.stringPtr };
        STRING_LAYOUT32 target = { stringPtr };
        AssignRetailString(&target, &source);
        stringPtr = target.ptr;
    }
    return *this;
}

STRING_LAYOUT32* LOGICSTACK_LAYOUT32::String()
{
    if (type & 2) {
        STRING_LAYOUT32 converted = { 0 };
        RetailInt2Str(&converted, number);
        STRING_LAYOUT32 target = { stringPtr };
        AssignRetailString(&target, &converted);
        stringPtr = target.ptr;
        if (converted.ptr && converted.ptr != RetailEmptyStringAddress())
            zs1::engine::RetailOperatorDelete(
                reinterpret_cast<void*>(static_cast<uintptr_t>(converted.ptr)));
    }
    return reinterpret_cast<STRING_LAYOUT32*>(&stringPtr);
}

int LOGICSTACK_LAYOUT32::Int()
{
    if (type & 1) {
        const char* text = Chars(stringPtr);
        const unsigned char c0 = static_cast<unsigned char>(text[0]);
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        if (!(c0 >= '0' && c0 <= '9') && !(c0 == '-' && c1 >= '0' && c1 <= '9'))
            MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: \xEF\xF0\xE8\xF1\xE2\xE0\xE8\xE2\xE0\xED\xE8\xE5 \xEF\xE5\xF0\xE5\xEC\xE5\xED\xED\xEE\xE9 int \xF1\xF2\xF0\xEE\xEA\xE8 '%s'",text);
        return ParseRetailInt(text);
    }
    return number;
}

void LOGICSTACK_LAYOUT32::Inc()
{
    type = static_cast<uint8_t>(type & ~0x10u);
    if (type & 0x22)
        ++number;
}

void LOGICSTACK_LAYOUT32::Dec()
{
    type = static_cast<uint8_t>(type & ~0x10u);
    if (type & 0x22)
        --number;
}

void LOGICSTACK_LAYOUT32::BinarOperator(int operation, const LOGICSTACK_LAYOUT32& other)
{
    // Retail handles string/string concatenation and equality before numeric coercion.
    if ((type & 1) && (other.type & 1)) {
        if (operation == 8) {
            const char* a = Chars(stringPtr);
            const char* b = Chars(other.stringPtr);
            const size_t la = strlen(a);
            const size_t lb = strlen(b);
            char* temp = static_cast<char*>(zs1::engine::RetailOperatorNew(la + lb + 1));
            if (temp) {
                if (la) memcpy(temp, a, la);
                if (lb) memcpy(temp + la, b, lb);
                temp[la + lb] = 0;
                ReplaceString(stringPtr, temp);
                zs1::engine::RetailOperatorDelete(temp);
            }
            type = 1;
            return;
        }
        if (operation == 13 || operation == 20) {
            const int cmp = strcmp(Chars(stringPtr), Chars(other.stringPtr));
            number = (operation == 13) ? (cmp == 0) : (cmp != 0);
            type = 2;
            return;
        }
    }

    const int rhs = NumericValueNoValidation(other);
    if (type & 1)
        number = NumericValueNoValidation(*this);

    switch (operation) {
    case 8:  number += rhs; break;
    case 9:  number -= rhs; break;
    case 19: number *= rhs; break;
    case 6:  number = rhs ? number / rhs : 0x0FFFFFFF; break;
    case 7:  number %= rhs; break;
    case 11: number |= rhs; break;
    case 10: number ^= rhs; break;
    case 12: number &= rhs; break;
    case 23: number = static_cast<int>(static_cast<unsigned>(number) << (static_cast<unsigned>(rhs) & 31u)); break;
    case 22: number >>= (static_cast<unsigned>(rhs) & 31u); break;
    case 21: number = (number && rhs) ? 1 : 0; break;
    case 14: number = (number || rhs) ? 1 : 0; break;
    case 16: number = number < rhs; break;
    case 18: number = number <= rhs; break;
    case 15: number = number > rhs; break;
    case 17: number = number >= rhs; break;
    case 13: number = number == rhs; break;
    case 20: number = number != rhs; break;
    default:
        MYERROR::Log(::Error,"!!!ERROE!!!SCRIPT::Unknown Binary command %i",operation);
        break;
    }
    type = 2;
}

NAMED_LOGICVAR_ENTRY_LAYOUT32::NAMED_LOGICVAR_ENTRY_LAYOUT32()
{
    name.ptr = RetailEmptyStringAddress();
    var.flag = 0;
    var.auxFlags = 0;
    var.value.ptr = RetailEmptyStringAddress();
}

NAMED_LOGICVAR_ENTRY_LAYOUT32& NAMED_LOGICVAR_ENTRY_LAYOUT32::operator=(
    const NAMED_LOGICVAR_ENTRY_LAYOUT32& other)
{
    AssignRetailString(&name, &other.name);
    var.flag = other.var.flag;
    var.auxFlags = other.var.auxFlags;
    AssignRetailString(&var.value, &other.var.value);
    var.a = other.var.a;
    var.type = other.var.type;
    var.extra = other.var.extra;
    return *this;
}

void LOGICSTACK_LAYOUT32::Read(void* stream)
{
    // Retail always consumes the one-byte type first through STREAM::Read.
    RetailStreamRead(stream, this, 1);
    if ((type & 8) == 0)
        return;

    if (type & 1) {
        STRING_LAYOUT32 value = { stringPtr };
        RetailStringReadRes(&value, stream);
        stringPtr = value.ptr;
        char* text = reinterpret_cast<char*>(static_cast<uintptr_t>(stringPtr));
        const size_t length = strlen(text);
        // 0x439657..0x439673 walks backwards and XORs every persisted byte.
        for (size_t i = length; i != 0; --i)
            text[i - 1] ^= 0x17;
        return;
    }

    RetailStreamRead(stream, &number, 4);
}


void LOGIC_LAYOUT32::PushStr(const STRING_LAYOUT32& value)
{
    LOGICSTACK_LAYOUT32 item(value);
    stack.Insert(item);
}

void LOGIC_LAYOUT32::PushInt(int value)
{
    LOGICSTACK_LAYOUT32 item(value);
    stack.Insert(item);
}

void LOGIC_LAYOUT32::PushObject(const void* object, const STRING_LAYOUT32& context)
{
    LOGICSTACK_LAYOUT32 item(object, context);
    stack.Insert(item);
}

} } // namespace zs1::logic
