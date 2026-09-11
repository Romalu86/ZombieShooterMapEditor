#include "zScriptExec.h"

#include "zCommon.h"
#include "zDebugLog.h"
#include "zHelpParser.h"
#include "zHighScores.h"
#include "zScriptExec_engine.h"
#include "zUserMngr.h"
#include "mapedit/runtime.hpp"
#include "mapedit/zs1/runtime_layouts.hpp"


namespace zs1 {


const char* ScriptExecDispatch(
    int command,
    int arg1,
    int arg2,
    const char* str1,
    const char* str2,
    int* intResult,
    void** objectResult)
{
    // Original writes zero unconditionally before dispatch.
    *intResult = 0;

    if (command == 0x01) {
        *intResult = static_cast<int>(sqrt(static_cast<double>(arg1)));
        return nullptr;
    }
    if (command == 0x02) {
        script_engine::SetFloatPair(arg1, arg2);
        return nullptr;
    }
    if (command == 0x03) {
        script_engine::ToggleFlag();
        return nullptr;
    }
    if (command == 0x04) {
        script_engine::SetAngleState(static_cast<float>(arg1), static_cast<float>(arg2));
        return nullptr;
    }
    if (command == 0x05) {
        *intResult = g_UserMngr->GetUsersCnt();
        return nullptr;
    }
    if (command == 0x06) {
        return g_UserMngr->GetUserNameByOrdinal(arg1);
    }
    if (command == 0x07) {
        *intResult = g_UserMngr->AddUser(str1) ? 1 : 0;
        return nullptr;
    }
    if (command == 0x08) {
        if (arg1 == 2)
            g_UserMngr->SetAuxInt(str1, arg2);
        else
            g_UserMngr->SetInt(arg1 != 0, str1, arg2);
        return nullptr;
    }
    if (command == 0x09) {
        *intResult = arg1 == 2
            ? g_UserMngr->GetAuxInt(str1, arg2)
            : g_UserMngr->GetInt(arg1 != 0, str1, arg2);
        return nullptr;
    }
    if (command == 0x0A) {
        if (arg1 == 2)
            g_UserMngr->SetAuxStr(str1, str2);
        else
            g_UserMngr->SetStr(arg1 != 0, str1, str2);
        return nullptr;
    }
    if (command == 0x0B) {
        return arg1 == 2
            ? g_UserMngr->GetAuxStr(str1, str2)
            : g_UserMngr->GetStr(arg1 != 0, str1, str2);
    }
    if (command == 0x0C) {
        // Return value is deliberately ignored by the original dispatcher.
        (void)g_UserMngr->DeleteUserByOrdinal(arg1);
        return nullptr;
    }
    if (command == 0x0D) {
        (void)g_UserMngr->RenameUserByOrdinal(arg1, str1);
        return nullptr;
    }
    if (command == 0x0E) {
        (void)g_UserMngr->SetCurUserByOrdinal(arg1);
        return nullptr;
    }
    if (command == 0x0F) {
        *intResult = g_UserMngr->GetCurUserOrdinal();
        return nullptr;
    }
    if (command == 0x10) {
        if (g_DebugLog)
            g_DebugLog->Write(str1);
        return nullptr;
    }
    if (command == 0x11) {
        *intResult = g_HighScores->Add(str1, arg1, arg2);
        return nullptr;
    }
    if (command == 0x12) {
        *intResult = g_HighScores->GetCount();
        return nullptr;
    }
    if (command == 0x13) {
        return g_HighScores->GetName(arg1);
    }
    if (command == 0x14) {
        *intResult = g_HighScores->GetValue1(arg1);
        return nullptr;
    }
    if (command == 0x15) {
        *intResult = g_HighScores->GetValue2(arg1);
        return nullptr;
    }
    if (command == 0x16) {
        g_UserMngr->Save();
        return nullptr;
    }
    if (command == 0x17) {
        *intResult = g_HelpParser->LoadLevel(arg1);
        return nullptr;
    }
    if (command == 0x18) {
        *intResult = g_HelpParser->GetItemType(arg1, arg2);
        return nullptr;
    }
    if (command == 0x19) {
        // ASM performs mov ebp,[eax] immediately: no null guard here by design.
        return g_HelpParser->GetItemText(arg1, arg2)->m_str;
    }
    if (command == 0x1A) {
        *intResult = g_HelpParser->GetItemValue1(arg1, arg2);
        return nullptr;
    }
    if (command == 0x1B) {
        *intResult = g_HelpParser->GetItemValue2(arg1, arg2);
        return nullptr;
    }
    if (command == 0x1C) {
        *intResult = g_HelpParser->GetItemMetric(arg1, arg2);
        return nullptr;
    }
    if (command == 0x1D) {
        // Retail 0x004729B5..0x00472A09 is inline in the dispatcher.
        if (arg1 < 0)
            return nullptr;
        unsigned char* base = static_cast<unsigned char*>(script_engine::g_mainObject);
        const int count = *reinterpret_cast<int*>(base + 0xB0C);
        if (arg1 >= count)
            return nullptr;
        void* object = reinterpret_cast<void**>(base + 0xB10)[arg1];
        if (!object)
            return nullptr;
        const int divisor = *reinterpret_cast<int*>(static_cast<unsigned char*>(object) + 0x78);
        if (!divisor)
            return nullptr;
        *intResult = ((arg2 << 8) / divisor) & 0xFF;
        return nullptr;
    }
    if (command == 0x1E) {
        // Retail performs both dereferences directly in this owner.
        *objectResult = *reinterpret_cast<void**>(
            static_cast<unsigned char*>(script_engine::g_mainObject) + 0xADC);
        return nullptr;
    }
    if (command == 0x1F) {
        *intResult = *reinterpret_cast<int*>(
            static_cast<unsigned char*>(script_engine::g_object1F) + 0x28);
        return nullptr;
    }
    if (command == 0x20) {
        g_HighScores->ResetRecordsFile(true);
        return nullptr;
    }
    if (command == 0x21) {
        // Original ignores the bool return and leaves intResult at zero.
        (void)g_UserMngr->SelectOrAddUserByName(str1);
        return nullptr;
    }
    if (command == 0x22) {
        // Literal ASM behavior. Along the real chain ECX still contains 0x13 from
        // the earlier comparison at 0x00472875. The original therefore poisons the
        // global logger pointer with address 0x13 and immediately calls Write().
        // Nonsense marker string in the binary: "aassaass - bb".
        g_DebugLog = reinterpret_cast<zDebugLog*>(static_cast<unsigned long>(0x13));
        g_DebugLog->Write("aassaass - bb");
        return nullptr;
    }
    if (command == 0x23) {
        if (arg1 == 1) { *intResult = script_engine::g_moveQueryA; return nullptr; }
        if (arg1 == 2) { *intResult = script_engine::g_moveQueryB; return nullptr; }
        if (arg1 == 3) { *intResult = script_engine::g_moveQueryCount; return nullptr; }
        if (arg1 == 4) { *intResult = script_engine::g_moveQueryC; return nullptr; }
        return nullptr;
    }
    if (command == 0x24) {
        script_engine::g_flag49C4AC = static_cast<unsigned char>(arg1 != 0);
        return nullptr;
    }
    if (command == 0x25) {
        // Target 0x00472BDA loads arg1 into ECX and calls SPRITE owner 0x00456060.
        reinterpret_cast<SPRITE*>(static_cast<unsigned long>(static_cast<unsigned int>(arg1)))
            ->InsertPauseBeforeMoveActions(arg2);
        return nullptr;
    }
    if (command == 0x26) {
        script_engine::g_flag5F0AC8 = static_cast<unsigned char>(arg1 != 0);
        return nullptr;
    }
    if (command == 0x27) {
        // Dispatcher reads the byte directly; 0x00472DD0 is a separate exported owner.
        *intResult = script_engine::g_flag5F0AC8 ? 1 : 0;
        return nullptr;
    }
    if (command == 0x28) {
        engine::SPRITE_LAYOUT32* sprite = reinterpret_cast<engine::SPRITE_LAYOUT32*>(
            static_cast<unsigned long>(static_cast<unsigned int>(arg1)));
        if (!sprite)
            return nullptr;
        float value = 0.0f;
        if (arg2 == 1) value = sprite->x;
        else if (arg2 == 2) value = sprite->y;
        else if (arg2 == 3) value = sprite->z;
        *intResult = static_cast<int>(value * 1000.0f);
        return nullptr;
    }
    if (command == 0x29) {
        engine::SPRITE_LAYOUT32* sprite = reinterpret_cast<engine::SPRITE_LAYOUT32*>(
            static_cast<unsigned long>(static_cast<unsigned int>(arg1)));
        if (sprite) {
            const float x = static_cast<float>(arg2) * 0.0010000000474974513f;
            reinterpret_cast<SPRITE*>(sprite)->ChangeCoor(x, sprite->y, sprite->z);
        }
        return nullptr;
    }
    if (command == 0x2A) {
        engine::SPRITE_LAYOUT32* sprite = reinterpret_cast<engine::SPRITE_LAYOUT32*>(
            static_cast<unsigned long>(static_cast<unsigned int>(arg1)));
        if (sprite) {
            const float y = static_cast<float>(arg2) * 0.0010000000474974513f;
            reinterpret_cast<SPRITE*>(sprite)->ChangeCoor(sprite->x, y, sprite->z);
        }
        return nullptr;
    }
    if (command == 0x2B) {
        engine::SPRITE_LAYOUT32* sprite = reinterpret_cast<engine::SPRITE_LAYOUT32*>(
            static_cast<unsigned long>(static_cast<unsigned int>(arg1)));
        if (sprite) {
            const float z = static_cast<float>(arg2) * 0.0010000000474974513f;
            reinterpret_cast<SPRITE*>(sprite)->ChangeCoor(sprite->x, sprite->y, z);
        }
        return nullptr;
    }
    if (command == 0x2C) {
        *intResult = script_engine::g_oneShotValue;
        script_engine::g_oneShotValue = 0;
        return nullptr;
    }
    if (command == 0x2D) {
        // Target calls real SPRITE thiscall owner 0x00455FC0 with ECX=arg1.
        reinterpret_cast<SPRITE*>(static_cast<unsigned long>(static_cast<unsigned int>(arg1)))
            ->GotoNearestMoveAction();
        return nullptr;
    }
    if (command == 0x2E) {
        // Retail 0x00472AA5..0x00472B60 keeps this scan inline in ScriptExecDispatch.
        engine::SPRITE_LAYOUT32* sprite = reinterpret_cast<engine::SPRITE_LAYOUT32*>(
            static_cast<unsigned long>(static_cast<unsigned int>(arg1)));

        script_engine::g_moveQueryA = -1;
        script_engine::g_moveQueryB = -1;
        script_engine::g_moveQueryC = -1;
        script_engine::g_moveQueryCount = 0;
        if (!sprite)
            return nullptr;

        int index = sprite->actionsCount - 1;
        int guard = 100;
        while (index >= 0) {
            --guard;
            if (guard <= 0) {
                return nullptr;
            }

            engine::ACT* actions = reinterpret_cast<engine::ACT*>(
                static_cast<unsigned long>(sprite->actionsData));
            const engine::ACT& action = actions[index];
            if (action.command == 0x21) {
                if (script_engine::g_moveQueryCount == arg2) {
                    script_engine::g_moveQueryA = action.a;
                    script_engine::g_moveQueryB = action.b;
                    script_engine::g_moveQueryC = action.c;
                }
                ++script_engine::g_moveQueryCount;
            }
            --index;
        }
        return nullptr;
    }
    if (command == 0x2F) {
        g_UserMngr->LoadAux(arg1);
        return nullptr;
    }
    if (command == 0x30) {
        g_UserMngr->SaveAux(arg1);
        return nullptr;
    }
    if (command == 0x31) {
        g_UserMngr->ClearAux();
        return nullptr;
    }
        return nullptr;

}

} // namespace zs1
