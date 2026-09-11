#include "zHighScores.h"
#include "zCommon.h"
#include "zUserMngr.h"

#include <string.h>
#include <new>

#if defined(_MSC_VER) || defined(__i386__)
struct HWND__;
struct HKEY__;
class STREAM;
#include "mapedit/core/string.hpp"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
extern "C" int __stdcall CopyFileA(const char*, const char*, int);
#endif

namespace zs1 {

zHighScores* g_HighScores = nullptr;

zHighScoreRecord::zHighScoreRecord() noexcept
    : m_name(nullptr), m_value1(0), m_value2(0), m_invalid(0)
{
    m_pad[0]=m_pad[1]=m_pad[2]=0;
}

// Target scalar deleting destructor folds this one-field teardown into the wrapper.
zHighScoreRecord::~zHighScoreRecord()
{
    AssignCString(&m_name, nullptr);
}

int zHighScoreRecord::Checksum() const
{
    if (m_invalid)
        return -999;

#if defined(_MSC_VER) || defined(__i386__)
    // Retail builds exactly: name + " " + Printf("%d",value1) + " " +
    // Printf("%d",value2), then passes the STRING buffer to Adler32.
    STRING text;
    text += m_name;
    text += " ";
    {
        STRING number = Printf("%d", m_value1);
        text += &number;
    }
    text += " ";
    {
        STRING number = Printf("%d", m_value2);
        text += &number;
    }
    return static_cast<int>(Adler32(text.m_buf));
#else
    const char* name = m_name;
    const unsigned int len = std::strlen(name) + 64;
    char* text = static_cast<char*>(::operator new(len));
    MAPEDIT_SNPRINTF(text, len, "%s %d %d", name, m_value1, m_value2);
    const int result = static_cast<int>(Adler32(text));
    ::operator delete(text);
    return result;
#endif
}

zHighScores::zHighScores() noexcept
    : m_unknown0(nullptr), m_records(nullptr), m_dataSize(0), m_capacity(0), m_loaded(0)
{
    m_pad[0]=m_pad[1]=m_pad[2]=0;
}

zHighScores::~zHighScores()
{
    Clear();
}

void zHighScores::Clear()
{
    for (int i = 0; i < m_dataSize; ++i)
        delete m_records[i];
    if (m_records)
        std::free(m_records);
    m_records = nullptr;
    m_dataSize = 0;
    m_capacity = 0;
}

zHighScoreRecord* zHighScores::AppendRecord()
{
    if (m_dataSize == m_capacity) {
        m_capacity += 8;
        m_records = static_cast<zHighScoreRecord**>(
            std::realloc(m_records, static_cast<unsigned int>(m_capacity) * sizeof(zHighScoreRecord*)));
    }

    ++m_dataSize;
    zHighScoreRecord* record = new zHighScoreRecord();
    m_records[m_dataSize - 1] = record;
    return record;
}

int zHighScores::Add(const char* name, int value1, int value2)
{
    Load();

    int insertAt = 0;
    while (insertAt < m_dataSize) {
        zHighScoreRecord* current = m_records[insertAt];
        if (value1 > current->m_value1)
            break;
        if (value1 == current->m_value1 && value2 > current->m_value2)
            break;
        ++insertAt;
    }

    AppendRecord();
    for (int i = m_dataSize - 1; i > insertAt; --i) {
        zHighScoreRecord* dst = m_records[i];
        zHighScoreRecord* src = m_records[i - 1];
        AssignCString(&dst->m_name, src->m_name);
        dst->m_value1 = src->m_value1;
        dst->m_value2 = src->m_value2;
        dst->m_invalid = src->m_invalid;
    }

    zHighScoreRecord* record = m_records[insertAt];
    AssignCString(&record->m_name, name);
    record->m_value1 = value1;
    record->m_value2 = value2;
    record->m_invalid = 0;
    Save();
    return insertAt;
}

const char* zHighScores::GetName(int index) const
{
    return m_records[index]->m_name;
}

int zHighScores::GetValue1(int index) const
{
    return m_records[index]->m_value1;
}

int zHighScores::GetValue2(int index) const
{
    return m_records[index]->m_value2;
}

void zHighScores::BuildRecordsFileName(char* out, unsigned int outSize, bool defaultFile) const
{
    BuildRecordsFileNameBuffer(this,out,outSize,defaultFile);
}

bool zHighScores::Save()
{

#if defined(_MSC_VER) || defined(__i386__)
    // Retail builds the current records name once for BackupFile, destroys that
    // temporary, then builds it a second time for fopen("w").
    {
        STRING fileName = BuildRecordsFileName(false);
        BackupFile(fileName.m_buf);
    }
    FILE* file;
    {
        STRING fileName = BuildRecordsFileName(false);
        file = std::fopen(fileName.m_buf, "w");
    }
#else
    char fileName[1024];
    BuildRecordsFileName(fileName, sizeof(fileName), false);
    BackupFile(fileName);
    FILE* file = std::fopen(fileName, "w");
#endif
    if (file) {
        for (int i = 0; i < m_dataSize; ++i) {
            const zHighScoreRecord* record = m_records[i];
            std::fprintf(file, "%s\n", record->m_name);
            std::fprintf(file, "%d\n", record->m_value1);
            std::fprintf(file, "%d\n", record->m_value2);
            std::fprintf(file, "%d\n", record->Checksum());
        }
        std::fclose(file);
    }
    return true;
}

bool zHighScores::Load()
{
    Clear();

    FILE* file;
#if defined(_MSC_VER) || defined(__i386__)
    {
        STRING fileName = BuildRecordsFileName(false);
        file = std::fopen(fileName.m_buf, "r");
    }
#else
    char fileName[1024];
    BuildRecordsFileName(fileName, sizeof(fileName), false);
    file = std::fopen(fileName, "r");
#endif
    if (!file) {
        ResetRecordsFile(false);
#if defined(_MSC_VER) || defined(__i386__)
        {
            STRING fileName = BuildRecordsFileName(false);
            file = std::fopen(fileName.m_buf, "r");
        }
#else
        BuildRecordsFileName(fileName, sizeof(fileName), false);
        file = std::fopen(fileName, "r");
#endif
        if (!file) {
            m_loaded = 1;
            return true;
        }
    }

#if defined(_MSC_VER) && _MSC_VER <= 1200
    while (!(file->_flag & 0x10)) {
#else
    while (!std::feof(file)) {
#endif
        char* nameLine = ReadLine(file);
        if (!nameLine)
            break;

#if defined(_MSC_VER) || defined(__i386__)
        STRING nameCopy(nameLine);
        char* value1Line = ReadLine(file);
        if (!value1Line)
            break;
        const int value1 = std::atoi(value1Line);

        char* value2Line = ReadLine(file);
        if (!value2Line)
            break;
        const int value2 = std::atoi(value2Line);

        char* checksumLine = ReadLine(file);
        if (!checksumLine)
            break;
        const int storedChecksum = std::atoi(checksumLine);

        zHighScoreRecord* record = AppendRecord();
        AssignCString(&record->m_name, nameCopy.m_buf);
#else
        char* nameCopy = nullptr;
        AssignCString(&nameCopy, nameLine);
        char* value1Line = ReadLine(file);
        if (!value1Line) { AssignCString(&nameCopy, nullptr); break; }
        const int value1 = std::atoi(value1Line);
        char* value2Line = ReadLine(file);
        if (!value2Line) { AssignCString(&nameCopy, nullptr); break; }
        const int value2 = std::atoi(value2Line);
        char* checksumLine = ReadLine(file);
        if (!checksumLine) { AssignCString(&nameCopy, nullptr); break; }
        const int storedChecksum = std::atoi(checksumLine);
        zHighScoreRecord* record = AppendRecord();
        AssignCString(&record->m_name, nameCopy);
#endif
        record->m_value1 = value1;
        record->m_value2 = value2;
        record->m_invalid = 0;

        if (record->Checksum() != storedChecksum) {
            if (storedChecksum != -999) {
                // Retail 0x00474BAE reassigns the original name; it does NOT
                // replace it with an empty string. Only the two values are zeroed.
#if defined(_MSC_VER) || defined(__i386__)
                AssignCString(&record->m_name, nameCopy.m_buf);
#else
                AssignCString(&record->m_name, nameCopy);
#endif
                record->m_value1 = 0;
                record->m_value2 = 0;
            }
            record->m_invalid = 1;
        }
#if !defined(_MSC_VER) && !defined(__i386__)
        AssignCString(&nameCopy, nullptr);
#endif
    }

    std::fclose(file);
    m_loaded = 1;
    return true;
}

// MapEditZS1.exe 0x00474D10..0x00474E4E.
void zHighScores::ResetRecordsFile(bool reload)
{
    Clear();

#if defined(_WIN32)
    {
        STRING current=BuildRecordsFileName(false);
        BackupFile(current.m_buf);
    }

    int copied;
    {
        STRING current=BuildRecordsFileName(false);
        STRING defaults=BuildRecordsFileName(true);
        copied=::CopyFileA(defaults.m_buf,current.m_buf,0);
    }

    if (!copied) {
        // Retail rebuilds both names after CopyFileA failure, formats a temporary
        // STRING and destroys it.  There is no MYERROR::Log call on this path.
        STRING current=BuildRecordsFileName(false);
        STRING defaults=BuildRecordsFileName(true);
        STRING errorText=Printf("error file %s to %s",defaults.m_buf,current.m_buf);
        (void)errorText;
    }
#else
    // Host-only syntax-gate path; production target is Win32/x86.
    char current[1024];
    BuildRecordsFileName(current,sizeof(current),false);
    BackupFile(current);
#endif

    if (reload)
        Load();
}

} // namespace zs1
