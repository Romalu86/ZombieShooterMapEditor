#pragma once
// MAP owner and layout check. Included in ABI order by mapedit/runtime.hpp.
// Retail map.h exposes EmptyVid before the inline PopVid wrapper.  The umbrella
// runtime include reaches the global declaration only after map.hpp, so keep the
// same non-owning extern visible here for VC6 header-inline emission.
extern VID* EmptyVid;
class MAP {
public:
    MAP(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init);
    virtual ~MAP();
    virtual int WorkWndMessage(HWND__*, unsigned long, unsigned long, unsigned long);
    virtual void DeletePointerToSprite(SPRITE*);
    virtual int Tact();
    virtual void DrawSecondaryInfo();
    virtual void Release();
    virtual void Load(STRING name);
    virtual void Save(STRING name);
    int ScriptRun(int n_func,const SPRITE* var1,const SPRITE* var2,int var3);
    STRING ScriptVariable(STRING name);
    void RemoveSpriteFromLayer(SPRITE* spr);
    void InsertSpriteToLayer(SPRITE* spr);
    // Semantic names recovered from direct ZS1 owners 0x0041C010/0x0041C070.
    // These touch only the raw non-owning named-sprite tail list.
    void RemoveNamedSprite(SPRITE* spr);
    void InsertNamedSprite(SPRITE* spr);
    void DrawSpriteNames();
    virtual SPRITE* CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent);
    SPRITE* LoadSprite(STREAM* res,int version);
    SPRITE* ReadPointer(STREAM* res);
    SPRITE* Decode(SPRITE* old_sprite);
    SPRITE* OldLoadSprite(RESOURCE* res);
    void LoadVid(RESOURCE* res);
    void LoadWeapon(RESOURCE* res);
    VID* CreateVid(RESOURCE* res,int nvid);

    // Keep the retail ZS1 MAP layout unchanged (4096 in-object slots) and
    // extend the editor/runtime capacity through side storage.
    static constexpr int kRetailVidCapacity=MAPEDIT_MAX_VID;
    static constexpr int kVidCapacity=8192;
    VID*& VidSlot(int nvid);
    VID* VidSlot(int nvid) const;
    void ClearVidSlots();
    static int EncodeVidQuery(int nvid);

    int      m_fps;             // +0x004
    int      m_fpsCnt;          // +0x008
    uint32_t m_flags;           // +0x00C; init-success is bit 2
    uint32_t m_unknown10;       // +0x010
    float    m_speed;           // +0x014
    STRING   m_title;           // +0x018
    STRING   m_mapName;         // +0x01C
    STRING   m_startupLoad;      // +0x020; exact semantic name pending; MAP_EDIT ctor feeds this to Load
    STRING   m_prevMap;         // +0x024
    STRING   m_resName;         // +0x028
    int      m_noTact;          // +0x02C
    uint32_t m_unknown30;       // +0x030
    float    m_w;               // +0x034
    float    m_h;               // +0x038
    uint32_t m_shiftFlag;       // +0x03C
    float m_shiftX1,m_shiftX2,m_shiftY1,m_shiftY2,m_shiftX,m_shiftY; // +0x040..0x054
    SPRITE_LIST m_layers[MAPEDIT_MAP_LAYER_COUNT]; // +0x058..0x1A7; ZS1 target has 21 layers
    LOGIC      m_logic;         // +0x1A8; PORT1 bridge size is target-proven 0x870
    RESOURCE   m_resource;      // +0xA18
    RELATION   m_relation;      // +0xA58
    short*     m_groundz;       // +0xA78
    short*     m_tempGroundz;   // +0xA7C
    int        m_groundW;       // +0xA80
    int        m_groundH;       // +0xA84
    HINSTANCE__* m_instance;    // +0xA88
    HWND__*      m_hWnd;        // +0xA8C
    HACCEL__*    m_hAccel;      // +0xA90
    int          m_curArmy;     // +0xA94; current army/player index
    PLAYER*      m_player[4];   // +0xA98
    INPUT        m_input;       // +0xAA8
    MENU         m_menu;        // +0xAC8
    GROUPS       m_groups;      // +0xAE0
    int          m_noWeapon;    // +0xB04
    void*        m_weapon;      // +0xB08
    int          m_noVid;       // +0xB0C
    VID*         m_vids[MAPEDIT_MAX_VID]; // +0xB10, ZS1 target MAX_VID=4096
    MOUSETIPS    m_mousetips;   // +0x4B10
    LIST<SPRITE*> m_zs1TailList; // +0x4B18; target LIST_SPRITE (non-owning), vptr family differs from layer SPRITE_LIST

    int IsInitSuccess();
    int IsMapEdit();
    int OptLoad();
    // ZS1 DrawSnow 0x00422657 inlines map.h pause-bit access.
    int IsPaused() { return static_cast<int>((m_flags >> 4) & 1u); }
    STRING GetMouseTipsString();
    int OptEnemyAttackNeutralTrains();
    int OptDrawHpLines(const SPRITE* spr);
    float SizeX(); float SizeY();
    int ValidateXY(float x,float y);
    int ValidateVid(int nvid);
    VID* ReadVid(STREAM* res);
    void WriteVid(STREAM* res,const VID* vid);
    int StartTact();
    int DemoTact();
    // ZS1 DrawSnow folds these map.h one-field transforms into the caller.
    float ToScreenX(float x) { return x - m_shiftX; }
    float ToScreenY(float y) { return y - m_shiftY; }
    float ToScreenY(float y,float z);
    int ToScreenXInt(float x); int ToScreenYInt(float y);
    float FromScreenX(float screenX); float FromScreenY(float screenY);
    float GetGroundZ(float x,float y);
    float GetGroundZScr(float screenX,float screenY);
    float GetGroundZ(const VID* vid,float x,float y);
    int IsGroundPointBlocked(float x,float y,float z);
    int IsGroundSegmentStartBlocked(float x1,float y1,float z1,float x2,float y2,float z2);
    void ResetGroundZ();
    void SetGroundZ(float x,float y,float newZ);
    void SetTempGroundZ(float x,float y,float newZ);
    void ClearTempGroundZ(float x,float y,float newZ);
    void DeleteExtraVid();
    void ExchangeVid(VID* vid1,VID* vid2);
    void CreateEmptyHardwareGround();
    void RestoreDeviceObjects();
    void InvalidateDeviceObjects();
    void NetworkTact();
    void SetScrollBox(float minX,float minY,float maxX,float maxY);
    void ChangeSizeXY(float newSizeX,float newSizeY);
    SPRITE* FirstSprite(int nlayer,int* index) {
        SPRITE_LIST* const sprites=&m_layers[nlayer];
        *index=sprites->m_no-1;
        if (*index<0)
            return 0;
        while (!sprites->m_data[*index]) {
            --*index;
            if (*index<0)
                return 0;
        }
        return sprites->m_data[*index];
    }
    SPRITE* NextSprite(int nlayer,int* i);
    SPRITE* FirstSpriteByType(int nlayer,int* i,unsigned int type);
    SPRITE* NextSpriteByType(int nlayer,int* i,unsigned int type);
    SPRITE* GetSprite(int type,float x,float y,SPRITE* prev);
    SPRITE* GetSpriteScr(int type,float screenX,float screenY);
    SPRITE* FindNearestSprite(int type,float x,float y,float radius,SPRITE* prev);
    void FindSpritesInsidePolygon(int type,const POLYGON* polygon,SPRITE_LIST* list);
    int VidToControlBox(DIALOG_COMBO_BOX* list,unsigned long unitTypeMask,int selectVid);
    int VidToListBox(DIALOG_LIST_BOX* list,unsigned long unitTypeMask,int selectVid,int sort);
    STRING OpenSaveDialog(int save,const char* filter);
    STRING OpenDialog(const char* filter);
    STRING SaveDialog(const char* filter);
    int NextVid(int old_vid,unsigned int typeunit);
    int PrevVid(int old_vid,unsigned int typeunit);
    void SetShiftCoor(float centerX,float centerY,int effect);
    void ControlShiftCoor();
    SPRITE* Flagman(int army);
    SPRITE* Flagman();
    SPRITE* SpriteUnderCursor();
    MENU* Menu();
    int GetFPS();
    int NPlayer();
    PLAYER* Player(int narmy);
    PLAYER* Player();
    int GetScrollType();
    float GetTimeCoeff();
    void SetSelectSpriteUnderCursor(int flag);
    int OptSelectSpriteUnderCursor();
    VID* Vid(int nvid);
    void Error(int type,char* text,unsigned long value);
    void SetScrollType(int type);
    void DrawLayer(int layer);
    // Retail map.h/script.h source shape: these tiny MAP script-stack wrappers
    // are header-inline.  Direct target ExecFunc ASM inlines PopInt/PopStr/
    // PopSprite into case bodies, while the standalone COMDAT owners remain
    // available when VC6 decides not to inline them.
    int PopInt() {
        LOGICSTACK* value=&m_logic.stack.m_data[--m_logic.stack.m_no];
        return value->Int();
    }
    const STRING* PopStr() {
        LOGICSTACK* value=&m_logic.stack.m_data[--m_logic.stack.m_no];
        return value->String();
    }
    SPRITE* PopSprite() { return reinterpret_cast<SPRITE*>(PopInt()); }
    VID* PopVid(const char* operation) {
        const int nvid=PopInt();
        VID* vid=Vid(nvid);
        if(vid==EmptyVid && ::Error && operation[0])
            MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",operation,nvid);
        return vid;
    }
    void PushInt(int value) { m_logic.PushInt(value); }
    void PushStr(const STRING* value) { m_logic.PushStr(value); }
    void PushObject(const void* object,const STRING& context) { m_logic.PushObject(object,&context); }
    STRING FileName();
    void PauseOn();
    void PauseOff();
    void ReloadVid();
    void LoadInEndTact(const STRING* filename);
    void SetFlagman(int army,SPRITE* sprite);
    int ExecFunc(int command);
};

// Standard-layout mirror: raw ABI anchors avoid compiler-specific offsetof warnings
// from polymorphic helper classes while preserving every checked original offset.
struct MAP_LAYOUT_CHECK {
    uint8_t pad00[0x0C];
    uint32_t flags;                     // +0x000C
    uint8_t pad10[0x0C];
    uint32_t mapName;                   // +0x001C
    uint32_t startupLoad;               // +0x0020
    uint8_t pad24[0x34];                // through +0x0057
    uint8_t layers[0x150];              // +0x0058, 21 * 0x10
    uint8_t logic[0x870];               // +0x01A8
    uint8_t resource[0x40];             // +0x0A18
    uint8_t relation[0x20];             // +0x0A58
    uint32_t groundz;                   // +0x0A78
    uint8_t padA7C[0x0C];
    uint32_t instance;                  // +0x0A88
    uint32_t hwnd;                      // +0x0A8C
    uint32_t accel;                     // +0x0A90
    int currentArmy;                    // +0x0A94
    uint32_t player[4];                 // +0x0A98
    uint8_t input[0x20];                // +0x0AA8
    uint8_t menu[0x18];                 // +0x0AC8
    uint8_t groups[0x24];               // +0x0AE0
    uint8_t noWeaponAndWeapon[0x08];    // +0x0B04
    int noVid;                          // +0x0B0C
    uint32_t vids[MAPEDIT_MAX_VID];     // +0x0B10
    uint8_t mousetips[0x08];            // +0x4B10
    uint8_t tailList[0x10];             // +0x4B18
};


