#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"
#include "mapedit/zs1/script_exec.hpp"

namespace {
inline int ZS1PointerBits(const void* p)
{
    return static_cast<int>(static_cast<unsigned int>(reinterpret_cast<size_t>(p)));
}
}

// Exact target family: direct LOGIC/LIST ownership, contextual STRING temporaries,
// and no reconstruction-only VM helper layer inside this owner.
int LOGIC::CallFunction(int function, const char* signature,
                        const void* a, const void* b, int c)
{
    int result=0;
    int indexActive=0;
    int arrayIndex=0;
    int pointerSentinel=0x7FFFFFFF;

    if(compileError || !byteCode)
        return 0;

    if(function<0)
        function=mainFunction;
    if(function<0 || function>=variables.m_no) {
        MYERROR::Log(::Error,"!!!ERROR!!! SCRIPT Call unexisted function %i",function);
        return 0;
    }

    NAMED_LIST_STRUCT<LOGICVAR>* vars=variables.m_data;
    NAMED_LIST_STRUCT<LOGICVAR>& fn=vars[function];
    if(fn.val.flag!=3) {
        MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: Call unexisted function %s()",fn.name.m_buf);
        return 0;
    }

    // Target 0x00437EF0..0x00437F63: PushInt is a script.h inline owner.
    const int initialStack=stack.m_no;
    PushInt(initialStack);
    PushInt(byteCodeSize);
    int frameBase=stack.m_no;

    // Target 0x00437F68..0x00438422.  The context STRINGs are intentionally
    // retained even though the value constructors do not store the context.
    const char* sig=signature;
    if(fn.val.extra>=1) {
        if(sig[0]=='s') {
            STRING context(fn.name.m_buf,"(str const");
            LOGICSTACK temp(a,&context);
            stack.m_data[fn.val.type]=temp;
        } else if(sig[0]=='p') {
            STRING context(fn.name.m_buf,"(obj const");
            LOGICSTACK temp(a,&context);
            stack.m_data[fn.val.type]=temp;
        } else {
            STRING context(fn.name.m_buf,"(int const");
            LOGICSTACK temp(ZS1PointerBits(a),&context);
            stack.m_data[fn.val.type]=temp;
        }
    }
    if(fn.val.extra>=2) {
        if(sig[1]=='s') {
            STRING context(fn.name.m_buf,"(..., str const");
            LOGICSTACK temp(b,&context);
            stack.m_data[fn.val.type+1]=temp;
        } else if(sig[1]=='p') {
            STRING context(fn.name.m_buf,"(..., obj const");
            LOGICSTACK temp(b,&context);
            stack.m_data[fn.val.type+1]=temp;
        } else {
            STRING context(fn.name.m_buf,"(..., int const");
            LOGICSTACK temp(ZS1PointerBits(b),&context);
            stack.m_data[fn.val.type+1]=temp;
        }
    }
    if(fn.val.extra>=3) {
        if(sig[2]=='s') {
            STRING context(fn.name.m_buf,"(..., ...,  str const");
            LOGICSTACK temp(reinterpret_cast<const void*>(c),&context);
            stack.m_data[fn.val.type+2]=temp;
        } else if(sig[2]=='p') {
            STRING context(fn.name.m_buf,"(..., ...,  obj const");
            LOGICSTACK temp(reinterpret_cast<const void*>(c),&context);
            stack.m_data[fn.val.type+2]=temp;
        } else {
            STRING context(fn.name.m_buf,"(..., ...,  int const");
            LOGICSTACK temp(c,&context);
            stack.m_data[fn.val.type+2]=temp;
        }
    }

    if(sig[0]=='p') pointerSentinel=ZS1PointerBits(a);
    else if(sig[1]=='p') pointerSentinel=ZS1PointerBits(b);
    else if(sig[2]=='p') pointerSentinel=c;

    int instruction=fn.val.a;
    int statementStart=instruction;
    runtimeScratch860=ownedBuffer38;

    while(instruction<byteCodeSize) {
        runtimeScratch864=runtimeScratch860;
        runtimeScratch860=static_cast<unsigned char*>(ownedBuffer38)+instruction*4;

        if(stack.m_no<frameBase)
            MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: '%s' stack error %i","pop, but not push",instruction);

        const int stackCount=stack.m_no;
        const unsigned operation=byteCode[instruction];

        if(operation>=6 && operation<=23) {
            stack.m_data[stackCount-2].BinarOperator(static_cast<int>(operation),stack.m_data[stackCount-1]);
            --stack.m_no;
            ++instruction;
            continue;
        }

        if(operation>=32 && operation<=39) {
            const int encodedVariable=*reinterpret_cast<const int*>(byteCode+instruction+1);
            int variable=encodedVariable;
            ++instruction;
            if((stack.m_data[variable].type&0x20) && indexActive)
                variable=stack.m_data[variable].number;
            const int effective=variable+arrayIndex;

            switch(operation) {
            case 39:
                --stack.m_no;
                stack.m_data[effective].BinarOperator(byteCode[instruction+4],stack.m_data[stackCount-1]);
                stack.Insert(stack.m_data[effective]);
                instruction+=5;
                break;

            case 38: {
                LOGICSTACK& base=stack.m_data[encodedVariable];
                LOGICSTACK& dst=stack.m_data[effective];
                LOGICSTACK& src=stack.m_data[stack.m_no-1];
                if((base.type&1) && indexActive && !(base.type&4)) {
                    base.string.m_buf[arrayIndex]=static_cast<char>(src.Int());
                } else if(dst.type&1) {
                    dst.type=static_cast<uint8_t>(dst.type&~0x40u);
                    dst.string=*src.String();
                } else {
                    dst.type=static_cast<uint8_t>(dst.type&0xAFu);
                    dst.number=src.Int();
                    if(src.type&0x10)
                        dst.type=static_cast<uint8_t>(dst.type|0x10u);
                }
                instruction+=4;
                break;
            }

            case 36:
                if(!indexActive && (stack.m_data[effective].type&4))
                    PushInt(encodedVariable);
                else
                    stack.Push(&stack.m_data[effective]);
                instruction+=4;
                break;

            case 37: {
                const unsigned int address=(stack.m_data[effective].type&1)
                    ? static_cast<unsigned int>(reinterpret_cast<size_t>(&stack.m_data[effective].string.m_buf))
                    : static_cast<unsigned int>(reinterpret_cast<size_t>(&stack.m_data[effective].number));
                PushInt(static_cast<int>(address));
                instruction+=4;
                break;
            }

            case 34:
                stack.m_data[effective].Inc();
                stack.Insert(stack.m_data[effective]);
                instruction+=4;
                break;

            case 35:
                stack.m_data[effective].Dec();
                stack.Insert(stack.m_data[effective]);
                instruction+=4;
                break;

            case 32: {
                LOGICSTACK copy(stack.m_data[effective]);
                stack.Insert(copy);
                stack.m_data[effective].Inc();
                instruction+=4;
                break;
            }

            case 33: {
                LOGICSTACK copy(stack.m_data[effective]);
                stack.Insert(copy);
                stack.m_data[effective].Dec();
                instruction+=4;
                break;
            }
            }
            indexActive=0;
            arrayIndex=0;
            continue;
        }

        ++instruction;
        switch(operation) {
        case 1: {
            STRING context("int const");
            LOGICSTACK item(*reinterpret_cast<const int*>(byteCode+instruction),&context);
            stack.Insert(item);
            instruction+=4;
            break;
        }

        case 2: {
            STRING context("str const");
            STRING text(reinterpret_cast<const char*>(byteCode+instruction));
            LOGICSTACK item(&text,&context);
            stack.Insert(item);
            instruction+=static_cast<int>(strlen(reinterpret_cast<const char*>(byteCode+instruction)))+1;
            break;
        }

        case 3: {
            LOGICSTACK& top=stack.m_data[stack.m_no-1];
            top.number=-top.Int();
            top.type=2;
            break;
        }
        case 4: {
            LOGICSTACK& top=stack.m_data[stack.m_no-1];
            top.number=~top.Int();
            top.type=2;
            break;
        }
        case 5: {
            LOGICSTACK& top=stack.m_data[stack.m_no-1];
            top.number=(top.Int()==0);
            top.type=2;
            break;
        }

        case 24: {
            LOGICSTACK& condition=stack.m_data[--stack.m_no];
            if(condition.Int()) instruction+=4;
            else instruction+=*reinterpret_cast<const int*>(byteCode+instruction);
            if(stack.m_no-frameBase>1)
                MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: '%s' stack error %i","if",instruction);
            statementStart=instruction;
            stack.SetNo(frameBase);
            break;
        }

        case 25: {
            const int sourceLine=*reinterpret_cast<const int*>(byteCode+instruction);
            instruction+=4;
            statementStart=instruction;
            if(stackCount-frameBase>1)
                MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: '%s' stack error %i","; in line ",sourceLine);
            stack.SetNo(frameBase);
            break;
        }

        case 26:
            stack.m_no=stackCount-1;
            break;

        case 28:
            instruction+=*reinterpret_cast<const int*>(byteCode+instruction);
            break;

        case 29: {
            LOGICSTACK& condition=stack.m_data[--stack.m_no];
            if(!condition.Int()) {
                instruction+=*reinterpret_cast<const int*>(byteCode+instruction);
            } else {
                byteCode[statementStart]=28;
                *reinterpret_cast<int*>(byteCode+statementStart+1)=
                    *reinterpret_cast<const int*>(byteCode+instruction)+instruction-statementStart-1;
                instruction+=4;
            }
            statementStart=instruction;
            if(stack.m_no-frameBase>1)
                MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: '%s' stack error %i","iff",instruction);
            stack.SetNo(frameBase);
            break;
        }

        case 30: {
            PushInt(frameBase);
            PushInt(instruction+4);
            const int callee=*reinterpret_cast<const int*>(byteCode+instruction);
            frameBase=stack.m_no;
            if(vars[callee].val.a<0) {
                MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: Undefined function %s()",vars[callee].name.m_buf);
                statementStart=instruction;
                break;
            }
            instruction=vars[callee].val.a;
            statementStart=instruction;
            break;
        }

        case 31: {
            if(stackCount-frameBase>1)
                MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: '%s' stack error %i","return",instruction);

            if(stack.m_no>frameBase) {
                LOGICSTACK returnValue(stack.m_data[--stack.m_no]);
                stack.SetNo(frameBase);
                instruction=stack.m_data[--stack.m_no].Int();
                frameBase=stack.m_data[--stack.m_no].Int();
                stack.Insert(returnValue);
                if(instruction>=byteCodeSize && !result && !(returnValue.type&1))
                    result=returnValue.Int();
            } else {
                stack.SetNo(frameBase);
                instruction=stack.m_data[--stack.m_no].Int();
                frameBase=stack.m_data[--stack.m_no].Int();
            }
            statementStart=instruction;
            break;
        }

        case 40:
            arrayIndex=stack.m_data[--stack.m_no].Int();
            indexActive=1;
            break;

        case 41: {
            LOGICSTACK& top=stack.m_data[stack.m_no-1];
            top.String();
            top.type=1;
            break;
        }

        case 42: {
            LOGICSTACK& top=stack.m_data[stack.m_no-1];
            top.number=top.Int();
            top.type=2;
            break;
        }

        case 43: {
            LOGICSTACK& top=stack.m_data[stack.m_no-1];
            top.number=top.Int();
            top.type=top.number?0x12:0x02;
            break;
        }

        case 44:
            if(stack.m_data[stack.m_no-1].Int()) instruction+=4;
            else instruction+=*reinterpret_cast<const int*>(byteCode+instruction);
            break;

        case 45:
            if(!stack.m_data[stack.m_no-1].Int()) instruction+=4;
            else instruction+=*reinterpret_cast<const int*>(byteCode+instruction);
            break;

        default:
            if(operation==0x54 && stack.m_data[stackCount-1].Int()==pointerSentinel)
                result=1;
            ScriptExecFunc(static_cast<int>(operation));
            break;
        }
    }

    stack.SetNo(initialStack);
    return result;
}
