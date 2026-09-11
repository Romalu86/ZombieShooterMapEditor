#pragma once
// RAIL/BUILDING/CREATURE/PLANE/MAN/CANNON/LINKER owner family. Included in ABI order by mapedit/runtime.hpp.
class RAIL : public TERRAIN {
public:
    RAIL(VID*,float,float,float,ANGLE,SPRITE*);
    ~RAIL();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual int Action(int,int,int,int) override;
    R_DOT* p1; // +0x78
    R_DOT* p2; // +0x7C
    R_DOT* Getp1() { return p1; }
    R_DOT* Getp2() { return p2; }
    void UnBreak(R_DOT* dot);
};

class BUILDING : public UNIT {
public:
    BUILDING(VID*,float,float,float,ANGLE,SPRITE*);
    ~BUILDING();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual int Action(int,int,int,int) override;
    virtual void MoveTact() override;
    void MasterRepair();
    VID* buildUnit;              // +0x90
    int buildUnitX;              // +0x94
    int buildUnitY;              // +0x98
    int cur_unit_need_repair;    // +0x9C
    SPRITE* lastreparedunit;     // +0xA0
};

class CREATURE : public UNIT {
public:
    CREATURE(VID*,float,float,float,ANGLE,SPRITE*);
    ~CREATURE();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual int Action(int,int,int,int) override;
    virtual void MoveTact() override;
    virtual void DeletePointerToSprite(SPRITE*) override;
    unsigned int stateJustCreated; // +0x90, bit0 used by retail
    REGION* in_region;             // +0x94
    REGION* FindRegion(float x,float y);
};

class CIV_ROBOT : public CREATURE {
public:
    CIV_ROBOT(VID*,float,float,float,ANGLE,SPRITE*);
    ~CIV_ROBOT();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual int Action(int,int,int,int) override;
    virtual void MoveTact() override;
    virtual void DeletePointerToSprite(SPRITE*) override;
    unsigned int behave_state; // +0x98; CodeView unnamed 4-byte state
    PTR_SPRITE near_train;     // +0x9C
    int near_explosion;        // +0xA0
    int change_state_flag;     // +0xA4
    int IsRobotBuilding(const SPRITE* building);
    SPRITE* FindRobotBuilding();
    void PathIsBlocked();
    void ChangeAnimation(int act);
    void RotateHead(ANGLE direct);
};

class PLANE : public UNIT {
public:
    PLANE(VID*,float,float,float,ANGLE,SPRITE*);
    ~PLANE();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    // ZS1 SPRITE adds virtual Rotate at +0x20; PLANE-specific slots therefore
    // begin at +0x24 and occupy +0x24..+0x40.
    virtual void PlaneNextCommand(int var1,int var2);
    virtual void ZSpeedInitialization();
    virtual void CheckFlightProperties();
    virtual int WayBlocked();
    virtual void FlightIfWayBlocked();
    virtual void FlightToTarget();
    virtual void FlightToTargetAdditionalActions();
    virtual void FreeFlight();
};

class BALLOON : public PLANE {
public:
    BALLOON(VID*,float,float,float,ANGLE,SPRITE*);
    ~BALLOON();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual void MoveTact() override;
    virtual void ZSpeedInitialization() override;
    virtual void CheckFlightProperties() override;
    virtual void FlightToTargetAdditionalActions() override;
    int must_taran;                 // +0x90
    unsigned char zspeed_state;     // +0x94
    unsigned char pad95[3];
    int SetCommand(int command,SPRITE* new_goal);
    void MoveToNearestBase();
    void ConnectToBase();
    int IsBalloonMoveFinished();
    int IsItFreeBase(const SPRITE* p);
    int IsItBase(const SPRITE* p);
};

class MAN : public UNIT {
public:
    int weaponAmmo[10];
    MAN(VID*,float,float,float,ANGLE,SPRITE*);
    ~MAN();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    virtual void MoveTact();
    int ChangeWeapon(int weapon);
};

class CANNON : public SPRITE {
public:
    unsigned int stateMissileStart;
    CANNON(VID*,float,float,float,ANGLE,SPRITE*);
    ~CANNON();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    virtual void MoveTact();
    virtual void DeletePointerToSprite(SPRITE*);
};

class LINKER : public SPRITE {
public:
    LINKER(VID*,float,float,float,ANGLE,SPRITE*);
    ~LINKER();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    float linkX;                // +0x70
    float linkY;                // +0x74
    float linkZ;                // +0x78
    ANGLE beginDirection;       // +0x7C
    uint8_t pad07D[3];
    SPRITE* parent;             // +0x80; LINKER-specific owner, distinct from SPRITE::m_parent
    void LinkRotate(ANGLE newDirection);
    SPRITE* Parent();
};

