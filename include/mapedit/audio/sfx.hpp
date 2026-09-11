#pragma once
// SFX/SFXBUFFER/MIX owners. Included in ABI order by mapedit/runtime.hpp.
class SFX {
public:
    SFX();
    STRING filename[8];              // +0x00..+0x1F
    STRING ffbname[8];               // +0x20..+0x3F
    uint32_t property;               // +0x40
    uint8_t priority;                // +0x44
    uint8_t pad45[3];
    int volumeBias;             // +0x48; ZS1 serialized value * 10
    void* buffer[8];                 // +0x4C..+0x6B IDirectSoundBuffer*
    int noRandomFile;                // +0x6C
    ~SFX();
    void Release();
    int IsLoaded();
    int IsLooped();
    int NoRandomSound();
    int IsDistIndepended();
    int IsDirIndepended();
    int IsVIP();
    int Priority();
    int MaxSameSFX();
    void Load(STRING* const name,unsigned long property,unsigned char priority,int volumeBias,STRING* const ffbname,SOUND* sound);
    void ReLoad(SOUND* sound);
    IDirectSoundBuffer* Duplicate(IDirectSound* directSound);
};

class SFXBUFFER {
public:
    SFXBUFFER();
    virtual ~SFXBUFFER();            // vptr +0x00
    void* directSoundBuffer;         // +0x04
    int active;                      // +0x08
    int sfxIndex;                    // +0x0C
    int volume;                      // +0x10
    int balance;                     // +0x14
    unsigned long startTime;         // +0x18
    void Destroy();
    int IsPlayed();
    int GetSFX();
    unsigned int GetStartTime();
    int Volume();
    int Balance();
    int StartMove();
    void Stop();
    void Pause();
    void Play(int balance,int volume);
    void Resume();
    int CheckPlay();
    void Load(int nsfx,IDirectSoundBuffer* newSoundEffects);
private:
    void Error(TYPE_ERROR type,const char* text,unsigned long err) const;
};
struct SFXBUFFER_ABI_CHECK {
    void* vptr;
    void* directSoundBuffer;
    int active;
    int sfxIndex;
    int volume;
    int balance;
    unsigned long startTime;
};

class MIX {
public:
    MIX();
    int sfxIndex;
    int unknown04;
    int balance;
    int volume;
};

