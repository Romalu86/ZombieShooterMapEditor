#pragma once
// MENU/GROUP/GROUPS/MOUSETIPS/EX_SPRITE_DATA owners. Included in ABI order by mapedit/runtime.hpp.
class MENU : public SPRITE_LIST {
public:
    MENU();
    ~MENU();
    uint32_t clickFlags;          // +0x10; original lClick/rClick bitfield storage
    SPRITE* sprite;              // +0x14
    SPRITE* SpriteUnderCursor();
    int Control(INPUT* input);
    int NVidUnderCursor();
    int NDirUnderCursor();
    int IsLClick();
    int IsRClick();
    SPRITE* FindNamed(const STRING& name);
    int Load(const STRING* name);
    int Save(const STRING* name);
    int DeleteFromFile(const STRING* name);
    void Error(TYPE_ERROR type,char* text,unsigned long err);
    int Delete(SPRITE* spr);
};
class GROUP : public SPRITE_LIST {
public:
    float x;                  // +0x10
    float y;                  // +0x14
    unsigned long behave;     // +0x18
    int nPlayer;              // +0x1C
    GROUP* nextGroup;         // +0x20
    GROUP(GROUP* prev,SPRITE* spr);
    virtual ~GROUP();
    void Draw();
    void DrawNumber(int number);
    void Insert(SPRITE* spr);
    GROUP* PrevGroup();
    float X() const;
    float Y() const;
    float DistanceTo(const SPRITE* spr);
};
class GROUPS {
public:
    GROUPS();
    GROUP first;
    ~GROUPS();              // +0x00; intrusive sentinel, nextGroup at +0x20
    void DeleteAll();
    void DrawNumber();
    void InsertToNearGroup(SPRITE* spr);
    void ShiftFirstLeft();
    void ShiftFirstRight();
    GROUP* CreateNewGroup(SPRITE* spr);
    void DeletePointerToSprite(SPRITE* spr);
    GROUP* First();
    const GROUP* First() const;
    GROUP* Next(const GROUP* group);
    const GROUP* Next(const GROUP* group) const;
    void Save(RESOURCE* res);
    void Load(RESOURCE* res);
};
class MOUSETIPS {
public:
    MOUSETIPS();
    virtual ~MOUSETIPS();
    void* tip;
    void Tact(INPUT* input);
    void Clear();
    void DeletePointerToSprite(SPRITE* spr);
    int IsOut();
};

class EX_SPRITE_DATA {
public:
    // ZS1 target 0x0044D220 / sizeof 0x44.  The aliases retain the historical
    // A53 source spellings while moving every live access onto the target ABI.
    float lastX;                              // +0x00 previousX
    float lastY;                              // +0x04 previousY
    float lastZ;                              // +0x08 previousZ
    int gridFrame;                            // +0x0C
    // +0x10: RealCurrentTime stamp used by ChangeCoor; retail ctor leaves it untouched.
    union { unsigned long unknown10; unsigned long coorTime; unsigned long changeCoorTime; }; // +0x10
    union { int lifeTime; unsigned long deathTimer; };                                  // +0x14
    union { int flag18; unsigned long birthSmokeTimer; };                               // +0x18
    // +0x1C: CurrentTime origin used by the property/table track in SPRITE::Tact.
    union { unsigned long unknown1C; unsigned long tableStartTime; };                    // +0x1C
    union { int unknown20; float tableCoeff; };                        // +0x20
    union { float moveSpeed; float maxSpeed; };                        // +0x24
    GAMMA gamma;                              // +0x28; two target DWORDs
    LIST<int> items;                          // +0x30..+0x3F
    STRING name;                              // +0x40; target comparison string
    EX_SPRITE_DATA(const SPRITE* sprite);
};

struct VID_DOT { float x,y,z; };

// MapEdit CodeView/ASM-visible prefix used by SPRITE::CreateChild.
// Only fields proven by direct retail accesses are exposed here; the rest of
// the historical WEAPON record remains intentionally opaque.
