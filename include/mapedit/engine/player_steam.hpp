#pragma once
// PLAYER_STEAM owner. Included in ABI order by mapedit/runtime.hpp.
class PLAYER_STEAM : public PLAYER {
public:
    PLAYER_STEAM(int type,int army);
    virtual ~PLAYER_STEAM();
    virtual void DeletePointerToSprite(SPRITE* sprite) override;
    virtual void Release() override;
    virtual void SetFlagman(SPRITE* unit) override;
    virtual void Control(INPUT* input) override;
    virtual void StateBarOn() override;
    virtual void StateBarOff() override;
    virtual void PutMessage(const STRING* text,float x,float y) override;
    virtual void AddUnitToStateBar(SPRITE* unit) override;

    int ChangeIcon(int nIcon,SPRITE* sprite);
    int ChangeIcon(int nIcon,VID* vid);
    int ChangeBuildIcon(int nIcon,VID* vid);
    void ChangeStateBar(int newMode);
    void DrawStateBar(const INPUT* input);
    void SetTrainForKey(UNIT* engine,int n);
    void DebugEngineDraw();
    int IsEnoughMoney(int nvid);
    int IsTargetEngine();
    void Control_KeyProcessing(INPUT* input);
    void Control_MenuProcessing(INPUT* input,int* button);
    void Control_RClickProcessing(INPUT* input);
    int CanCapture(const SPRITE* sprite);
    int CanSelect(const SPRITE* sprite);
    int CanCreateUnit(VID* nvid);
    int OptCleverEnemyAttack();
    void SetCleverEnemyAttack(int newAttack);

    unsigned int m_options;       // +0x28; bit0 optCanSelectEnemy, bit1 clever attack
    int m_statebarMode;           // +0x2C
    int m_statebarDepoUnit;       // +0x30
    SPRITE_LIST m_flagmanPathDots;// +0x34
    MESSAGE m_message;            // +0x44
    int m_kill[4];                // +0x348, executable-proven extent
    UNIT* m_trains[10];           // +0x358, ctor loop is exactly 10
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

