#pragma once

#include "mapedit/legacy_compiler.hpp"

class STRING;


namespace zs1 {

#pragma pack(push, 4)
class zHighScoreRecord {
public:
    // Convenience constructor; the original AppendRecord path initialized these fields inline.
    zHighScoreRecord() noexcept;
    // vtable scalar deleting destructor: FUNCTION ZS1 0x004745B0
    virtual ~zHighScoreRecord();

    int Checksum() const;

    char* m_name;             // +0x04
    int m_value1;             // +0x08
    int m_value2;             // +0x0C
    unsigned char m_invalid;   // +0x10
    unsigned char m_pad[3];
};
#pragma pack(pop)

#if defined(_M_IX86)
#endif

#pragma pack(push, 4)
class zHighScores {
public:
    // Convenience constructor; the original executable did not emit it as a separate owner.
    zHighScores() noexcept;
    ~zHighScores();

    void Clear();
    zHighScoreRecord* AppendRecord();
    int Add(const char* name, int value1, int value2);
    const char* GetName(int index) const;
    int GetValue1(int index) const;
    int GetValue2(int index) const;

    STRING BuildRecordsFileName(bool defaultFile) const;
    void BuildRecordsFileName(char* out, unsigned int outSize, bool defaultFile) const;
    bool Save();
    bool Load();
    void ResetRecordsFile(bool reload);

    bool IsLoaded() const noexcept { return m_loaded != 0; }
    int GetCount() const noexcept { return m_dataSize; }

private:
    void* m_unknown0;                 // +0x00, owner unresolved in this pass
    zHighScoreRecord** m_records;     // +0x04
    int m_dataSize;                   // +0x08
    int m_capacity;                   // +0x0C
    unsigned char m_loaded;            // +0x10
    unsigned char m_pad[3];
};
#pragma pack(pop)

void BuildRecordsFileNameBuffer(const zHighScores* self,char* out,unsigned int outSize,bool defaultFile);

extern zHighScores* g_HighScores; // GLOBAL: ZS1 0x005F1AD8

} // namespace zs1
