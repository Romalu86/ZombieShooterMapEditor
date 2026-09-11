#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"

namespace {

inline unsigned char* CompilerByteCode(LOGIC* logic)
{
    return logic->byteCode;
}

const int kUnsizedArrayCount = 999999;

// GLOBAL: ZS1 0x004C8CB0.  Nested expression compilation observes the same
// pointer/array-argument state, so retail keeps this outside the individual owner.
int g_CompilerArrayArgumentContext = 0;

inline const char* CompilerSource(const LOGIC* logic)
{
    return logic->pos;
}

inline NAMED_LIST_STRUCT<LOGICVAR>* CompilerVariables(LOGIC* logic)
{
    return logic->variables.m_data;
}

inline NAMED_LIST_STRUCT<STRING>* CompilerDefines(LOGIC* logic)
{
    return logic->strings.m_data;
}

inline LOGICSTACK* CompilerStack(LOGIC* logic)
{
    return logic->stack.m_data;
}

inline void ReportLogicError(LOGIC* logic, int type, const char* word, int detail)
{
    STRING text(word ? word : "");
    logic->Error(type, &text, detail);
}

inline void RegisterActionVariable(LOGIC* logic, const char* name, int slot)
{
    if (!name || strncmp(name, "Action", 6) != 0)
        return;
    const char* suffix = name + 6;
    if (!*suffix)
        return;
    const bool numeric = isdigit(static_cast<unsigned char>(*suffix)) != 0 ||
                         (*suffix == '-' && isdigit(static_cast<unsigned char>(suffix[1])) != 0);
    if (!numeric)
        return;
    int action = 0;
    if (suffix[1] == 'x')
        sscanf(suffix, "%i", &action);
    else
        action = atoi(suffix);
    if (static_cast<unsigned int>(action) < 256u)
        logic->actionN[action] = slot;
}

inline void InsertCompilerVariable(LOGIC* logic, const STRING& name, int slot)
{
    LOGICVAR value(1, slot);
    logic->variables.Insert(name, value);
}

inline void EmitRuntimeInitializer(LOGIC* logic, int slot)
{
    logic->CompileExpression(1);
    logic->EmitByteInt32(0x26, slot);
    logic->EmitByteInt32(0x19, logic->line);
}

inline void EmitStringLiteral(LOGIC* logic, const char* text)
{
    logic->EmitByteIfNoError(2);
    if (logic->compileError)
        return;
    const size_t len = strlen(text ? text : "") + 1;
    memcpy(logic->byteCode + logic->byteCodeSize, text ? text : "", len);
    logic->byteCodeSize += static_cast<int>(len);
}

inline void EmitIntegerLiteral(LOGIC* logic, int value)
{
    logic->EmitByteInt32(1, value);
}

inline void MarkCalledFunctionResult(LOGIC* logic, NAMED_LIST_STRUCT<LOGICVAR>& entry)
{
    if (logic->parseContext)
        entry.val.auxFlags = static_cast<unsigned char>(entry.val.auxFlags | 1u);
}

inline void RestoreCompilerVariableScope(LOGIC* logic, int savedCount)
{
    if (savedCount <= 0) {
        logic->variables.m_max = 0;
        logic->variables.m_no = 0;
        delete[] logic->variables.m_data;
        logic->variables.m_data = 0;
    }
    else if (savedCount < logic->variables.m_no) {
        logic->variables.m_no = savedCount;
    }
}

inline void RestoreCompilerStackScope(LOGIC* logic, int savedCount)
{
    if (savedCount <= 0) {
        logic->stack.m_max = 0;
        logic->stack.m_no = 0;
        delete[] logic->stack.m_data;
        logic->stack.m_data = 0;
    }
    else if (savedCount < logic->stack.m_no) {
        logic->stack.m_no = savedCount;
    }
}

inline void PatchBreakFixups(LOGIC* logic, int* fixups)
{
    if (!fixups)
        return;
    for (int* p = fixups; *p; ++p)
        *reinterpret_cast<int*>(logic->byteCode + *p) = logic->byteCodeSize - *p;
}

} // namespace

void LOGIC::IntVar(int declarationMode)
{
    int count = 1;
    int flags = 0;
    STRING name;

    skipempty();
    if (compileError)
        return;

    if (*CompilerSource(this) == '*') {
        flags = 0x20;
        ++pos;
    }

    GetName(&name);
    const char* nameText = name.m_buf;
    if (variables.Location(&name) >= 0) {
        STRING text = Printf("int redefinition '%s'", nameText);
        Error(10, &text, 0);
    }
    if (compileError)
        return;

    const int slot = stack.m_no;
    RegisterActionVariable(this, nameText, slot);
    InsertCompilerVariable(this, name, slot);

    if (Word("[")) {
        flags |= 4;
        if (*CompilerSource(this) == ']')
            count = kUnsizedArrayCount;
        else
            count = GetInt();
        WordEnd("]");
    }

    int initialized = 0;
    if (Word("=")) {
        if (flags & 4) {
            WordEnd("{");
            for (;;) {
                if (count != kUnsizedArrayCount && initialized >= count) {
                    ReportLogicError(this, 10, "too many initializers", 0);
                    break;
                }

                if (declarationMode == 0) {
                    {
                        STRING context(zs1::logic::g_RetailLogicErrorText);
                        LOGICSTACK stackValue(0, &context);
                        stackValue.type = static_cast<uint8_t>(flags | 0xC2);
                        stack.Push(&stackValue);
                    }
                    EmitRuntimeInitializer(this, slot + initialized);
                }
                else {
                    const int value = GetInt();
                    {
                        STRING context(zs1::logic::g_RetailLogicErrorText);
                        LOGICSTACK stackValue(value, &context);
                        stackValue.type = static_cast<uint8_t>(flags | 0x8A);
                        stack.Push(&stackValue);
                    }
                }
                ++initialized;

                if (!Word(","))
                    break;
                if (compileError)
                    break;
            }
            WordEnd("}");
            if (count == kUnsizedArrayCount)
                count = initialized;
        }
        else {
            if (declarationMode == 0) {
                {
                    STRING context(zs1::logic::g_RetailLogicErrorText);
                    LOGICSTACK stackValue(0, &context);
                    stackValue.type = static_cast<uint8_t>(flags | 0xC2);
                    stack.Push(&stackValue);
                }
                EmitRuntimeInitializer(this, slot);
            }
            else {
                const int value = GetInt();
                {
                    STRING context(zs1::logic::g_RetailLogicErrorText);
                    LOGICSTACK stackValue(value, &context);
                    stackValue.type = static_cast<uint8_t>(flags | 0x8A);
                    stack.Push(&stackValue);
                }
            }
            initialized = 1;
            count = 1;
        }
    }
    else if (count == kUnsizedArrayCount) {
        ReportLogicError(this, 10, "for [] need initialisation", 0);
    }

    for (int i = initialized; i < count && !compileError; ++i) {
        STRING context(zs1::logic::g_RetailLogicErrorText);
        LOGICSTACK stackValue(0, &context);
        stackValue.type = static_cast<uint8_t>(flags | 0xC2);
        stack.Push(&stackValue);
    }

    SetNoElement(count);
}

void LOGIC::StringVar(int declarationMode)
{
    int count = 1;
    int flags = 0;
    STRING name;

    skipempty();
    if (compileError)
        return;

    if (*CompilerSource(this) == '*') {
        flags = 0x20;
        ++pos;
    }

    GetName(&name);
    const char* nameText = name.m_buf;
    if (variables.Location(&name) >= 0) {
        STRING text = Printf("string redefinition '%s'", nameText);
        Error(10, &text, 0);
    }
    if (compileError)
        return;

    const int slot = stack.m_no;
    InsertCompilerVariable(this, name, slot);

    if (Word("[")) {
        flags |= 4;
        if (*CompilerSource(this) == ']')
            count = kUnsizedArrayCount;
        else
            count = GetInt();
        WordEnd("]");
    }

    int initialized = 0;
    if (Word("=")) {
        if (flags & 4) {
            WordEnd("{");
            for (;;) {
                if (count != kUnsizedArrayCount && initialized >= count) {
                    ReportLogicError(this, 10, "too many initializers", 0);
                    break;
                }

                if (declarationMode == 0) {
                    {
                        STRING source(STRING::EMPTY);
                        STRING context(zs1::logic::g_RetailLogicErrorText);
                        LOGICSTACK stackValue(&source, &context);
                        stackValue.type = static_cast<uint8_t>(flags | 0xC1);
                        stack.Push(&stackValue);
                    }
                    EmitRuntimeInitializer(this, slot + initialized);
                }
                else {
                    STRING value;
                    value = GetConstantString();
                    {
                        STRING source(value.m_buf);
                        STRING context(zs1::logic::g_RetailLogicErrorText);
                        LOGICSTACK stackValue(&source, &context);
                        stackValue.type = static_cast<uint8_t>(flags | 0x89);
                        stack.Push(&stackValue);
                    }
                }
                ++initialized;

                if (!Word(","))
                    break;
                if (compileError)
                    break;
            }
            WordEnd("}");
            if (count == kUnsizedArrayCount)
                count = initialized;
        }
        else {
            if (declarationMode == 0) {
                {
                    STRING source(STRING::EMPTY);
                    STRING context(zs1::logic::g_RetailLogicErrorText);
                    LOGICSTACK stackValue(&source, &context);
                    stackValue.type = static_cast<uint8_t>(flags | 0xC1);
                    stack.Push(&stackValue);
                }
                EmitRuntimeInitializer(this, slot);
            }
            else {
                STRING value;
                value = GetConstantString();
                {
                    STRING source(value.m_buf);
                    STRING context(zs1::logic::g_RetailLogicErrorText);
                    LOGICSTACK stackValue(&source, &context);
                    stackValue.type = static_cast<uint8_t>(flags | 0x89);
                    stack.Push(&stackValue);
                }
            }
            initialized = 1;
            count = 1;
        }
    }
    else if (count == kUnsizedArrayCount) {
        ReportLogicError(this, 10, "for [] need initialisation", 0);
    }

    for (int i = initialized; i < count && !compileError; ++i) {
        STRING source(STRING::EMPTY);
        STRING context(zs1::logic::g_RetailLogicErrorText);
        LOGICSTACK stackValue(&source, &context);
        stackValue.type = static_cast<uint8_t>(flags | 0xC1);
        stack.Push(&stackValue);
    }

    SetNoElement(count);
}

void LOGIC::mnog()
{
    if (compileError)
        return;

    // A label definition resumes parsing at the beginning of this primary in
    // retail, so preserve that small loop instead of hiding it in oper().
    for (;;) {
        if (compileError)
            return;

        STRING name;
        int unary = 0;
        int variableOperation = 0x24; // load variable
        int castOperation = 0;

        if (Word("(int)"))
            castOperation = 0x2A;
        else if (Word("(string)"))
            castOperation = 0x29;
        else if (Word("(sprite)"))
            castOperation = 0x2B;

        if (Word("-"))
            unary = 3;
        else if (Word("+"))
            unary = 0;
        else if (Word("~"))
            unary = 4;
        else if (Word("!"))
            unary = 5;

        if (Word("--"))
            variableOperation = 0x23;
        else if (Word("++"))
            variableOperation = 0x22;
        else if (Word("&"))
            variableOperation = 0x25;

        const char* source = CompilerSource(this);

        if (isdigit(static_cast<unsigned char>(*source))) {
            GetName(&name);
            int value = 0;
            sscanf(name.m_buf, "%i", &value);
            if (unary == 3) {
                value = -value;
                unary = 0;
            }
            else if (unary == 4) {
                value = ~value;
                unary = 0;
            }
            else if (unary == 5) {
                value = value == 0;
                unary = 0;
            }
            EmitIntegerLiteral(this, value);
        }
        else if (*source == '"') {
            EmitByteIfNoError(2);
            if (!compileError)
                byteCodeSize += GetString(
                    reinterpret_cast<char*>(CompilerByteCode(this) + byteCodeSize));
        }
        else if (*source == '\'') {
            ++pos;
            const int value = static_cast<signed char>(*CompilerSource(this));
            EmitIntegerLiteral(this, value);
            ++pos;
            if (*CompilerSource(this) != '\'')
                ReportLogicError(this, 13, "second '", 0);
            ++pos; // retail advances even after recording the error
        }
        else if (Word("sizeof")) {
            if (!Word("("))
                ReportLogicError(this, 13, "'(' for sizeof", 0);

            int value = 0;
            if (Word("int") || Word("string")) {
                value = 4;
            }
            else {
                GetName(&name);
                const int variable = variables.Location(&name);
                NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
                if (variable < 0 || entries[variable].val.flag != 1)
                    ReportLogicError(this, 4, "sizeof parameter", 0);
                else
                    value = entries[variable].val.extra * 4;
            }
            EmitIntegerLiteral(this, value);
            WordEnd(")");
        }
        else if (Word("static")) {
            if (Word("int")) {
                do {
                    IntVar(1);
                } while (!compileError && Word(","));
            }
            else if (Word("string")) {
                do {
                    StringVar(1);
                } while (!compileError && Word(","));
            }
            else {
                ReportLogicError(this, 4, "static variable", 0);
            }
            return;
        }
        else if (Word("int")) {
            do {
                IntVar(0);
            } while (!compileError && Word(","));
            return;
        }
        else if (Word("string")) {
            do {
                StringVar(0);
            } while (!compileError && Word(","));
            return;
        }
        else if (Word("return")) {
            CompileExpression(1);
            EmitByteIfNoError(0x1F);
            if (currentSymbol) {
                LOGICVAR* current = currentSymbol;
                current->auxFlags = static_cast<uint8_t>(current->auxFlags | 2u);
            }
            return;
        }
        else if (Word("(")) {
            CompileExpression(1);
            WordEnd(")");
        }
        else {
            source = CompilerSource(this);
            if (isalpha(static_cast<unsigned char>(*source))) {
                GetName(&name);
                const char* nameText = name.m_buf;
                int variable = variables.Location(&name);
                NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);

                if (variable < 0) {
                    if (*CompilerSource(this) != ':') {
                        STRING text = Printf("Undeclared identifier '%s'", nameText);
                        Error(10, &text, 0);
                        return;
                    }

                    ++pos;
                    LOGICVAR label; // target leaves type/extra unspecified here
                    label.flag = 7;
                    label.auxFlags = 0;
                    label.a = byteCodeSize;
                    variables.Insert(name, label);
                    continue;
                }

                NAMED_LIST_STRUCT<LOGICVAR>& entry = entries[variable];
                switch (entry.val.flag) {
                case 1: {
                    int indexCodeSize = 0;
                    uint8_t* indexCode = 0;

                    if (Word("[")) {
                        const int indexStart = byteCodeSize;
                        LOGICSTACK* values = CompilerStack(this);
                        if (!(values[entry.val.a].type & 0x25u))
                            ReportLogicError(this, 10, "[] for not array", 0);

                        CompileExpression(1);
                        EmitByteIfNoError(0x28);
                        indexCodeSize = byteCodeSize - indexStart;
                        if (indexCodeSize > 0) {
                            indexCode = static_cast<uint8_t*>(
                                zs1::engine::RetailOperatorNew(
                                    static_cast<size_t>(indexCodeSize)));
                            if (!indexCode) {
                                ReportLogicError(this, 2, nameText, 0);
                            }
                            else {
                                memcpy(indexCode,
                                            CompilerByteCode(this) + indexStart,
                                            static_cast<size_t>(indexCodeSize));
                            }
                        }
                        byteCodeSize = indexStart;
                        WordEnd("]");
                    }

                    int command = variableOperation;
                    int compoundOperation = 0;

                    if (Word("=")) {
                        if (Word("{")) {
                            LOGICSTACK* values = CompilerStack(this);
                            if (!(values[entry.val.a].type & 0x24u))
                                ReportLogicError(this, 10, "= {} for not array", 0);

                            int element = 0;
                            do {
                                CompileExpression(1);
                                EmitByteInt32(0x26, entry.val.a + element);
                                EmitByteInt32(0x19, line);
                                ++element;
                            } while (!compileError && Word(","));
                            WordEnd("}");
                            command = variableOperation;
                        }
                        else {
                            CompileExpression(1);
                            command = 0x26;
                        }
                    }
                    else if (Word("+=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 8;
                    }
                    else if (Word("-=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 9;
                    }
                    else if (Word("/=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 6;
                    }
                    else if (Word("*=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 19;
                    }
                    else if (Word("%=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 7;
                    }
                    else if (Word("&=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 12;
                    }
                    else if (Word("|=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 11;
                    }
                    else if (Word("^=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 10;
                    }
                    else if (Word("<<=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 23;
                    }
                    else if (Word(">>=")) {
                        CompileExpression(1); command = 0x27; compoundOperation = 22;
                    }
                    else if (Word("++")) {
                        command = 0x20;
                    }
                    else if (Word("--")) {
                        command = 0x21;
                    }

                    if (indexCodeSize) {
                        if (indexCode) {
                            memcpy(CompilerByteCode(this) + byteCodeSize,
                                        indexCode,
                                        static_cast<size_t>(indexCodeSize));
                            byteCodeSize += indexCodeSize;
                            zs1::engine::RetailOperatorDelete(indexCode);
                            indexCode = 0;
                        }
                    }
                    else {
                        LOGICSTACK* values = CompilerStack(this);
                        if (values[entry.val.a].type & 0x24u) {
                            if (command >= 0x20 && command <= 0x23)
                                ReportLogicError(this, 10,
                                                 "Increment or decrement for array", 0);

                            if (!(command == 0x24 && g_CompilerArrayArgumentContext))
                                ReportLogicError(this, 4, "operation for array", command);
                        }
                    }

                    EmitByteInt32(static_cast<uint8_t>(command), entry.val.a);
                    if (command == 0x27)
                        EmitByteIfNoError(static_cast<uint8_t>(compoundOperation));
                    break;
                }

                case 2: { // external/native function
                    WordEnd("(");
                    int count = 0;
                    while (!compileError && !Word(")")) {
                        if (count <= entry.val.extra) {
                            LOGICSTACK* values = CompilerStack(this);
                            const int slot = entry.val.type + count;
                            if (slot >= 0 && slot < stack.m_no && (values[slot].type & 0x20u))
                                g_CompilerArrayArgumentContext = 1;
                        }
                        CompileExpression(1);
                        Word(",");
                        g_CompilerArrayArgumentContext = 0;
                        ++count;
                    }

                    LOGICSTACK* values = CompilerStack(this);
                    while (count < entry.val.extra) {
                        const int slot = entry.val.type + count;
                        if (!(values[slot].type & 8u))
                            break;
                        EmitByteInt32(0x24, slot);
                        ++count;
                    }

                    if (count != entry.val.extra) {
                        ReportLogicError(this, 4,
                                         "extern function parameters number", 0);
                    }
                    else {
                        EmitByteIfNoError(static_cast<uint8_t>(entry.val.a));
                        MarkCalledFunctionResult(this, entry);
                    }
                    break;
                }

                case 3: { // script function
                    WordEnd("(");
                    int count = 0;
                    while (!compileError && !Word(")")) {
                        if (count <= entry.val.extra) {
                            LOGICSTACK* values = CompilerStack(this);
                            const int slot = entry.val.type + count;
                            if (slot >= 0 && slot < stack.m_no && (values[slot].type & 0x20u))
                                g_CompilerArrayArgumentContext = 1;
                        }
                        CompileExpression(1);
                        Word(",");
                        g_CompilerArrayArgumentContext = 0;
                        EmitByteInt32(0x26, entry.val.type + count);
                        EmitByteIfNoError(0x1A);
                        ++count;
                    }

                    LOGICSTACK* values = CompilerStack(this);
                    while (count < entry.val.extra) {
                        const int slot = entry.val.type + count;
                        LOGICSTACK& value = values[slot];
                        if (!(value.type & 8u))
                            break;
                        if (value.type & 1u)
                            EmitStringLiteral(this, value.string.m_buf);
                        else
                            EmitIntegerLiteral(this, value.number);
                        EmitByteInt32(0x26, slot);
                        EmitByteIfNoError(0x1A);
                        ++count;
                    }

                    if (count != entry.val.extra) {
                        ReportLogicError(this, 4, "function parameters number", 0);
                    }
                    else {
                        EmitByteInt32(0x1E, entry.val.a);
                        MarkCalledFunctionResult(this, entry);
                    }
                    break;
                }

                case 4:
                    EmitStringLiteral(this, entry.val.value.m_buf);
                    break;

                case 5:
                    EmitIntegerLiteral(this, entry.val.a);
                    break;

                case 6:
                    break;

                case 7:
                    if (*CompilerSource(this) == ':') {
                        STRING text = Printf("Label redefinition '%s'", nameText);
                        Error(10, &text, 0);
                    }
                    else {
                        STRING text = Printf("Incorrect use label '%s'", nameText);
                        Error(10, &text, 0);
                    }
                    return;

                case 8:
                    if (*CompilerSource(this) != ':') {
                        STRING text = Printf("Incorrect use label '%s'", nameText);
                        Error(10, &text, 0);
                        return;
                    }
                    ++pos;
                    entry.val.flag = 7;
                    CompilerByteCode(this)[entry.val.a] = static_cast<uint8_t>(
                        byteCodeSize - entry.val.a);
                    entry.val.a = byteCodeSize;
                    continue;

                default:
                    break;
                }

            }
            else {
                if (unary)
                    ReportLogicError(this, 10, "error symbol", 0);
                if (castOperation)
                    ReportLogicError(this, 10, "error change type", 0);

                const char ch = *CompilerSource(this);
                if (strchr(".$#@`", ch))
                    ReportLogicError(this, 10, "error symbol", 0);
                else if (!strchr(";)", ch))
                    ReportLogicError(this, 10, "syntax error", 0);
                return;
            }
        }

        if (compileError) {
            return;
        }
        if (unary)
            EmitByteIfNoError(static_cast<uint8_t>(unary));
        if (castOperation)
            EmitByteIfNoError(static_cast<uint8_t>(castOperation));
        return;
    }
}

void LOGIC::SetOperation(int byteCodePos, int operation)
{
    if (compileError)
        return;

    uint8_t* code = CompilerByteCode(this);

    // Retail folds two adjacent integer constants only when the complete tail is
    // exactly [01 dword][01 dword].  This is intentionally not generalized.
    if (code[byteCodePos] == 1 && code[byteCodePos + 5] == 1 && byteCodeSize - byteCodePos == 10) {
        int& left = *reinterpret_cast<int*>(code + byteCodePos + 1);
        const int right = *reinterpret_cast<int*>(code + byteCodePos + 6);

        switch (operation) {
        case 6:  left /= right; break;
        case 7:  left %= right; break;
        case 8:  left += right; break;
        case 9:  left -= right; break;
        case 10: left ^= right; break;
        case 11: left |= right; break;
        case 12: left &= right; break;
        case 19: left *= right; break;
        case 22: left >>= right; break;
        case 23: left <<= right; break;
        default: break;
        }

        // Retail always consumes the second integer record once the exact
        // [01 dword][01 dword] tail predicate matched, including default slots.
        byteCodeSize -= 5;
        return;
    }

    // Constant-string tail handling from 0x435BC7..0x435C30.  For operation 8
    // the strings are concatenated; for the other operation values retail still
    // removes the second record marker/terminator pair before returning.
    if (code[byteCodePos] == 2) {
        const size_t leftLength = strlen(reinterpret_cast<const char*>(code + byteCodePos + 1));
        const int secondMarker = byteCodePos + static_cast<int>(leftLength) + 2;
        if (code[secondMarker] == 2) {
            char* second = reinterpret_cast<char*>(code + secondMarker + 1);
            const size_t rightLength = strlen(second);
            const int encodedSize = static_cast<int>(leftLength + rightLength) + 4;
            if (byteCodeSize - byteCodePos == encodedSize) {
                if (operation == 8) {
                    memmove(code + byteCodePos + leftLength + 1,
                                 second,
                                 rightLength + 1);
                }
                byteCodeSize -= 2;
                return;
            }
        }
    }

    EmitByteIfNoError(static_cast<uint8_t>(operation));
}

void LOGIC::slag()
{
    const int start = byteCodeSize;
    mnog();
    while (!compileError) {
        if (Word("*")) {
            mnog();
            SetOperation(start, 19);
            continue;
        }
        if (Word("/")) {
            mnog();
            SetOperation(start, 6);
            continue;
        }
        if (Word("%")) {
            mnog();
            SetOperation(start, 7);
            continue;
        }
        return;
    }
}

void LOGIC::cmpslag()
{
    const int start = byteCodeSize;
    slag();
    while (!compileError) {
        if (Word("+")) {
            slag();
            SetOperation(start, 8);
            continue;
        }
        if (Word("-")) {
            slag();
            SetOperation(start, 9);
            continue;
        }
        return;
    }
}

void LOGIC::logicslag()
{
    const int start = byteCodeSize;
    cmpslag();
    while (!compileError) {
        if (Word(">=")) { cmpslag(); EmitByteIfNoError(0x11); continue; }
        if (Word(">>")) { cmpslag(); SetOperation(start, 0x16); continue; }
        if (Word(">"))  { cmpslag(); EmitByteIfNoError(0x0F); continue; }
        if (Word("<=")) { cmpslag(); EmitByteIfNoError(0x12); continue; }
        if (Word("<<")) { cmpslag(); SetOperation(start, 0x17); continue; }
        if (Word("<"))  { cmpslag(); EmitByteIfNoError(0x10); continue; }
        if (Word("==")) { cmpslag(); EmitByteIfNoError(0x0D); continue; }
        if (Word("!=")) { cmpslag(); EmitByteIfNoError(0x14); continue; }
        return;
    }
}

void LOGIC::vyragAnd()
{
    const int start = byteCodeSize;
    logicslag();
    if (compileError)
        return;
    while (Word("&")) {
        logicslag();
        SetOperation(start, 0x0C);
    }
}

void LOGIC::vyragXor()
{
    const int start = byteCodeSize;
    vyragAnd();
    if (compileError)
        return;
    while (Word("^")) {
        vyragAnd();
        SetOperation(start, 0x0A);
    }
}

void LOGIC::vyragOr()
{
    const int start = byteCodeSize;
    vyragXor();
    if (compileError)
        return;
    while (Word("|")) {
        vyragXor();
        SetOperation(start, 0x0B);
    }
}

void LOGIC::vyragCmpAnd()
{
    vyragOr();
    if (compileError)
        return;

    while (Word("&&")) {
        uint8_t* code = CompilerByteCode(this);
        code[byteCodeSize] = 0x2C;
        const int fixup = ++byteCodeSize;
        byteCodeSize += 4;

        vyragOr();

        code = CompilerByteCode(this);
        code[byteCodeSize] = 0x15;
        ++byteCodeSize;
        *reinterpret_cast<int*>(code + fixup) = byteCodeSize - fixup;
    }
}

void LOGIC::CompileExpression(int context)
{
    const int previousContext = parseContext;
    parseContext = context;

    vyragCmpAnd();
    // Exact retail quirk: an error from the first stage exits without restoring
    // +0x86C.  Do not turn this into an RAII/context-guard cleanup.
    if (compileError)
        return;

    while (Word("||")) {
        EmitByteIfNoError(0x2D);
        const int fixup = byteCodeSize;
        byteCodeSize += 4;

        vyragCmpAnd();

        uint8_t* code = CompilerByteCode(this);
        code[byteCodeSize] = 0x0E;
        ++byteCodeSize;
        *reinterpret_cast<int*>(code + fixup) = byteCodeSize - fixup;
    }

    parseContext = previousContext;
}

int LOGIC::vyrag_oper()
{
    CompileExpression(0);
    while (!compileError) {
        EmitByteInt32(0x19, line + 1);
        if (!Word(","))
            return 0;
        CompileExpression(0);
    }
    return compileError;
}

void LOGIC::oper(int* breakFixups)
{
    if (compileError)
        return;

    const int inverted = Word("iff");
    if (inverted || Word("if")) {
        WordEnd("(");
        CompileExpression(1);
        WordEnd(")");

        EmitByteIfNoError(static_cast<uint8_t>(inverted ? 0x1D : 0x18));
        const int branch = byteCodeSize;
        byteCodeSize += 4;

        oper(breakFixups);
        *reinterpret_cast<int*>(CompilerByteCode(this) + branch) = byteCodeSize - branch;

        if (Word("else")) {
            uint8_t* code = CompilerByteCode(this);
            *reinterpret_cast<int*>(code + branch) += 5;
            EmitByteIfNoError(0x1C);
            const int endBranch = byteCodeSize;
            byteCodeSize += 4;
            oper(breakFixups);
            *reinterpret_cast<int*>(CompilerByteCode(this) + endBranch) = byteCodeSize - endBranch;
        }
        return;
    }

    if (Word("while")) {
        int localBreaks[128] = {0};
        WordEnd("(");
        const int loopStart = byteCodeSize;
        CompileExpression(1);
        WordEnd(")");

        EmitByteIfNoError(0x18);
        const int loopEnd = byteCodeSize;
        byteCodeSize += 4;
        oper(localBreaks);

        EmitByteIfNoError(0x1C);
        const int loopBack = byteCodeSize;
        *reinterpret_cast<int*>(CompilerByteCode(this) + loopBack) = loopStart - loopBack;
        byteCodeSize += 4;
        *reinterpret_cast<int*>(CompilerByteCode(this) + loopEnd) = byteCodeSize - loopEnd;
        PatchBreakFixups(this, localBreaks);
        return;
    }

    if (Word("do")) {
        int localBreaks[128] = {0};
        const int loopStart = byteCodeSize;
        oper(localBreaks);
        WordEnd("while");
        WordEnd("(");
        CompileExpression(1);
        WordEnd(")");
        EmitByteIfNoError(5);
        EmitByteIfNoError(0x18);
        const int loopBack = byteCodeSize;
        *reinterpret_cast<int*>(CompilerByteCode(this) + loopBack) = loopStart - loopBack;
        byteCodeSize += 4;
        PatchBreakFixups(this, localBreaks);
        return;
    }

    if (Word("for")) {
        int localBreaks[128] = {0};
        WordEnd("(");
        vyrag_oper();
        WordEnd(";");
        const int condition = byteCodeSize;
        CompileExpression(1);
        WordEnd(";");

        EmitByteIfNoError(0x18);
        const int loopEnd = byteCodeSize;
        byteCodeSize += 4;
        EmitByteIfNoError(0x1C);
        const int bodyBranch = byteCodeSize;
        const int increment = byteCodeSize + 4;
        byteCodeSize += 4;

        vyrag_oper();
        WordEnd(")");
        EmitByteIfNoError(0x1C);
        const int conditionBack = byteCodeSize;
        *reinterpret_cast<int*>(CompilerByteCode(this) + conditionBack) = condition - conditionBack;
        byteCodeSize += 4;
        *reinterpret_cast<int*>(CompilerByteCode(this) + bodyBranch) = byteCodeSize - bodyBranch;

        oper(localBreaks);
        EmitByteIfNoError(0x1C);
        const int incrementBack = byteCodeSize;
        *reinterpret_cast<int*>(CompilerByteCode(this) + incrementBack) = increment - incrementBack;
        byteCodeSize += 4;
        *reinterpret_cast<int*>(CompilerByteCode(this) + loopEnd) = byteCodeSize - loopEnd;
        PatchBreakFixups(this, localBreaks);
        return;
    }

    if (Word("break")) {
        WordEnd(";");
        if (!breakFixups) {
            ReportLogicError(this, 10, "'break' without loop", 0);
            return;
        }

        int count = 0;
        while (breakFixups[count]) {
            if (count >= 128) {
                ReportLogicError(this, 10, "Too many 'break'", 0);
                return;
            }
            ++count;
        }
        EmitByteIfNoError(0x1C);
        breakFixups[count] = byteCodeSize;
        byteCodeSize += 4;
        if (count + 1 < 128)
            breakFixups[count + 1] = 0;
        return;
    }

    if (Word("goto")) {
        STRING labelName;
        GetName(&labelName);
        const char* labelText = labelName.m_buf;
        int label = variables.Location(&labelName);

        if (label < 0) {
            LOGICVAR value;
            value.flag = 8;
            value.a = byteCodeSize + 1;
            variables.Insert(labelName, value);
            label = variables.m_no - 1;
        }
        else {
            NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
            if (entries[label].val.flag == 8)
                { STRING text = Printf("second use undefined label '%s'", labelText); Error(10, &text, 0); }
            else if (entries[label].val.flag != 7)
                { STRING text = Printf("'%s' is not label", labelText); Error(10, &text, 0); }
        }

        if (!compileError) {
            EmitByteIfNoError(0x1C);
            const int operand = byteCodeSize;
            NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
            *reinterpret_cast<int*>(CompilerByteCode(this) + operand) = entries[label].val.a - operand;
            byteCodeSize += 4;
        }
        return;
    }

    if (Word("{")) {
        const int savedVariableCount = variables.m_no;
        while (!compileError && !Word("}"))
            oper(breakFixups);
        RestoreCompilerVariableScope(this, savedVariableCount);
        return;
    }

    vyrag_oper();
    WordEnd(";");
}

int LOGIC::func()
{
    STRING symbolName;
    LOGICVAR function;
    char fileName[1024];

    if (skipempty2())
        return 1;
    if (compileError)
        return 1;

    {
    const char* source = CompilerSource(this);

    // Retail handles #define/#undef by raw character tests, rather than Word(),
    // so parserMode suppresses macro replacement while the directive name is read.
    if (source[0]=='#' && source[1]=='d' && source[2]=='e' && source[3]=='f' && source[4]=='i' && source[5]=='n' && source[6]=='e') {
        const int defineLine = line;
        pos += 7;
        parserMode = 1;
        GetName(&symbolName);
        parserMode = 0;

        STRING value;
        if (defineLine == line)
            GetLine(&value);

        const int found = strings.Location(&symbolName);
        if (found < 0) {
            strings.Insert(symbolName, value);
        }
        else {
            NAMED_LIST_STRUCT<STRING>* entries = CompilerDefines(this);
            entries[found].val = value;
        }
        goto finish;
    }

    source = CompilerSource(this);
    if (source[0]=='#' && source[1]=='u' && source[2]=='n' && source[3]=='d' && source[4]=='e' && source[5]=='f') {
        pos += 6;
        parserMode = 1;
        GetName(&symbolName);
        parserMode = 0;

        const int found = strings.Location(&symbolName);
        if (found < 0) {
            ReportLogicError(this, 4, "#undef parameters", 0);
        }
        else {
            NAMED_LIST_STRUCT<STRING>* entries = CompilerDefines(this);
            --strings.m_no;
            for (int i = found; i < strings.m_no; ++i) {
                entries[i].name = entries[i + 1].name;
                entries[i].val = entries[i + 1].val;
            }
            if (strings.m_no == 0)
                strings.Release();
        }
        goto finish;
    }

    if (Word("#include")) {
        STRING oldName;
        oldName = this->name;

        source = CompilerSource(this);
        if (*source != '"' && *source != '<') {
            ReportLogicError(this, 13, "include file name", 0);
            goto finish;
        }

        ++pos;
        int length = 0;
        while (true) {
            source = CompilerSource(this);
            if (*source == '"' || *source == '>')
                break;
            if (pos >= end) {
                ReportLogicError(this, 10, "End of file", 0);
                break;
            }
            fileName[length++] = *source;
            ++pos;
        }
        ++pos;
        fileName[length] = 0;

        FILE* file = static_cast<FILE*>(fopen(fileName, "rb"));
        if (!file) {
            ReportLogicError(this, 7, fileName, 0);
            goto finish;
        }

        const int size = zs1::logic::RetailFileLength(file);
        char* const oldPos = pos;
        char* const oldEnd = end;
        char* const oldBuffer = sourceBuffer;
        const int oldLine = line;
        const int oldParserFlags = parserFlags54;

        uint8_t* includeBuffer = static_cast<uint8_t*>(
            zs1::engine::RetailOperatorNew(static_cast<size_t>(size) + 4096u));
        sourceBuffer = reinterpret_cast<char*>(includeBuffer);
        if (!includeBuffer) {
            ReportLogicError(this, 2, "include", 0);
            fclose(file);
            goto finish;
        }

        pos = reinterpret_cast<char*>(includeBuffer + 4066);
        end = pos + size;
        fread(includeBuffer + 4066, static_cast<size_t>(size), 1, file);
        line = 0;
        parserFlags54 = 0;

        this->name = fileName;

        while (!compileError && !func()) {
        }

        fclose(file);
        zs1::engine::RetailOperatorDelete(includeBuffer);
        sourceBuffer = oldBuffer;
        pos = oldPos;
        end = oldEnd;
        line = oldLine;
        parserFlags54 = oldParserFlags;
        this->name = oldName;
        goto finish;
    }

    if (Word("extern")) {
        const int savedVariableCount = variables.m_no;
        GetName(&symbolName);
        if (variables.Location(&symbolName) >= 0)
            ReportLogicError(this, 10, "function redefinition", 0);

        const int stackStart = stack.m_no;
        WordEnd("(");
        for (;;) {
            if (Word("int"))
                IntVar(4);
            else if (Word("string"))
                StringVar(4);
            if (!Word(","))
                break;
            if (compileError)
                break;
        }
        WordEnd(")");
        const int parameterCount = stack.m_no - stackStart;
        const int code = GetInt();
        if (!isdigit(static_cast<unsigned char>(pos[-1])))
            ReportLogicError(this, 13, "extern function code", 0);

        RestoreCompilerVariableScope(this, savedVariableCount);

        if (!compileError) {
            function.flag = 2;
            function.a = code;
            function.type = stackStart;
            function.extra = parameterCount;
            variables.Insert(symbolName, function);
            WordEnd(";");
        }
        goto finish;
    }

    if (Word("static")) {
        if (Word("int")) {
            do { IntVar(3); } while (!compileError && Word(","));
            WordEnd(";");
        }
        else if (Word("string")) {
            do { StringVar(3); } while (!compileError && Word(","));
            WordEnd(";");
        }
        else {
            ReportLogicError(this, 4, "static variable", 0);
        }
        goto finish;
    }

    if (Word("int")) {
        do { IntVar(2); } while (!compileError && Word(","));
        WordEnd(";");
        goto finish;
    }

    if (Word("string")) {
        do { StringVar(2); } while (!compileError && Word(","));
        WordEnd(";");
        goto finish;
    }

    // Function declaration/definition.
    const int savedVariableCount = variables.m_no;
    GetName(&symbolName);
    int declared = variables.Location(&symbolName);

    const int stackStart = stack.m_no;
    function.flag = 3;
    function.a = byteCodeSize;
    function.type = stackStart;
    function.extra = 0;

    if (declared >= 0) {
        NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
        currentSymbol = &entries[declared].val;
    }
    else {
        currentSymbol = &function;
    }

    WordEnd("(");
    int parameterOrdinal = 0;
    for (;;) {
        const int beforeVariables = variables.m_no;
        if (Word("int"))
            IntVar(4);
        else if (Word("string"))
            StringVar(4);

        if (declared >= 0 && variables.m_no > beforeVariables) {
            NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
            entries[variables.m_no - 1].val.a = entries[declared].val.type + parameterOrdinal;
        }
        ++parameterOrdinal;

        if (!Word(","))
            break;
        if (compileError)
            break;
    }
    WordEnd(")");

    const int parameterCount = stack.m_no - stackStart;
    function.extra = parameterCount;

    if (declared >= 0) {
        NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
        if (parameterCount != entries[declared].val.extra || entries[declared].val.a != -1)
            ReportLogicError(this, 10, "function redefinition", 0);
        else
            RestoreCompilerStackScope(this, stackStart);
    }

    if (Word(";")) {
        function.a = -1;
    }
    else if (!compileError) {
        if (declared >= 0) {
            NAMED_LIST_STRUCT<LOGICVAR>* entries = CompilerVariables(this);
            entries[declared].val.a = function.a;
        }
        WordEnd("{");
        while (!compileError && !Word("}"))
            oper(0);
        EmitByteIfNoError(0x1F);
    }

    RestoreCompilerVariableScope(this, savedVariableCount);

    if (declared < 0 && !compileError) {
        variables.Insert(symbolName, function);
        const int functionIndex = variables.m_no - 1;
        const char* text = symbolName.m_buf;

        if (text && strcmp(text, "main") == 0) {
            mainFunction = functionIndex;
        }
        else if (text && strncmp(text, "ScriptEvent", 11) == 0) {
            const char* suffix = text + 11;
            const bool numeric = isdigit(static_cast<unsigned char>(*suffix)) != 0 ||
                                 (*suffix == '-' && isdigit(static_cast<unsigned char>(suffix[1])) != 0);
            if (numeric) {
                int eventNo = 0;
                if (suffix[1] == 'x')
                    sscanf(suffix, "%i", &eventNo);
                else
                    eventNo = atoi(suffix);

                if (parameterCount != 3) {
                    ReportLogicError(this, 10, "ScriptEvent must have 3 parameters", 0);
                }
                else if (static_cast<unsigned int>(eventNo) >= 256u) {
                    ReportLogicError(this, 10, "too big number in function name", 0);
                }
                else {
                    scriptEventFunction[eventNo] = functionIndex;
                }
            }
        }
    }

    }

finish:
    currentSymbol = 0;
    return skipempty2();
}

