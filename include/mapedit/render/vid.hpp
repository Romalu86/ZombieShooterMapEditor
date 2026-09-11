#pragma once
// VID base owner. Included in ABI order by mapedit/runtime.hpp.
class VID {
public:
    VID();
    virtual VID* CreateMirror();                                      // vtable +0x00
    virtual ~VID();                                                   // +0x04 scalar deleting destructor in retail
    virtual void DrawVidToVid(const SPRITE* sprite);                  // +0x08
    virtual void Draw(const SPRITE* sprite);                          // +0x0C
    virtual void DrawShadow(const SPRITE* sprite);                    // +0x10
    virtual void DrawToVid(const SPRITE* sprite,const VID_TEXCOOR* coor,TEXTURE* video_tex,TEXTURE* z_tex); // +0x14
    virtual void Load(RESOURCE* res);                                 // +0x18
    virtual void SetGamma(const GAMMA* gamma,unsigned int n_gamma);   // +0x1C
    virtual void SetScriptPackedValue125(int value);                   // +0x20; ZS1-only script setter slot
    virtual int HaveShadow();                                         // +0x24; target base returns 0
    virtual void SetLayer();                                          // +0x28; target clears/derives layer

    int m_idx;                   // +0x004 (compiler vfptr occupies +0x000)
    STRING m_name;              // +0x008; genuine retail STRING member
    uint32_t m_unknown0C;        // +0x00C
    uint32_t m_spriteClass;      // +0x010
    uint32_t m_flag;             // +0x014
    int m_unknown18;             // +0x018
    float m_footprintWidth;      // +0x01C; exact MapEdit editor snap step X
    float m_footprintHeight;     // +0x020; exact MapEdit editor snap step Y
    float m_hitVerticalOffset;   // +0x024; vertical hit-test offset used by SPRITE/REGION
    int m_baseHp;                // +0x028; base/default HP used by script VID HP scaling
    float m_defaultMaxSpeed;     // +0x02C; ZS1 moveSpeed
    float m_moveSpeedMirror;     // +0x030; ZS1 mirrored move speed
    float m_maxZSpeed;           // +0x034; ZS1 verticalMoveSpeed
    float m_moveSpeed38;         // +0x038; fourth speed-like serialized parameter
    float m_acceleration;        // +0x03C; 999999 sentinel means instant acceleration
    float m_deceleration;        // +0x040; 999999 sentinel means instant stop
    float m_childDirectionLock;  // +0x044; ZS1 inverse/direction-lock parameter
    int m_weaponIndex;           // +0x048; ZS1 weaponState48 / WEAPON record index
    float m_scriptValue4C;       // +0x04C; ZS1 Get/SetVidData selector 134
    int m_fireDamage;            // +0x050; base recursive fire-damage term
    float m_linkOffsetX;         // +0x054; serialized linked-child X offset
    float m_linkOffsetY;         // +0x058; serialized linked-child Y offset
    float m_linkOffsetZ;         // +0x05C; serialized linked-child Z offset
    int m_linkVidIndex;          // +0x060; serialized linked VID index
    VID* m_linkVid;               // +0x064; resolved linked VID pointer
    float m_groundOffset;         // +0x068; vertical cruise/ground offset
    float m_groundToleranceAbove; // +0x06C; CanPlace max ground-z delta
    float m_groundToleranceBelow; // +0x070; CanPlace max z-ground delta
    uint32_t m_defaultDeathTimer; // +0x074; EX_SPRITE_DATA ctor source

    unsigned int m_noDirections; // +0x078
    int m_noAnimCadr[17];         // +0x07C..+0x0BF
    int m_aniSfx[17];             // +0x0C0..+0x103
    int m_aniFrameSpeed[17];      // +0x104..+0x147
    float m_aniSpawnX[17];        // +0x148..+0x18B
    float m_aniSpawnY[17];        // +0x18C..+0x1CF
    float m_aniSpawnZ[17];        // +0x1D0..+0x213
    int m_aniSpawnMode[17];       // +0x214..+0x257
    VID* m_aniChildVid[17];       // +0x258..+0x29B
    int m_aniFireCount[17];       // +0x29C..+0x2DF
    GAMMA m_gamma;                // +0x2E0; genuine retail GAMMA member
    float m_colorScaleR;          // +0x2E8
    float m_colorScaleG;          // +0x2EC
    float m_colorScaleB;          // +0x2F0
    STRING m_resourceName;        // +0x2F4; genuine retail STRING member
    uint16_t m_extraTypeFlags;    // +0x2F8
    uint16_t m_phaseRandomInterval; // +0x2FA
    short m_dotFrameCount;        // +0x2FC
    short m_regionTileStepX;      // +0x2FE
    short m_regionTileStepY;      // +0x300
    uint8_t opaque302[0x304-0x302];
    int m_aniFrameStart[17];      // +0x304..+0x347
    int m_aniFrameLimit[17];      // +0x348..+0x38B
    float m_snapOffsetX;          // +0x38C
    float m_snapOffsetY;          // +0x390
    int m_layer;                  // +0x394
    int m_editorDirectionOffset;  // +0x398
    int m_limit394;               // +0x39C; global per-VID creation limit (legacy source name retained)
    int m_limit398[4];            // +0x3A0..+0x3AF; per-army creation limits
    int m_entitiesNumber[4];      // +0x3B0..+0x3BF
    int m_killed[4];              // +0x3C0..+0x3CF
    int m_reColored[4];           // +0x3D0..+0x3DF
    int m_maxHp[4];               // +0x3E0..+0x3EF
    GAMMA m_gammaByArmy[4];       // +0x3F0..+0x40F
    int m_eventFunction[21];      // +0x410..+0x463; ZS1 has 21 script/event slots
    unsigned long m_lastEntityTime; // +0x464; runtime entity timestamp
    WEAPON* m_weapon;             // +0x468; ZS1 exData / converted WEAPON pointer
    VID* m_mirrorNext;            // +0x46C
    VID* m_exchangeVid;           // +0x470
    int m_nLinkDots;              // +0x474
    VID_DOT* m_linkDots;          // +0x478
    int* m_dotFrameStarts;        // +0x47C
    int m_moveTactData;           // +0x480
    int m_exSpriteData;           // +0x484
    uint32_t m_propertyBits;      // +0x488
    uint32_t m_prop;              // +0x48C
    int PropHide() { return static_cast<int>((m_propertyBits>>6)&1u); }
    int PropAlwaysTop() { return static_cast<int>(m_flag&0x8000u); }
    int PropDblLight() { return static_cast<int>(m_flag&0x00800000u); }
    void Error(int type,char* text,unsigned long err);
    void SetPropHide(int flag);
    int IsSpriteType(unsigned int type) const { return static_cast<int>(m_unknown0C & type); }
    int IsExtraType();
    int IsEmptyType() { return m_extraTypeFlags==0; }
    int IsTextureType() { return static_cast<int>(m_extraTypeFlags&0x0001u); }
    int IsAlphaType() { return static_cast<int>(m_extraTypeFlags&0x0002u); }
    int IsZBufferType() { return static_cast<int>(m_extraTypeFlags&0x0004u); }
    int IsPaletteType() { return static_cast<int>(m_extraTypeFlags&0x0008u); }
    int IsNewVersionType() { return static_cast<int>(m_extraTypeFlags&0x0010u); }
    int IsHardwareType() { return static_cast<int>(m_extraTypeFlags&0x0020u); }
    int IsLightType();
    int IsCompressType() { return static_cast<int>(m_extraTypeFlags&0x0100u); }
    int IsAltGammaType() { return static_cast<int>(m_extraTypeFlags&0x0400u); }
    void SetAltGammaType();
    int IsDXTType() { return static_cast<int>(m_extraTypeFlags&0x0800u); }
    int IsPseudo3DType() { return static_cast<int>(m_extraTypeFlags&0x1000u); }
    int IsFontType() { return static_cast<int>(m_extraTypeFlags&0x4000u); }
    void SetExtraType() { m_extraTypeFlags|=0x0200u; }
    int PropSkipMapEd() { return static_cast<int>(m_flag&0x2000u); }
    int PropHash() { return static_cast<int>(m_flag & 0x40u); }
    int PropGamma() { return static_cast<int>(m_flag&0x00000800u); }
    int PropBlur() { return static_cast<int>(m_flag&0x00200000u); }
    int PropGround() { return static_cast<int>(m_flag&0x40000000u); }
    int PropWave() { return static_cast<int>(m_flag&0x00010000u); }
    int PropHardwareDirect() { return static_cast<int>(m_flag&0x20000000u); }
    int PropBuildVidZToGridZ() { return static_cast<int>(m_flag&0x20u); }
    int PropBuildSizeToGridZ() { return static_cast<int>(m_flag&0x08u); }
    int PropWind() { return static_cast<int>(m_flag&0x1000u); }
    void SetChildAndLink();
    void LoadParameters(RESOURCE* res);
    int PropInvisibleForEnemy();
    int PropRadialDamage();
    int PropNotDamageForFriend();
    int IsInvulnerable();
    int PropVertDir() { return static_cast<int>(m_flag & 0x00080000u); }
    int PropBirthAsSmoke() const { return static_cast<int>(m_flag&0x00000080u); }
    int PropTrack() const { return static_cast<int>(m_flag&0x00000010u); }
    int PropChildInEnd() { return static_cast<int>(m_flag&0x00040000u); }
    int PropRandBirth() const { return static_cast<int>(m_flag&0x00000001u); }
    int PropZeroZ() { return static_cast<int>(m_flag & 0x00000200u); }
    int PropNoise() { return static_cast<int>(m_flag&0x00000100u); }
    int PropOnePhase() { return static_cast<int>(m_flag&0x01000000u); }
    int PropGravity() const { return static_cast<int>(m_flag&0x00000002u); }
    int PropGravity2() const { return static_cast<int>(m_flag&0x00000004u); }
    int PropSelfMoving() const { return static_cast<int>(m_flag&0x08000000u); }
    int PropRandSpeed();
    int PropRandZSpeed();
    int PropBounce();
    int PropMoveWithAnyDirection() { return static_cast<int>(m_flag&0x00100000u); }
    int PropCrush() { return static_cast<int>(m_flag&0x00004000u); }
    int CanFight() const;
    float CalculateZSpeed(float delta_z,float size) const;
    int NoSprites() { return m_entitiesNumber[0]+m_entitiesNumber[1]+m_entitiesNumber[2]+m_entitiesNumber[3]; }
    int NoSprites(int army);
    int GetMaxHp(int army);
    void SetHpCoeff(int army,int hpPercent);
    void SetMaxHp(int army,int newHp);
    int Killed(int army);
    int Killed();
    int ReColored(int army);
    int ReColored();
    void IncreaseNoSprites(int army);
    void DecreaseNoSprites(int army);
    void ResetSprites();
    int RealDirection(ANGLE direction);
    ANGLE SteppedDirection(ANGLE direction);
    int PropNotCreateAsChild();
    void SetPropNotCreateAsChild(int on);
    int PropNotChangeLinkerCoor() { return static_cast<int>(m_flag & 0x02000000u); }
    int GetMaxAmmo();
    int GetFireDamage();
    int GetBuildTime();
    STRING GetNumberName();
    void SetGridZ(const SPRITE* sprite);
    void ResetGridZ(const SPRITE* sprite);
    static void SetViewPort(int x0,int y0,int x1,int y1);
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

// MapEdit CodeView / original VID_HARDWARE layout.  This is a genuine
// derived polymorphic class; MSVC reuses VID's vfptr at +0x000.
