#pragma once

#include "mapedit/legacy_compiler.hpp"
#include <stddef.h>

namespace zs1 { namespace layout32 {
typedef unsigned int ptr32;
#pragma pack(push,4)
struct zArg { ptr32 vptr; ptr32 name; int intValue; ptr32 strValue; int argType; };
struct zArgList { ptr32 vptr; ptr32 items; int size; int capacity; unsigned char dirty; unsigned char pad[3]; };
struct zUser { zArgList base; ptr32 name; };
struct zUserMngr { zArgList base; ptr32 rootPath; ptr32 users[100]; zArgList globals; int curUser; };
struct zDebugLog { ptr32 vptr; ptr32 file; ptr32 filename; unsigned char state; unsigned char pad[3]; };
struct zHighScoreRecord { ptr32 vptr; ptr32 name; int value1; int value2; unsigned char flag; unsigned char pad[3]; };
struct zHighScoresObserved { ptr32 unknown0; ptr32 records; int dataSize; int capacity; unsigned char loaded; unsigned char pad[3]; };
struct zHelpItem { ptr32 vptr; int value1; int value2; ptr32 text; int value3; int type; };
struct zHelpLevel { ptr32 vptr; ptr32 items; int size; int sizeAlloc; int levelNum; unsigned char loaded; unsigned char pad[3]; };
struct zHelpParserObserved { ptr32 unknown0; ptr32 levels; int levelSize; int levelCapacity; ptr32 rootPath; };
#pragma pack(pop)
} } // namespace zs1::layout32
