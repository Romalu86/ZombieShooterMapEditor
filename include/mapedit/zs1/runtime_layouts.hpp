#pragma once
// Target-proven ZS1 raw layouts used only where retail native dispatch performs
// direct offset access. These are views over active objects; they own no data.
namespace zs1 { namespace video {
#pragma pack(push,4)
struct VID_EXDATA_PREFIX32 {
    unsigned int unknown00;
    unsigned int flags;
    unsigned int unknown08;
    float collisionWeight;
    float trainValue10;
    float trainValue14;
    float scriptValue18;
    float scriptValue1C;
    float scriptValue20;
    unsigned int unknown24;
    int maxAmmo;
    int scriptValue2C;
    unsigned int unknown30;
    int buildTime;
};
struct VID_LAYOUT32 {
    unsigned int vptr; int index; unsigned int nameString; unsigned int collisionClassFlags;
    int spriteClass; unsigned int flag; unsigned int collisionMask;
    float footprintWidth,footprintHeight,footprintZ; int defaultMaxHp;
    float moveSpeed,moveSpeedMirror,verticalMoveSpeed; unsigned int unknown038;
    float moveAcceleration,moveDeceleration; unsigned int unknown044,weaponState48;
    float scriptValue4C; int fireDamage; unsigned char unknown054_to_063[0x10];
    unsigned int childLinkVid; unsigned char unknown068_to_06B[4];
    float groundStepUp,groundStepDown; int lifeTime,noDirectionMode;
    int noAnimCadr[17]; unsigned char unknown0C0_to_103[0x44]; int animationDuration[17];
    unsigned char unknown148_to_213[0xCC]; int childVidSigned[17];
    unsigned int childVid0_2[3],animationRoute264,childVid4_7[4],weaponVid,childVid9_16[8];
    int noChild[17]; unsigned char unknown2E0_to_2E7[8];
    float scriptFloat2E8,scriptFloat2EC,scriptFloat2F0; unsigned char unknown2F4_to_2F9[6];
    unsigned short frameSpeed; short dotFrameCount; unsigned char unknown2FE_to_303[6];
    int aniBegCadr[17],aniDirCadrs[17]; float halfWidth,halfHeight; int layer;
    unsigned int unknown398; int unitLimitDefault,unitLimit[4],entityCount[4],deaths[4],recolors[4],maxHp[4];
    unsigned char unknown3F0_to_45B[0x6C]; int collisionScript; unsigned char unknown460_to_467[8];
    unsigned int exData,unknown46C,scriptRelatedVid470; int nLinkDots;
    unsigned int dotCoords,dotFrameStarts,unknown480,runtimeFlags484,propertyFlags488; int notCreateAsChild;
};
#pragma pack(pop)
}}
namespace zs1 { namespace engine {
#pragma pack(push,4)
struct ACT { int command,a,b,c; };
struct SPRITE_LAYOUT32 {
    unsigned int vptr,unknown04; int begCadr,noCadr,endCadr; unsigned int tactTime,createTimeOrUnknown18,vid;
    float speed,zSpeed; unsigned int flag; int noRef; float x,y,z; unsigned int goal,child,parent; int ani;
    unsigned char direction,pad4D[3]; unsigned int unknown50,actionsVptr; int actionsCount,actionsCapacity;
    unsigned int actionsData,exData; int hp;
};
#pragma pack(pop)
}}
