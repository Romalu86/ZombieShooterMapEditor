#pragma once

#include "mapedit/legacy_compiler.hpp"

#if defined(_MSC_VER) || defined(__i386__)
struct _iobuf; typedef _iobuf FILE;
#else
#include <cstdio>
#endif


namespace zs1 {

// Runtime-owned path recovered from global 0x004A6B4C.
extern char* g_WindowsUserPath;


void AssignCString(char** destination, const char* source);

bool SplitLine(char* text, char** left, char** right, char delimiter);

char* ReadLine(FILE* file);

void BackupFile(const char* fileName);

unsigned int Adler32(const char* text);


} // namespace zs1
