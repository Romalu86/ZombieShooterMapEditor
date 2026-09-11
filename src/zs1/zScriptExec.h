#pragma once


namespace zs1 {

// Recovery-only names. Numeric values are exact from FUNCTION ZS1 0x004725C0.
enum ScriptOpcode {
    SqrtInt                    = 0x01,
    SetFloatPair               = 0x02,
    ToggleFlag                 = 0x03,
    SetAngleState              = 0x04,
    UserGetCount               = 0x05,
    UserGetNameByOrdinal       = 0x06,
    UserAdd                    = 0x07,
    ParamSetInt                = 0x08,
    ParamGetInt                = 0x09,
    ParamSetStr                = 0x0A,
    ParamGetStr                = 0x0B,
    UserDeleteByOrdinal        = 0x0C,
    UserRenameByOrdinal        = 0x0D,
    UserSetCurrentByOrdinal    = 0x0E,
    UserGetCurrentOrdinal      = 0x0F,
    DebugLogWrite              = 0x10,
    HighScoresAdd              = 0x11,
    HighScoresGetCount         = 0x12,
    HighScoresGetName          = 0x13,
    HighScoresGetValue1        = 0x14,
    HighScoresGetValue2        = 0x15,
    UserSave                   = 0x16,
    HelpLoadLevel              = 0x17,
    HelpGetItemType            = 0x18,
    HelpGetItemText            = 0x19,
    HelpGetItemValue1          = 0x1A,
    HelpGetItemValue2          = 0x1B,
    HelpGetItemMetric          = 0x1C,
    EngineEntityMetric         = 0x1D,
    EngineObjectReturn         = 0x1E,
    GlobalObjectField28        = 0x1F,
    HighScoresReset            = 0x20,
    UserSelectOrAddByName      = 0x21,
    DebugCrashProbe            = 0x22,
    MoveQueryGet               = 0x23,
    SetFlag49C4AC              = 0x24,
    InsertPauseBeforeMoves     = 0x25,
    SetFlag5F0AC8              = 0x26,
    GetFlag5F0AC8              = 0x27,
    SpriteGetCoordinate        = 0x28,
    SpriteSetX                 = 0x29,
    SpriteSetY                 = 0x2A,
    SpriteSetZ                 = 0x2B,
    ConsumeOneShotValue        = 0x2C,
    GotoNearestMoveAction      = 0x2D,
    MoveQueryBuild             = 0x2E,
    UserLoadAux                = 0x2F,
    UserSaveAux                = 0x30,
    UserClearAux               = 0x31,
};

// Exact cdecl stack shape: sole direct caller 0x00448C9C pushes seven DWORDs and
// removes 0x1C bytes after return. Return is consumed as a char*.
const char* ScriptExecDispatch(
    int command,
    int arg1,
    int arg2,
    const char* str1,
    const char* str2,
    int* intResult,
    void** objectResult);

} // namespace zs1
