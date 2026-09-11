#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"

namespace {

inline unsigned char* ByteCode(LOGIC* logic)
{
    return logic->byteCode;
}

inline const char* Source(const LOGIC* logic)
{
    return logic->pos;
}

inline bool RetailIsAlpha(unsigned char c)
{
    return isalpha(static_cast<int>(c)) != 0;
}

inline bool RetailIsAlnum(unsigned char c)
{
    return isalnum(static_cast<int>(c)) != 0;
}

inline const char* DefineValue(const LOGIC* logic, int index)
{
    return logic->strings.m_data[index].val.m_buf;
}

inline void ReportLogicError(LOGIC* logic, int type, const char* word, int detail)
{
    STRING text(word ? word : "");
    logic->Error(type, &text, detail);
}

} // namespace

// Recovery name: the original source-level helper spelling is not present in the EXE.
void LOGIC::EmitByteIfNoError(uint8_t opcode)
{
    if (compileError)
        return;
    ByteCode(this)[byteCodeSize] = opcode;
    ++byteCodeSize;
}

// Exact 5-byte bytecode emitter used throughout the compiler.
void LOGIC::EmitByteInt32(uint8_t opcode, int value)
{
    unsigned char* code = ByteCode(this);
    code[byteCodeSize] = opcode;
    ++byteCodeSize;
    *reinterpret_cast<int*>(code + byteCodeSize) = value;
    byteCodeSize += 4;
}

int LOGIC::skipempty2()
{
    if (compileError)
        return 1;

    int comment = 0;
    int skipDepth = 0;
    char token[4096];

    while (pos < end) {
        if (compileError)
            break;
        char* p = pos;

        if (!comment) {
            if ((RetailIsAlpha(static_cast<unsigned char>(*p)) || *p == '_') && !parserMode) {
                int length = 0;
                while (RetailIsAlnum(static_cast<unsigned char>(p[length])) || p[length] == '_') {
                    if (length >= 4095) {
                        ReportLogicError(this, 10, "Very long name", 0);
                        ++length;
                        continue;
                    }
                    token[length] = p[length];
                    ++length;
                }
                token[length] = 0;
                STRING key(token);
                const int define = strings.Location(&key);
                if (define >= 0) {
                    const char* replacement = DefineValue(this, define);
                    const int replacementLength = static_cast<int>(strlen(replacement));
                    pos += (length - replacementLength);
                    p = pos;
                    memcpy(p, replacement, static_cast<size_t>(replacementLength));
                }
            }

            p = pos;
            if (p[0]=='#' && p[1]=='i' && p[2]=='f' && p[3]=='d' && p[4]=='e' && p[5]=='f') {
                pos += 6;
                ++parserFlags54;
                if (!skipDepth) {
                    STRING nameValue;
                    parserMode = 1;
                    GetName(&nameValue);
                    parserMode = 0;
                    if (variables.Location(&nameValue) < 0 && strings.Location(&nameValue) < 0)
                        skipDepth = parserFlags54;
                }
            }
            else if (p[0]=='#' && p[1]=='i' && p[2]=='f' && p[3]=='n' && p[4]=='d' && p[5]=='e' && p[6]=='f') {
                pos += 7;
                ++parserFlags54;
                if (!skipDepth) {
                    STRING nameValue;
                    parserMode = 1;
                    GetName(&nameValue);
                    parserMode = 0;
                    if (variables.Location(&nameValue) >= 0 || strings.Location(&nameValue) >= 0)
                        skipDepth = parserFlags54;
                }
            }
            else if (p[0]=='#' && p[1]=='e' && p[2]=='n' && p[3]=='d' && p[4]=='i' && p[5]=='f') {
                pos += 6;
                if (skipDepth == parserFlags54)
                    skipDepth = 0;
                --parserFlags54;
                if (parserFlags54 < 0)
                    ReportLogicError(this, 10, "#endif without #ifdef", 0);
            }
            else if (p[0]=='#' && p[1]=='e' && p[2]=='l' && p[3]=='s' && p[4]=='e') {
                pos += 5;
                if (!skipDepth && parserFlags54 > 0)
                    skipDepth = parserFlags54;
                else if (skipDepth == parserFlags54)
                    skipDepth = 0;
                if (parserFlags54 <= 0)
                    ReportLogicError(this, 10, "#else without #ifdef", 0);
            }

            p = pos;
            if (p[0] == '/' && p[1] == '/')
                comment = 1;
            else if (p[0] == '/' && p[1] == '*')
                comment = 2;
            else {
                if (*p == '?')
                    ReportLogicError(this, 10, "?: not supported in this version", 0);
                if (!skipDepth && !isspace(static_cast<unsigned char>(*p)) && *p)
                    return 0;
            }
        }
        else {
            p = pos;
            if ((comment == 1 && *p == '\n') ||
                (comment == 2 && *p == '/' && p[-1] == '*'))
                comment = 0;
        }

        p = pos;
        const char c = *p;
        ++pos;
        if (c == '\n')
            ++line;
    }

    if (parserFlags54 > 0)
        ReportLogicError(this, 10, "#ifdef without #endif", parserFlags54);
    return 1;
}

int LOGIC::skipempty()
{
    if (compileError)
        return compileError;
    const int result = skipempty2();
    if (result)
        ReportLogicError(this, 10, "End of file", 0);
    return result;
}

int LOGIC::GetLine(STRING* out)
{
    if (compileError)
        return 0;
    skipempty();
    if (compileError)
        return 0;

    char buf[4096];
    int n = 0;
    while (pos < end) {
        char* p = pos;
        const char c = *p;
        if (c == '\n' || c == '\r')
            break;
        if (n >= 4095) {
            ReportLogicError(this, 10, "Very long line", 0);
            break;
        }
        buf[n++] = c;
        ++pos;
    }
    buf[n] = 0;
    *out = buf;
    if (!buf[0])
        ReportLogicError(this, 10, "empty line", 0);

    // Retail source-family keeps comment stripping and trailing-whitespace
    // removal in STRING owners; target ASM calls those owners from GetLine.
    *out = out->Before("//");
    out->RemoveEndChars(" \n\r\t");
    return skipempty();
}

int LOGIC::GetName(STRING* out)
{
    if (compileError)
        return 0;
    skipempty();
    if (compileError)
        return 0;

    char buf[4096];
    int n = 0;
    while (pos < end) {
        const char* p = Source(this);
        const unsigned char c = static_cast<unsigned char>(*p);
        if (!RetailIsAlnum(c) && c != '_')
            break;
        if (n >= 4095) {
            ReportLogicError(this, 10, "Very long name", 0);
            ++pos;
            ++n;
            continue;
        }
        buf[n++] = *p;
        ++pos;
    }
    buf[n < 4096 ? n : 4095] = 0;
    *out = buf;
    if (!buf[0])
        ReportLogicError(this, 4, "name", 0);
    skipempty();
    return n;
}

int LOGIC::Word(const char* word)
{
    const size_t len = strlen(word);
    skipempty();
    if (compileError)
        return 0;

    const char* source = Source(this);

    // Retail calls the same bounded compare helper twice on the success route.
    // Keeping both calls preserves even this redundant MSVC6 source behaviour.
    if (strncmp(source, word, len) != 0)
        return 0;
    if (strncmp(source, word, len) != 0)
        return 0;

    const unsigned char first = static_cast<unsigned char>(word[0]);
    if (RetailIsAlpha(first) || first == '#') {
        const unsigned char next = static_cast<unsigned char>(source[len]);
        if (RetailIsAlnum(next) || next == '_')
            return 0;
    }

    if (len == 1 && source[1] == word[0] && strchr("-+|&=", word[0]))
        return 0;

    pos += static_cast<int>(len);
    // Target calls skipempty2 directly here, not the EOF-reporting wrapper.
    skipempty2();
    return 1;
}

int LOGIC::WordEnd(const char* word)
{
    if (compileError)
        return 0;
    if (Word(word))
        return 1;
    ReportLogicError(this, 13, word ? word : "", 0);
    return 0;
}

int LOGIC::GetInt()
{
    if (compileError)
        return 0;

    skipempty();
    const int start = byteCodeSize;
    CompileExpression(1);

    unsigned char* code = ByteCode(this);
    if (code[start] != 1 || byteCodeSize - start != 5)
        ReportLogicError(this, 4, "constant int value", 0);

    // Retail still rewinds/reads the 5-byte record after reporting the error.
    byteCodeSize -= 5;
    return *reinterpret_cast<int*>(code + byteCodeSize + 1);
}

// Retail returns STRING by value; MSVC6 therefore emits the hidden return-buffer
// ABI seen at 0x004330C0 (ret 4).  Keeping the real C++ return type is required
// for both readable ownership and the target constructor/destructor source shape.
STRING LOGIC::GetConstantString()
{
    skipempty();
    if (compileError)
        return STRING(zs1::logic::g_RetailLogicErrorText);

    const int start = byteCodeSize;
    int encodedStringBytes = 0;
    CompileExpression(1);

    unsigned char* code = ByteCode(this);
    if (code[start] == 2)
        encodedStringBytes = static_cast<int>(strlen(reinterpret_cast<const char*>(code + start + 1))) + 1;

    if (code[start] != 2 || byteCodeSize - start != encodedStringBytes + 1)
        ReportLogicError(this, 4, "constant string value", 0);

    byteCodeSize -= encodedStringBytes + 1;
    return STRING(reinterpret_cast<const char*>(code + byteCodeSize + 1));
}

int LOGIC::GetString(char* out)
{
    if (compileError)
        return 0;

    char* start = out;
    char* p = pos;
    if (*p == '"') {
        ++pos;
        while (pos < end && *pos != '"') {
            p = pos;
            if (*p == '\\') {
                const char escaped = p[1];
                if (escaped == '\r' && p[2] == '\n') {
                    ++line;
                    pos += 2;
                }
                else if (escaped == '\n') {
                    ++line;
                    ++pos;
                }
                else if (isdigit(static_cast<unsigned char>(p[1])) &&
                         isdigit(static_cast<unsigned char>(p[2])) &&
                         isdigit(static_cast<unsigned char>(p[3]))) {
                    *out++ = static_cast<char>((((p[1] - '0') * 8 + p[2] - '0') * 8) + p[3] - '0');
                    pos += 3;
                }
                else if (escaped == 'n') { *out++ = '\n'; ++pos; }
                else if (escaped == 'r') { *out++ = '\r'; ++pos; }
                else if (escaped == 't') { *out++ = '\t'; ++pos; }
                else if (escaped == '\'') { *out++ = '\''; ++pos; }
                else if (escaped == '\\') { *out++ = '\\'; ++pos; }
                else if (escaped == 0) { *out++ = 0; ++pos; }
                else if (escaped == '"') { *out++ = '"'; ++pos; }
                else {
                    ReportLogicError(this, 14, "symbol after '\\\'", 0);
                    ++pos;
                    *out++ = *pos;
                }
            }
            else {
                *out++ = *p;
            }
            ++pos;
        }
        char* close = pos++;
        if (close >= end)
            ReportLogicError(this, 10, "End of file", 0);
        *out++ = 0;
    }
    return static_cast<int>(out - start);
}

int LOGIC::SetNoElement(int value)
{
    const int result = 3 * variables.m_no;
    unsigned char* data = reinterpret_cast<unsigned char*>(variables.m_data);
    *reinterpret_cast<int*>(data + result * 8 - 4) = value;
    return result;
}

