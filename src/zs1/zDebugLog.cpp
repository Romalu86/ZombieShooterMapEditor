#include "zDebugLog.h"
#include "zCommon.h"

#include <ctime>
#include <cstdio>
#include <cstring>
#include <new>

namespace zs1 {

zDebugLog* g_DebugLog = nullptr;

namespace {

void ReplaceFirst(char* text, char needle, char replacement)
{
    if (!text)
        return;
    if (char* p = std::strchr(text, needle))
        *p = replacement;
}

} // namespace

zDebugLog::zDebugLog()
    : m_file(nullptr), m_fileName(nullptr), m_openTried(0)
{
    m_pad[0]=m_pad[1]=m_pad[2]=0;
    // The original constructor calls the engine date/time helpers whose format strings are
    // %Y-%m-%d (0x0049AC80) and %H:%M:%S (0x0049AC74).
    char date[64] = {0};
    char timeText[64] = {0};
    std::time_t now = std::time(nullptr);
    std::tm local = { 0 };
#if defined(_MSC_VER) && _MSC_VER < 1400
    {
        std::tm* nativeLocal = std::localtime(&now);
        if (nativeLocal)
            local = *nativeLocal;
    }
#elif defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::strftime(date, sizeof(date), "%Y-%m-%d", &local);
    std::strftime(timeText, sizeof(timeText), "%H:%M:%S", &local);
    ReplaceFirst(timeText, ':', 'h');
    ReplaceFirst(timeText, ':', 'm');

    const char* base = g_WindowsUserPath;
    const char* logs = "Logs\\";
    const char* middle = "_dbglog ";
    const char* ext = ".txt";
    const unsigned int len = std::strlen(base) + std::strlen(logs) + std::strlen(date)
        + std::strlen(middle) + std::strlen(timeText) + std::strlen(ext);

    char* name = static_cast<char*>(::operator new(len + 1));
    MAPEDIT_SNPRINTF(name, len + 1, "%s%s%s%s%s%s", base, logs, date, middle, timeText, ext);
    m_fileName = name;
}

// Compiler-generated scalar deleting destructor around 0x00474450.
zDebugLog::~zDebugLog()
{
    AssignCString(&m_fileName, nullptr);
    if (m_file) {
        std::fclose(m_file);
        m_file = nullptr;
    }
}

void zDebugLog::Open()
{
    if (!m_openTried && !m_file)
        m_file = std::fopen(m_fileName, "w");
    m_openTried = 1;
}

void zDebugLog::Write(const char* text)
{
    Open();
    if (!text || !m_file)
        return;
    std::fputs(text, m_file);
    std::fputs("\n", m_file);
    std::fflush(m_file);
}

} // namespace zs1
