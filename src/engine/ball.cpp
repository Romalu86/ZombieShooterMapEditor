#include "mapedit/runtime.hpp"
#include "../zs1/zScriptExec_engine.h"

namespace {

// Convert a radian angle to the engine's unsigned 8-bit ANGLE domain.
static int BallFloorToInt(double value)
{
    int result=static_cast<int>(value);
    if (static_cast<double>(result)>value)
        --result;
    return result;
}

static float BallAbs(float value)
{
    return value<0.0f ? -value : value;
}

static uint8_t BallDirectionFromRadians(float radians)
{
    const float pi=3.1415927410125732421875f;
    const double scaled=static_cast<double>(radians/pi*128.0f+0.5f);
    int value=BallFloorToInt(scaled);

    if (value>=0x100)
        value-=((value>>8)<<8);
    if (value<0)
        value+=(((0xFF-value)>>8)<<8);
    return static_cast<uint8_t>(value);
}

// Retail quantizer used by BALL when it rebounds from the current flagman.
// g_angleSecond is clamped in-place because the original helper mutates the
// zScript-owned global before doing the stepped angle search.
static float BallQuantizedBounceRadians(float deltaX,float span)
{
    using namespace zs1::script_engine;

    const float requested=g_angleRadians*deltaX/span;
    if (g_angleSecond<2.0f)
        g_angleSecond=2.0f;

    const float outputStep=g_angleRadians/(g_angleSecond-1.0f);
    const float thresholdStep=g_angleRadians/g_angleSecond;
    float output=0.0f;
    float threshold=0.0f;
    const int count=static_cast<int>(g_angleSecond);

    for (int i=0;i<count;++i) {
        threshold+=thresholdStep;
        if (threshold>=BallAbs(requested))
            return requested>=0.0f ? output : -output;
        output+=outputStep;
    }
    return output;
}

static float BallFlagmanSpan(const BALL* ball)
{
    SPRITE* const flagman=Map->Flagman(Map->m_curArmy);
    if (!flagman || !flagman->m_child)
        return 0.0f;
    return flagman->m_child->Vid()->m_snapOffsetX+ball->Vid()->m_snapOffsetX;
}

static uint8_t BallAngleDifference(uint8_t a,uint8_t b)
{
    const uint8_t ab=static_cast<uint8_t>(a-b);
    const uint8_t ba=static_cast<uint8_t>(b-a);
    return ab<ba ? ab : ba;
}

static int BallIsOneShotVid(int nvid)
{
    return nvid==0x1FA || nvid==0x1FB || nvid==0x1FC;
}

static uint8_t BallNudgeDirection(uint8_t direction)
{
    if (direction<0x7D)
        ++direction;
    else if (direction>0x82)
        --direction;
    return direction;
}

static int BallSpecialUnderObject(const BALL* ball,const SPRITE* blocker)
{
    if (blocker->Vid()->m_idx!=0x5F8 || ball->Vid()->m_fireDamage>=30)
        return 0;

    const ANGLE toBlocker=Decart2Polar(blocker->X()-ball->X(),
                                       blocker->Y()-ball->Y());
    return BallAngleDifference(toBlocker.value,blocker->m_dir)<0x46;
}

// The normal impact branch uses the opposite boolean convention from the
// earlier "ball under object" branch.  Preserve that retail distinction.
static int BallUseAmplifiedImpactDamage(const BALL* ball,const SPRITE* blocker)
{
    if (blocker->Vid()->m_idx!=0x5F8)
        return 0;
    if (ball->Vid()->m_fireDamage>=30)
        return 1;

    const ANGLE toBlocker=Decart2Polar(blocker->X()-ball->X(),
                                       blocker->Y()-ball->Y());
    return BallAngleDifference(toBlocker.value,blocker->m_dir)>=0x46;
}

static uint8_t BallLowByteOfFloat(float value)
{
    union FLOAT_BITS {
        float f;
        unsigned int u;
    } bits;
    bits.f=value;
    return static_cast<uint8_t>(bits.u);
}

}

BALL::BALL(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,x,y,z,direction,parent),
      state(0),
      lastX(m_x),
      lastY(m_y),
      underTime(0),
      impactCount(0)
{
}

BALL::~BALL()
{
}

void* BALL::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->BALL::~BALL();
    if (flags&1u)
        operator delete(self);
    return self;
}

// Retail BALL::MoveTact, including the original cumulative four-step collision
// sweep and its otherwise surprising low-byte fallback direction.
void BALL::MoveTact()
{
    using namespace zs1::script_engine;

    const float kLeftLimit=7.0f;
    const float kLeftReturn=17.0f;
    const float kRightLimit=778.0f;
    const float kRightReturn=768.0f;
    const float kTopLimit=10.0f;
    const float kProbeRadius=12.0f;

    if (m_x<=kLeftLimit) {
        ChangeCoor(kLeftReturn,m_y,m_z);
        ChangeDirection(ANGLE(static_cast<uint8_t>(0x40)));
    }
    if (m_x>=kRightLimit) {
        ChangeCoor(kRightReturn,m_y,m_z);
        ChangeDirection(ANGLE(static_cast<uint8_t>(0xC0)));
    }

    if (m_y-m_z-Map->m_shiftY<=kTopLimit) {
        const uint8_t oldDirection=m_dir;
        uint8_t newDirection=static_cast<uint8_t>(-oldDirection);
        ANGLE probe(newDirection);
        if (m_y-probe.CosY()*kProbeRadius-m_z-Map->m_shiftY<=kTopLimit)
            newDirection=static_cast<uint8_t>(0x80-oldDirection);
        newDirection=BallNudgeDirection(newDirection);

        CreateChildAndPlaySFX(11,0);
        ChangeCoor(m_x,m_y+1.0f,m_z);
        ChangeDirection(ANGLE(newDirection));

        const int nvid=Vid()->m_idx;
        if (nvid==0x5F3) {
            Action(0x6E,0,0,0);
            ChangeAnimation(15);
        }
        else if (BallIsOneShotVid(nvid)) {
            g_oneShotValue=reinterpret_cast<int>(this);
        }
    }

    float newX;
    float newY;
    float newZ;
    MoveTactCalcCoor(&newX,&newY,&newZ);

    // Target checks the "under object" path only when X changed.  Y-only
    // movement deliberately skips this block.
    if (m_x!=newX) {
        SPRITE* const under=CanPlace(m_x,m_y,m_z);
        if (under) {
            SPRITE* const flagman=Map->Flagman(Map->m_curArmy);
            if (!flagman || under->m_parent!=flagman) {
                Error(10,"ball under object",static_cast<unsigned long>(m_dir));
                underTime=CurrentTime;

                const int special=BallSpecialUnderObject(this,under);
                if (under!=Mouse && under->Vid()->m_baseHp!=0 && !special)
                    under->Action(0x55,under->Vid()->m_baseHp,
                                  reinterpret_cast<int>(this),0);

                ChangeCoor(newX,newY,newZ);
                goto post_move;
            }
        }
    }

    if (m_x==newX && m_y==newY)
        goto post_move;

    if (static_cast<unsigned long>(CurrentTime-underTime)<1000u)
        Error(10,"after under",static_cast<unsigned long>(m_dir));

    AskLine(&newX,&newY,&newZ);

    {
        const float dx=newX-m_x;
        const float dy=newY-m_y;
        const float dz=newZ-m_z;
        float candidateX;
        float candidateY;
        float candidateZ;
        SPRITE* blocker;

        candidateX=m_x+dx*0.25f;
        candidateY=m_y+dy*0.25f;
        candidateZ=m_z+dz*0.25f;
        blocker=CanPlaceWithCrush(candidateX,candidateY,candidateZ);
        if (!blocker) {
            ChangeCoor(candidateX,candidateY,candidateZ);

            candidateX=m_x+dx*0.5f;
            candidateY=m_y+dy*0.5f;
            candidateZ=m_z+dz*0.5f;
            blocker=CanPlaceWithCrush(candidateX,candidateY,candidateZ);
            if (!blocker) {
                ChangeCoor(candidateX,candidateY,candidateZ);

                candidateX=m_x+dx*0.75f;
                candidateY=m_y+dy*0.75f;
                candidateZ=m_z+dz*0.75f;
                blocker=CanPlaceWithCrush(candidateX,candidateY,candidateZ);
                if (!blocker) {
                    ChangeCoor(candidateX,candidateY,candidateZ);

                    candidateX=m_x+dx;
                    candidateY=m_y+dy;
                    candidateZ=m_z+dz;
                    blocker=CanPlaceWithCrush(candidateX,candidateY,candidateZ);
                    if (!blocker) {
                        ChangeCoor(candidateX,candidateY,candidateZ);
                        state=0;
                        goto post_move;
                    }
                }
            }
        }

        // At this point blocker is the first failed cumulative sweep sample.
        // The second CanPlace result is used only as a boolean in retail; the
        // parent comparison still uses the sweep blocker.
        if (CanPlace(m_x,m_y,m_z)) {
            SPRITE* const flagman=Map->Flagman(Map->m_curArmy);
            if (flagman && blocker->m_parent==flagman) {
                ChangeCoor(m_x+dx,
                           blocker->m_y-blocker->Vid()->m_footprintHeight*0.5f-
                           Vid()->m_footprintHeight*0.5f-1.0f,
                           m_z+dz);
                state=0;
                goto post_move;
            }
        }

        if (static_cast<unsigned long>(CurrentTime-underTime)<1000u)
            Error(10,"block after under",static_cast<unsigned long>(m_dir));

        SPRITE* const flagman=Map->Flagman(Map->m_curArmy);
        if (flagman && (blocker==flagman || blocker->m_parent==flagman)) {
            state=0;
            if (g_flag49C4AC) {
                blocker->ChangeAnimation(7);
            }
            else {
                const int eventFunction=Vid()->m_eventFunction[7];
                if (eventFunction>=0)
                    Map->ScriptRun(eventFunction,this,0,0);
            }

            const float deltaX=candidateX-blocker->m_x;
            uint8_t newDirection=BallDirectionFromRadians(
                BallQuantizedBounceRadians(deltaX,BallFlagmanSpan(this)));

            if (newDirection<1) {
                newDirection=1;
                CreateChildAndPlaySFX(9,0);
            }
            else if (newDirection>0xFE) {
                newDirection=0xFE;
                CreateChildAndPlaySFX(9,0);
            }
            else if (newDirection>0x37 && newDirection<=0x80) {
                newDirection=0x37;
                CreateChildAndPlaySFX(9,0);
            }
            else {
                if (newDirection>0x80 && newDirection<0xC8)
                    newDirection=0xC8;
                CreateChildAndPlaySFX(9,0);
            }

            newDirection=BallNudgeDirection(newDirection);
            ChangeDirection(ANGLE(newDirection));
            goto post_move;
        }

        uint8_t newDirection;
        if (m_x==lastX && m_y==lastY) {
            Error(10,"same coor",static_cast<unsigned long>(m_dir));
            if (state==2)
                newDirection=static_cast<uint8_t>(m_dir+0x22);
            else
                newDirection=BallLowByteOfFloat(dy);
            state=2;
            newDirection=BallNudgeDirection(newDirection);
            ChangeDirection(ANGLE(newDirection));
            goto post_move;
        }

        lastX=m_x;
        lastY=m_y;
        newDirection=static_cast<uint8_t>(-m_dir);
        {
            ANGLE probe(newDirection);
            if (CanPlace(m_x+probe.Sin()*kProbeRadius,
                         m_y-probe.CosY()*kProbeRadius,
                         m_z))
                newDirection=static_cast<uint8_t>(0x80-m_dir);
        }

        const int amplifiedDamage=BallUseAmplifiedImpactDamage(this,blocker);
        if (blocker!=Mouse && blocker->Vid()->m_baseHp!=0) {
            int damage=Vid()->m_fireDamage;
            if (amplifiedDamage)
                damage*=100;
            blocker->Action(0x55,damage,reinterpret_cast<int>(this),0);
        }

        if ((blocker!=Mouse || impactCount>=3) && Vid()->m_idx==0x5F3) {
            Action(0x6E,0,0,0);
            ChangeAnimation(15);
        }

        if (blocker==Mouse) {
            CreateChildAndPlaySFX(11,0);
            ++impactCount;
        }
        else {
            CreateChildAndPlaySFX(12,0);
            if (blocker->Vid()->m_noAnimCadr[7]!=0 &&
                (blocker->m_ani==0 || blocker->m_ani==2)) {
                blocker->ChangeAnimation(12);
            }
            else {
                blocker->CreateChildAndPlaySFX(12,0);
            }
        }

        if (BallIsOneShotVid(Vid()->m_idx))
            g_oneShotValue=reinterpret_cast<int>(this);

        state=1;
        newDirection=BallNudgeDirection(newDirection);
        ChangeDirection(ANGLE(newDirection));
    }

post_move:
    {
        const int nvid=Vid()->m_idx;
        const float screenY=m_y-m_z-Map->m_shiftY;
        const float bottom=Graph->ViewYMax()-25.0f;

        if (g_flag5F0AC8 && nvid!=0x5CA && nvid!=0x5F3 && screenY>=bottom) {
            g_flag5F0AC8=0;
            MYERROR::Log(::Error,"was direction = %i",static_cast<int>(m_dir));
            ChangeCoor(m_x,m_y-1.0f,m_z);
            ChangeDirection(ANGLE(static_cast<uint8_t>(0x80-m_dir)));
            MYERROR::Log(::Error,"new direction = %i",static_cast<int>(m_dir));
            return;
        }

        if (screenY<bottom || nvid==0x5CA || m_dir<0x40 || m_dir>0xC0)
            return;

        MYERROR::Log(::Error,"ball death direction = %i",static_cast<int>(m_dir));
        ChangeCoor(m_x,Graph->ViewYMax()-5.0f+Map->m_shiftY,m_z);
        Stop();
        ChangeAnimation(15);
    }
}
