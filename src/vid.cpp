#include "mapedit/runtime.hpp"

// Retail global dynamic initializer allocates the permanent fallback VID before
// WinMain and stores it in EmptyVid (global 0x004EE688).  The pre-A12
// reconstruction incorrectly left EmptyVid zero-initialized, so MAP::MAP crashed
// immediately after LoadVid at source line 186 when it dereferenced EmptyVid.
VID* EmptyVid = new VID;

namespace {
inline unsigned char* VidRaw(VID* vid)
{
    return reinterpret_cast<unsigned char*>(vid);
}

// VC6 helper at 0x0049909C converts the x87 value to a signed 64-bit
// integer with truncation toward zero; callers in the VID loaders consume
// the low 32 bits (and, for tile extents, only AX).  Keep that observable
// behavior without embedding compiler-specific assembly in the recovered
// source.  x87 FISTP returns the integer-indefinite value on NaN/overflow,
// whose low DWORD is zero.
inline int RetailFtolLow32(float value)
{
    const double d=static_cast<double>(value);
    if (!(d>=-9223372036854775808.0 && d<9223372036854775808.0))
        return 0;
    const __int64 converted=static_cast<__int64>(d);
    return static_cast<int>(static_cast<unsigned int>(converted));
}

}

// ZS1 target 0x00425560..0x004256A1.
// The retail object is genuinely polymorphic.  MSVC supplies the vfptr at +0;
// every explicit data write below is taken from the original constructor.
VID::VID()
{
    // STRING/GAMMA members are now represented by their genuine C++ types.
    // Their automatic member construction reproduces the retail ctor calls at
    // +0x008, +0x2E0, +0x2F4 and the GAMMA[4] vector at +0x3F0.

    m_spriteClass=6;
    m_unknown0C=0;
    m_flag=0;
    m_footprintWidth=24.0f;
    m_footprintHeight=16.0f;
    m_hitVerticalOffset=20.0f;
    m_snapOffsetX=m_footprintWidth/2.0f;
    m_snapOffsetY=m_footprintHeight/2.0f;

    m_colorScaleR=1.0f;
    m_colorScaleG=1.0f;
    m_colorScaleB=1.0f;

    m_mirrorNext=this;
    m_extraTypeFlags=0;
    m_exchangeVid=this;
    m_layer=19;
    m_idx=-1;
    m_baseHp=0;
    m_noDirections=1;
    m_editorDirectionOffset=0;
    m_phaseRandomInterval=71;
    m_linkVidIndex=0;
    m_linkVid=0;
    m_weaponIndex=0;
    // Retail VID::VID does not write +0x464 here.  ResetSprites(), called at
    // the end of this constructor, owns the initial m_lastEntityTime reset.
    m_weapon=0;
    m_nLinkDots=0;
    m_linkDots=0;
    m_dotFrameStarts=0;
    m_exSpriteData=0;
    m_moveTactData=0;
    m_prop=0;

    // Retail clears the seven one-bit properties individually, preserving the
    // rest of the bitfield storage exactly as VC6 does for member bitfields.
    m_propertyBits&=~0x7Fu;

    for (int ani=0;ani<17;++ani) {
        m_aniSfx[ani]=0;
        m_aniSpawnMode[ani]=0;
        m_aniChildVid[ani]=0;
        m_noAnimCadr[ani]=0;
        m_aniFrameStart[ani]=0;
        m_aniFrameLimit[ani]=0;
        m_aniFrameSpeed[ani]=71;
    }

    ResetSprites();
}

// Compiler-generated scalar deleting destructor: direct call to 0x00425700 and
// conditional operator delete on flags&1.
// ZS1 target 0x00425700..0x004257C7.
VID::~VID()
{
    // Retail sums the four counters once and reuses that exact value in Error.
    const int noSprites=m_entitiesNumber[0]+m_entitiesNumber[1]+
                        m_entitiesNumber[2]+m_entitiesNumber[3];
    if (noSprites)
        Error(10,const_cast<char*>("Not all sprites with this VID deleted"),static_cast<unsigned long>(noSprites));

    if (m_mirrorNext!=this) {
        VID* previous=m_mirrorNext;
        while (previous->m_mirrorNext!=this)
            previous=previous->m_mirrorNext;
        previous->m_mirrorNext=m_mirrorNext;
    }

    if (m_linkDots)
        ::operator delete(m_linkDots);
    m_linkDots=0;
    if (m_dotFrameStarts)
        ::operator delete(m_dotFrameStarts);
    m_dotFrameStarts=0;

    // m_resourceName and m_name are destroyed automatically in retail order.
}

// Serialized VID parameter block.  The portable engine is used only for local
// names; field order, conversions, editor branch and BuildSizeToGridZ dot-grid
// are taken from the MapEdit executable above.
void VID::LoadParameters(RESOURCE* res)
{
    unsigned char* const raw=VidRaw(this);
    res->Read(raw+0x0C,4u); res->Read(raw+0x10,4u); res->Read(raw+0x14,4u); res->Read(raw+0x18,4u);
    res->Read(raw+0x1C,4u); res->Read(raw+0x20,4u); res->Read(raw+0x24,4u); res->Read(raw+0x28,4u);
    res->Read(raw+0x2C,4u); res->Read(raw+0x30,4u); res->Read(raw+0x34,4u); res->Read(raw+0x38,4u);
    res->Read(raw+0x3C,4u); res->Read(raw+0x40,4u); res->Read(raw+0x44,4u); res->Read(raw+0x48,4u);
    res->Read(raw+0x4C,4u); res->Read(raw+0x50,4u); res->Read(raw+0x54,4u); res->Read(raw+0x58,4u);
    res->Read(raw+0x5C,4u); res->Read(raw+0x60,4u);
    // +0x64 is the resolved m_linkVid pointer and is not serialized.
    res->Read(raw+0x68,4u); res->Read(raw+0x6C,4u); res->Read(raw+0x70,4u); res->Read(raw+0x74,4u);

    // ZS1 target 0x425C2F skips four legacy serialized dwords.
    res->Shift(16);
    res->Read(&m_noDirections,4u);
    res->Read(m_noAnimCadr,0x44u);
    res->Read(m_aniSfx,0x44u);
    res->Read(m_aniFrameSpeed,0x44u);
    res->Read(m_aniSpawnX,0x44u);
    res->Read(m_aniSpawnY,0x44u);
    res->Read(m_aniSpawnZ,0x44u);
    res->Read(m_aniSpawnMode,0x44u);
    res->Read(m_aniFireCount,0x44u);

    int red=0,green=0,blue=0,alpha=0;
    res->Read(&red,4u);
    res->Read(&green,4u);
    res->Read(&blue,4u);
    res->Read(&alpha,4u);
    GAMMA loadedGamma(alpha,red,green,blue);
    m_gamma.operator=(&loadedGamma);
    for (int army=0;army<4;++army)
        m_gammaByArmy[army]=&m_gamma;

    res->Read(&m_colorScaleR,4u);
    res->Read(&m_colorScaleG,4u);
    res->Read(&m_colorScaleB,4u);
    if (IsZBufferType() && IsHardwareType()) {
        m_colorScaleR=1.0f;
        m_colorScaleG=1.0f;
        m_colorScaleB=1.0f;
    }

    // ZS1 0x425Dxx converts the serialized movement fields in-place.
    // The four speed-like values are milliseconds -> seconds, acceleration /
    // deceleration are per-microsecond values, and +0x44 uses the historical
    // reciprocal direction-lock representation.
    const float sentinel=999999.0f;
    if (m_childDirectionLock==sentinel)
        m_childDirectionLock=0.0f;
    else if (m_childDirectionLock==0.0f)
        m_childDirectionLock=sentinel;
    else
        m_childDirectionLock=256.0f/m_childDirectionLock;

    if (m_defaultMaxSpeed!=sentinel) m_defaultMaxSpeed/=1000.0f;
    if (m_moveSpeedMirror!=sentinel) m_moveSpeedMirror/=1000.0f;
    if (m_maxZSpeed!=sentinel) m_maxZSpeed/=1000.0f;
    if (m_moveSpeed38!=sentinel) m_moveSpeed38/=1000.0f;
    if (m_acceleration!=sentinel) m_acceleration/=1000000.0f;
    if (m_deceleration!=sentinel) m_deceleration/=1000000.0f;

    // Target derives +0x480 from the four movement values or flag bits 1/2/12.
    m_moveTactData=(m_defaultMaxSpeed!=0.0f || m_moveSpeedMirror!=0.0f ||
                    m_maxZSpeed!=0.0f || m_moveSpeed38!=0.0f ||
                    (m_flag&0x1006u)!=0u) ? 1 : 0;

    if (!m_noDirections) {
        Error(4,const_cast<char*>("NoDir==0"),0);
        exit(1);
    }
    for (int i=0;i<17;++i) {
        if (!m_aniFrameSpeed[i])
            m_aniFrameSpeed[i]=static_cast<unsigned int>(m_phaseRandomInterval);
    }

    m_editorDirectionOffset=128/static_cast<int>(m_noDirections);
    m_snapOffsetX=m_footprintWidth/2.0f;
    m_snapOffsetY=m_footprintHeight/2.0f;

    if (!m_dotFrameCount) {
        Error(4,const_cast<char*>("noCadr==0"),0);
    } else if (m_dotFrameCount<static_cast<int>(m_noDirections)) {
        Error(4,const_cast<char*>("noCadr < noDir"),0);
        m_noDirections=static_cast<unsigned int>(m_dotFrameCount);
    }

    int total=0;
    for (int i=0;i<17;++i)
        total+=m_noAnimCadr[i]*static_cast<int>(m_noDirections);
    if (total>m_dotFrameCount) {
        Error(13,const_cast<char*>("noCadr for noAnimCadr and noDir"),0);
        for (int i=16;i>=0;--i) {
            const int block=m_noAnimCadr[i]*static_cast<int>(m_noDirections);
            if (total-block<=m_dotFrameCount) {
                m_noAnimCadr[i]-=(total-m_dotFrameCount)/static_cast<int>(m_noDirections);
                break;
            }
            total-=block;
            m_noAnimCadr[i]=0;
        }
    }

    int start=0;
    int firstAnimation=-1;
    for (int i=0;i<17;++i) {
        if (!Sound->ValidateSFX(m_aniSfx[i]) && m_idx!=-1) {
            Error(4,const_cast<char*>("sfx"),static_cast<unsigned long>(m_aniSfx[i]));
            m_aniSfx[i]=0;
        }

        if (m_noAnimCadr[i]) {
            m_aniFrameStart[i]=start;
            m_aniFrameLimit[i]=m_noAnimCadr[i];
            if (firstAnimation<0) {
                firstAnimation=i;
                for (int j=0;j<i;++j) {
                    if (!m_aniFrameLimit[j])
                        m_aniFrameLimit[j]=m_aniFrameLimit[i];
                }
            }
        } else if (m_spriteClass==10 && (i&1) && i<=7 &&
                   m_noAnimCadr[firstAnimation+1]) {
            m_aniFrameStart[i]=m_aniFrameStart[firstAnimation+1];
            m_aniFrameLimit[i]=m_aniFrameLimit[firstAnimation+1];
        } else {
            m_aniFrameStart[i]=0;
            m_aniFrameLimit[i]=m_aniFrameLimit[firstAnimation];
        }

        start+=m_noAnimCadr[i]*static_cast<int>(m_noDirections);
        if (start>m_dotFrameCount) {
            Error(10,const_cast<char*>("noCadr and noAnimCadr and noDir"),static_cast<unsigned long>(i));
            m_aniFrameStart[i]=0;
            m_aniFrameLimit[i]=m_aniFrameLimit[firstAnimation];
        }
    }

    if (m_spriteClass==8)
        m_propertyBits|=0x20u;
    if (m_spriteClass==8 && Map->IsMapEdit())
        m_spriteClass=0;

    if (!PropBuildSizeToGridZ() || m_nLinkDots)
        return;

    // Retail allocation uses width in both factors. Preserve that historical
    // expression rather than silently correcting it to width*height.
    const int roundedWidth=(static_cast<int>(m_footprintWidth)+17)/8;
    const int tempCount=((static_cast<int>(m_footprintWidth)+17)*roundedWidth)/8+1;
    VID_DOT* temp=static_cast<VID_DOT*>(::operator new(static_cast<unsigned int>(tempCount*12)));
    m_nLinkDots=0;

    for (int yi=0;static_cast<float>(yi)<m_footprintHeight;yi+=8) {
        for (int xi=0;static_cast<float>(xi)<m_footprintWidth;xi+=8) {
            temp[m_nLinkDots].x=static_cast<float>(xi)-m_footprintWidth/2.0f;
            temp[m_nLinkDots].y=static_cast<float>(yi)-m_footprintHeight/2.0f;
            temp[m_nLinkDots].z=m_hitVerticalOffset;
            ++m_nLinkDots;
        }
        temp[m_nLinkDots].x=m_footprintWidth/2.0f;
        temp[m_nLinkDots].y=static_cast<float>(yi)-m_footprintHeight/2.0f;
        temp[m_nLinkDots].z=m_hitVerticalOffset;
        ++m_nLinkDots;
    }
    for (int xi=0;static_cast<float>(xi)<m_footprintWidth;xi+=8) {
        temp[m_nLinkDots].x=static_cast<float>(xi)-m_footprintWidth/2.0f;
        temp[m_nLinkDots].y=m_footprintHeight/2.0f;
        temp[m_nLinkDots].z=m_hitVerticalOffset;
        ++m_nLinkDots;
    }
    temp[m_nLinkDots].x=m_footprintWidth/2.0f;
    temp[m_nLinkDots].y=m_footprintHeight/2.0f;
    temp[m_nLinkDots].z=m_hitVerticalOffset;
    ++m_nLinkDots;

    if (m_linkDots)
        ::operator delete(m_linkDots);
    m_linkDots=static_cast<VID_DOT*>(::operator new(static_cast<unsigned int>(m_nLinkDots*12)));
    for (int i=0;i<m_nLinkDots;++i)
        m_linkDots[i]=temp[i];
    ::operator delete(temp);
}


// ZS1 target 0x004256B0..0x004256CA.
VID* VID::CreateMirror()
{
    return new VID();
}

void VID::DrawVidToVid(const SPRITE*)
{
}

void VID::Draw(const SPRITE*)
{
}

void VID::DrawShadow(const SPRITE*)
{
}

void VID::DrawToVid(const SPRITE*,const VID_TEXCOOR*,TEXTURE*,TEXTURE*)
{
}

void VID::Load(RESOURCE*)
{
}

// Base implementation is an exact no-op; concrete ZS1 VID families may override it.
void VID::SetScriptPackedValue125(int)
{
}

int VID::HaveShadow()
{
    return 0;
}

// ZS1 target 0x004256D0..0x004256DA; base virtual layer reset.
void VID::SetLayer()
{
    m_layer=0;
}

void VID::Error(int type,char* text,unsigned long err)
{
    MYERROR::Error(::Error,"VID [%i-%s]",type,text,err,m_idx,m_name.m_buf);
}

void VID::SetGamma(const GAMMA* gamma,unsigned int n_gamma)
{
    if (n_gamma<4u) {
        m_gammaByArmy[n_gamma]=gamma;
    } else if (n_gamma!=4u) {
        Error(4,const_cast<char*>("n_gamma in VID::SetGamma"),n_gamma);
    }
}







// ZS1 retail name: VID::IsLight; editor name retained for source ABI continuity.
int VID::IsLightType()
{
    // ZS1 retail 0x00449C10 tests the main VID flag at +0x14, not the
    // two-byte HEAD type word stored at +0x2F8.
    return m_flag&0x00000080u;
}


















// ZS1 retail name: VID::HaveWeapon; editor-facing name retained.
int VID::CanFight() const
{
    // ZS1 VID::HaveWeapon (0x00449BA0): child pointer slot 8 (+0x278)
    // and weaponState48 (+0x48) must both be non-zero.
    return m_aniChildVid[8]!=0 && m_weaponIndex!=0;
}

// ZS1 retail 0x00470D70..0x00470D9C: prefer linked VID only when child[8] and weapon state exist.
int VID::GetMaxAmmo()
{
    const VID* source=(m_linkVid && m_linkVid->m_aniChildVid[8] && m_linkVid->m_weaponIndex)
        ? m_linkVid : this;
    // Retail dereferences the selected weapon directly; do not add a reconstruction-only guard.
    return source->m_weapon->m_maxAmmo;
}

// Current MapEdit lineage: 0x00430013..0x004300E2.
int VID::GetFireDamage()
{
    const int linkedDamage=m_linkVid ? m_linkVid->GetFireDamage() : 0;
    const int fire15=m_aniChildVid[15] ? m_aniChildVid[15]->GetFireDamage()*m_aniFireCount[15] : 0;
    const int fire14=m_aniChildVid[14] ? m_aniChildVid[14]->GetFireDamage()*m_aniFireCount[14] : 0;
    const int fire8=m_aniChildVid[8] ? m_aniChildVid[8]->GetFireDamage()*m_aniFireCount[8] : 0;
    return m_fireDamage+fire8+fire14+fire15+linkedDamage;
}

// Zombie Shooter 1 retail VID::GetBuildTime.
int VID::GetBuildTime()
{
    const VID* source=(m_linkVid && m_linkVid->m_weaponIndex) ? m_linkVid : this;
    // ZS1 retail VID::GetBuildTime 0x004266D0 reads runtime WEAPON+0x34.
    // Keep the raw offset here until the whole WEAPON prefix has been given
    // proven ZS1 field names; PORT13's AS1-derived m_buildTime member is +0x24.
    // Retail dereferences the selected weapon directly, exactly like GetMaxAmmo.
    return *reinterpret_cast<const int*>(
        reinterpret_cast<const unsigned char*>(source->m_weapon)+0x34u) / 1000;
}


// Zombie Shooter 1 retail VID::PropHide.

// Current MapEdit lineage: 0x0043012F..0x00430177.
void VID::SetPropHide(int flag)
{
    const unsigned int hideBit=flag ? 0x40u : 0u;
    VID* current=this;
    do {
        current->m_propertyBits=(current->m_propertyBits & ~0x40u) | hideBit;
        current=current->m_linkVid;
    } while (current);
}



// at 0x004BD304 is "%04i %s".
STRING VID::GetNumberName()
{
    char buffer[1024];
    sprintf(buffer, "%04i %s", m_idx, m_name.m_buf);
    return STRING(buffer);
}

namespace {
int g_vidViewXMinRetail=0;
int g_vidViewXMaxRetail=0;
int g_vidViewYMinRetail=0;
int g_vidViewYMaxRetail=0;
}

void VID::SetViewPort(int x0,int y0,int x1,int y1)
{
    g_vidViewXMinRetail=x0;
    g_vidViewXMaxRetail=x1;
    g_vidViewYMinRetail=y0;
    g_vidViewYMaxRetail=y1;
}

// MapEditZS1.exe 0x00426500..0x0042659B.
void VID::SetGridZ(const SPRITE* sprite)
{
    if (sprite==Mouse)
        return;

    SPRITE* const current=const_cast<SPRITE*>(sprite);

    // Retail records the exact frame whose dot range is being materialized.
    // ResetGridZ must later clear this saved range, not the sprite's then-current
    // animation frame.
    current->m_exData->gridFrame=current->m_noCadr;

    int begin=0;
    int end=m_nLinkDots;
    if (m_dotFrameStarts) {
        const int frame=current->m_noCadr;
        begin=(frame<m_dotFrameCount) ? m_dotFrameStarts[frame] : 0;
        end=(frame<m_dotFrameCount-1) ? m_dotFrameStarts[frame+1] : m_nLinkDots;
    }

    for (int i=begin;i<end;++i) {
        Map->SetTempGroundZ(
            current->m_x+m_linkDots[i].x,
            current->m_y+m_linkDots[i].y,
            current->m_z+m_linkDots[i].z);
    }
}

// MapEditZS1.exe 0x004265A0..0x00426639.
void VID::ResetGridZ(const SPRITE* sprite)
{
    if (sprite==Mouse)
        return;

    SPRITE* const current=const_cast<SPRITE*>(sprite);
    const int frame=current->m_exData->gridFrame;
    if (frame<0)
        return;

    int begin=0;
    int end=m_nLinkDots;
    if (m_dotFrameStarts) {
        begin=(frame<m_dotFrameCount) ? m_dotFrameStarts[frame] : 0;
        end=(frame<m_dotFrameCount-1) ? m_dotFrameStarts[frame+1] : m_nLinkDots;
    }

    for (int i=begin;i<end;++i) {
        Map->ClearTempGroundZ(
            current->m_x+m_linkDots[i].x,
            current->m_y+m_linkDots[i].y,
            current->m_z+m_linkDots[i].z);
    }
}


// MapEditZS1.exe 0x00451680..0x004516C7. One-direction VIDs preserve the
// incoming ANGLE verbatim; only multi-direction VIDs are quantized.
ANGLE VID::SteppedDirection(ANGLE direction)
{
    if (m_noDirections>1)
        return ANGLE(static_cast<uint8_t>((RealDirection(direction)<<8)/static_cast<int>(m_noDirections)));
    return direction;
}

// ZS1 retail name: VID::GetEntitiesNumber(int); editor-facing name retained.
// Retail owner is callable and must remain out-of-line: 0x00449BC0..0x00449BCD.
int VID::NoSprites(int army)
{
    return m_entitiesNumber[army];
}

// ZS1 target 0x004257D0..0x0042587E.
void VID::ResetSprites()
{
    for (int i=0;i<21;++i)
        m_eventFunction[i]=-1;

    for (int i=0;i<4;++i) {
        m_entitiesNumber[i]=0;
        m_killed[i]=0;
        m_reColored[i]=0;
        m_maxHp[i]=m_baseHp;
        m_limit398[i]=-1;
    }
    m_limit394=-1;
    m_lastEntityTime=0;
    m_propertyBits&=~0x10u;
}











// ZS1 retail getter for NotCreateAsChild.
int VID::PropNotCreateAsChild()
{
    return static_cast<int>(m_prop);
}





int WEAPON::PropInTurn() const
{
    return static_cast<int>(m_property & 0x00000010u);
}

int WEAPON::PropSelfDirecting() const
{
    return static_cast<int>(m_property & 0x00000020u);
}

// Current MapEdit lineage: 0x00430370..0x004304B8.
// Exact high-level arithmetic recovered from retail ASM.  VC6/FPU lowering
// is intentionally left to the retail-compiler parity lane.
float VID::CalculateZSpeed(float delta_z,float size) const
{
    float speed;
    if (PropGravity()) {
        speed=size*Const->gravity/m_defaultMaxSpeed*0.5f +
              delta_z*m_defaultMaxSpeed/size;
        speed*=speed>0.0f ? 1.1f : 0.9f;
    }
    else if (PropGravity2()) {
        speed=size*Const->gravity2/m_defaultMaxSpeed*0.5f +
              delta_z*m_defaultMaxSpeed/size;
        speed*=speed>0.0f ? 1.1f : 0.9f;
    }
    else if (PropSelfMoving()) {
        speed=m_maxZSpeed;
    }
    else if (m_groundOffset==0.0f) {
        speed=delta_z*m_defaultMaxSpeed/size;
    }
    else {
        speed=0.0f;
    }

    if (speed>m_maxZSpeed)
        return m_maxZSpeed;
    if (speed<-m_maxZSpeed)
        return -m_maxZSpeed;
    return speed;
}

int WEAPON::PropFrontEye() const { return static_cast<int>(m_property & 0x2u); }
int WEAPON::PropRandomTarget() const { return static_cast<int>(m_property & 0x8u); }
int WEAPON::PropAttackAnyArmy() const { return static_cast<int>(m_property & 0x80u); }
int WEAPON::PropAttackNearOnly() const { return static_cast<int>(m_property & 0x100u); }
int WEAPON::PropAnyDirFire() const { return static_cast<int>(m_property & 0x1u); }

int VID::IsInvulnerable()
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(this)+0x28)==0;
}

int VID::PropInvisibleForEnemy()
{
    return static_cast<int>(m_flag & 0x00020000u);
}

int VID::PropRadialDamage()
{
    return static_cast<int>(m_flag & 0x04000000u);
}

int VID::PropNotDamageForFriend()
{
    return static_cast<int>(m_flag & 0x80000000u);
}

int VID::PropRandSpeed()
{
    return static_cast<int>(m_flag & 0x400u);
}

int VID::PropRandZSpeed()
{
    return static_cast<int>(m_flag & 0x00400000u);
}

int VID::PropBounce()
{
    return static_cast<int>(m_flag & 0x10000000u);
}

// ---------------------------------------------------------------------------
// VID_HARDWARE — original MapEdit DX7 hardware-VID owner.
// Layout and all bodies in this block are recovered directly from MapEdit.exe
// + NB11/CodeView.  Load/Draw remain in the next renderer-body block; do not
// replace them with base fallbacks or a fabricated vtable.
// ---------------------------------------------------------------------------

namespace {
inline VID*& VidHardwareSharedOwner(VID* vid)
{
    // Retail VID+0x46C is the circular/shared renderer ownership link used by
    // VID_HARDWARE mirrors.  VID's own destructor also walks this field.
    return *reinterpret_cast<VID**>(reinterpret_cast<unsigned char*>(vid)+0x46C);
}

}

void WordSet(void* dest,int cword,int noword)
{
    unsigned short* out=static_cast<unsigned short*>(dest);
    const unsigned short word=static_cast<unsigned short>(cword);
    while (noword-- > 0)
        *out++=word;
}

VID_HARDWARE::VID_HARDWARE()
    : VID(), m_texCoor(0), m_noSurf(0), m_textures(0)
{
}

// This is intentionally pointer-taking: CodeView and the CreateMirror caller
// both prove that the original signature is VID_HARDWARE(VID_HARDWARE*).
VID_HARDWARE::VID_HARDWARE(VID_HARDWARE* source)
    : VID()
{
    VidHardwareSharedOwner(this)=VidHardwareSharedOwner(source);
    VidHardwareSharedOwner(source)=this;
    m_layer=source->m_layer;
    m_extraTypeFlags=source->m_extraTypeFlags;
    m_dotFrameCount=source->m_dotFrameCount;
    m_phaseRandomInterval=source->m_phaseRandomInterval;
    m_regionTileStepX=source->m_regionTileStepX;
    m_regionTileStepY=source->m_regionTileStepY;
    m_texCoor=source->m_texCoor;
    m_textures=source->m_textures;
    m_noSurf=source->m_noSurf;
}

VID_HARDWARE::VID_HARDWARE(int nvid,int size_x,int size_y)
    : VID()
{
    m_footprintWidth=256.0f;
    m_footprintHeight=256.0f;
    m_hitVerticalOffset=1.0f;
    m_regionTileStepX=static_cast<short>(size_x);
    m_regionTileStepY=static_cast<short>(size_y);
    m_noDirections=1;
    m_idx=nvid;
    m_layer=0;
    m_name=STRING("Self Created Hardware Prerendered Ground ");

    // Retail first writes 0x25 then calls VID::SetExtraType() (OR 0x0200).
    m_extraTypeFlags=0x25u;
    SetExtraType();

    const int count=(size_x/256+1)*(size_y/256+1)+1;
    m_texCoor=static_cast<VID_TEXCOOR*>(::operator new(static_cast<unsigned int>(count*0x24)));
    if (!m_texCoor) {
        Error(2,const_cast<char*>("texcoor"),static_cast<unsigned long>(count));
        exit(1);
    }

    m_noSurf=0;
    int index=0;
    for (int y=0;y<size_y;y+=256) {
        for (int x=0;x<size_x;x+=256) {
            if (index)
                m_texCoor[index-1].next_fragment=index;

            VID_TEXCOOR& part=m_texCoor[index];
            part.nsurf=static_cast<int>(m_noSurf);
            part.begx=0;
            part.begy=0;
            part.shiftx=x;
            part.shifty=y;
            const int remainX=size_x-x;
            const int remainY=size_y-y;
            part.sizex=(remainX>256)?256:remainX;
            part.sizey=(remainY>256)?256:remainY;
            part.next_fragment=0;
            m_noSurf=static_cast<short>(m_noSurf+2);
            ++index;
        }
    }

    m_textures=static_cast<TEXTURE**>(::operator new(static_cast<unsigned int>(m_noSurf)*sizeof(TEXTURE*)));
    if (!m_textures) {
        Error(2,const_cast<char*>("textures"),static_cast<unsigned long>(m_noSurf));
        return;
    }
    for (int i=0;i<m_noSurf;i+=2) {
        m_textures[i]=0;
        m_textures[i+1]=0;
    }
}

VID_HARDWARE::~VID_HARDWARE()
{
    if (VidHardwareSharedOwner(this)==this) {
        if (m_texCoor)
            ::operator delete(m_texCoor);
        m_texCoor=0;

        if (m_textures) {
            while (--m_noSurf>=0) {
                TEXTURE* texture=m_textures[m_noSurf];
                if (texture)
                    delete texture;
            }
            ::operator delete(m_textures);
            m_textures=0;
        }
        m_noSurf=0;
    }
}

VID* VID_HARDWARE::CreateMirror()
{
    return new VID_HARDWARE(this);
}

// Current MapEdit lineage: 0x00437235..0x004375F5.
void VID_HARDWARE::DrawVidToVid(const SPRITE* sprite)
{
    if (!IsTextureType() || !IsZBufferType() || m_noDirections!=1)
        return;
    if (sprite->Vid()->PropInvisibleForEnemy())
        return;

    const int savedXMin=g_vidViewXMinRetail;
    const int savedXMax=g_vidViewXMaxRetail;
    const int savedYMin=g_vidViewYMinRetail;
    const int savedYMax=g_vidViewYMaxRetail;

    const int spriteX=static_cast<int>(sprite->X());
    const int spriteY=static_cast<int>(sprite->Y()-sprite->Z());

    VID_TEXCOOR* part=m_texCoor;
    while (part) {
        const int dx=abs(part->shiftx+part->begx+part->sizex/2-spriteX);
        const int spriteHalfX=static_cast<int>(sprite->Vid()->m_regionTileStepX)/2;
        if (dx < spriteHalfX + part->sizex/2) {
            const int dy=abs(part->shifty+part->begy+part->sizey/2-spriteY);
            const int spriteHalfY=static_cast<int>(sprite->Vid()->m_regionTileStepY)/2;
            if (dy < spriteHalfY + part->sizey/2) {
                g_vidViewXMinRetail=part->begx;
                g_vidViewXMaxRetail=part->begx+part->sizex;
                g_vidViewYMinRetail=part->begy;
                g_vidViewYMaxRetail=part->begy+part->sizey;

                const int surface=part->nsurf;
                if (!m_textures[surface]) {
                    m_textures[surface]=new TEXTURE(part->sizex,part->sizey,0x17,0);
                    int pitch=0;
                    unsigned char* bits=m_textures[surface]->Lock(&pitch,0);
                    memset(bits,0,static_cast<unsigned int>(pitch*m_textures[surface]->SizeY()));
                    m_textures[surface]->UnLock();

                    m_textures[surface+1]=new TEXTURE(part->sizex,part->sizey,0x50,2);
                    unsigned short* z= reinterpret_cast<unsigned short*>(m_textures[surface+1]->Lock(&pitch,0));
                    WordSet(z,0x400,(pitch/2)*m_textures[surface+1]->SizeY());
                    m_textures[surface+1]->UnLock();
                }

                sprite->Vid()->DrawToVid(sprite,part,m_textures[surface],m_textures[surface+1]);
            }
        }

        part=part->next_fragment ? m_texCoor+part->next_fragment : 0;
    }

    g_vidViewXMinRetail=savedXMin;
    g_vidViewXMaxRetail=savedXMax;
    g_vidViewYMinRetail=savedYMin;
    g_vidViewYMaxRetail=savedYMax;
}

// ZS1 retail MapEditZS1.exe 0x0042D4D0..0x0042DF87.
// Hardware draw owner recovered against the ZS1 retail DX7 path.
void VID_HARDWARE::Draw(const SPRITE* sprite)
{
    if (m_noSurf==0)
        return;

    const int frame=const_cast<SPRITE*>(sprite)->CurrentCadr();
    if (m_texCoor[frame].sizey==0 || PropHide())
        return;

    int x=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenX());
    int y=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenY());
    int z=static_cast<int>(sprite->Z());
    int noStep=1;

    if (PropZeroZ())
        z=3;

    if (PropWave()) {
        const ANGLE phase(static_cast<uint8_t>((CurrentTime>>3)&0xFFu));
        const float shiftZ=m_groundOffset*const_cast<ANGLE&>(phase).Sin();
        z+=static_cast<int>(shiftZ);
        y-=static_cast<int>(shiftZ);
    }

    // ZS1 retail VID_HARDWARE::Draw 0x0042D59F reads the two draw-scale
    // floats from VID+0x2E8/+0x2EC.  PORT13 used +0x2E0/+0x2E4, which
    // are the two DWORDs of GAMMA and therefore collapsed/distorted many
    // hardware VIDs (default gamma is normally all zeroes).
    float scaleX=m_colorScaleR;
    float scaleY=m_colorScaleG;

    if (m_propertyBits&2u) {
        const float coeff=const_cast<SPRITE*>(sprite)->ExData()->tableCoeff;
        unsigned char* weapon=reinterpret_cast<unsigned char*>(m_weapon);
        // ZS1 retail 0x0042D5BE..0x0042D65B: scale tables +0x100/+0x120.
        scaleX*=m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x100),coeff);
        scaleY*=m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x120),coeff);
    }

    if (m_propertyBits&4u) {
        const float coeff=const_cast<SPRITE*>(sprite)->ExData()->tableCoeff;
        unsigned char* weapon=reinterpret_cast<unsigned char*>(m_weapon);
        // ZS1 retail 0x0042D666..0x0042D75C: positional tables
        // +0x160/+0x180/+0x1A0.
        x+=static_cast<int>(m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x160),coeff));
        y+=static_cast<int>(m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x180),coeff));
        z+=static_cast<int>(m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x1A0),coeff));
    }

    ANGLE dir(static_cast<uint8_t>(0));
    if (PropHardwareDirect()) {
        if (PropVertDir()) {
            EX_SPRITE_DATA* ex=const_cast<SPRITE*>(sprite)->ExData();
            if (sprite->X()!=ex->lastX || sprite->Y()!=ex->lastY) {
                ANGLE actual(sprite->X()-ex->lastX,
                    (sprite->Y()-sprite->Z()-ex->lastY+ex->lastZ)/0.7070602178573608f);
                dir=&actual;
            } else if (sprite->Speed()!=0.0f && sprite->ZSpeed()!=0.0f) {
                ANGLE facing=const_cast<SPRITE*>(sprite)->Direction();
                const float vx=sprite->Speed()*facing.Sin();
                const float vy=-(sprite->ZSpeed()+sprite->Speed()*facing.Cos())/0.7070602178573608f;
                ANGLE actual(vx,vy);
                dir=&actual;
            } else {
                ANGLE facing=const_cast<SPRITE*>(sprite)->Direction();
                const int delta=facing.Int()-const_cast<SPRITE*>(sprite)->RealDirection()*256/static_cast<int>(m_noDirections);
                ANGLE actual(static_cast<uint8_t>(delta));
                dir=&actual;
            }
        } else {
            ANGLE facing=const_cast<SPRITE*>(sprite)->Direction();
            const int delta=facing.Int()-const_cast<SPRITE*>(sprite)->RealDirection()*256/static_cast<int>(m_noDirections);
            ANGLE actual(static_cast<uint8_t>(delta));
            dir=&actual;
        }
    }

    if (PropBlur() && m_texCoor[frame].sizex!=0 && m_texCoor[frame].sizey!=0) {
        EX_SPRITE_DATA* ex=const_cast<SPRITE*>(sprite)->ExData();
        const float dxFloat=ex->lastX-sprite->X();
        const float dyFloat=ex->lastY-ex->lastZ-sprite->Y()+sprite->Z();

        if (PropHardwareDirect() && PropVertDir()) {
            // ZS1 retail 0x0042D8EE..0x0042D943 uses a different sample-count
            // metric for vertically-directed hardware sprites.  It blends the
            // major/minor displacement (major*0.5 + minor), multiplies by
            // -1.5, divides by the frame height, rounds, then subtracts from
            // one.  PORT13 used the ordinary axis quotient for every blur VID.
            float dx=dxFloat<0.0f ? -dxFloat : dxFloat;
            float dy=dyFloat<0.0f ? -dyFloat : dyFloat;
            const float weighted=(dx>=dy) ? (dx*0.5f+dy) : (dy*0.5f+dx);
            noStep=1-static_cast<int>(weighted*(-1.5f)/
                                      static_cast<float>(m_texCoor[frame].sizey));
        } else {
            int dx=static_cast<int>(dxFloat);
            if (dx<0) dx=-dx;
            int dy=static_cast<int>(dyFloat);
            if (dy<0) dy=-dy;
            const int noStepX=dx/m_texCoor[frame].sizex;
            const int noStepY=dy/m_texCoor[frame].sizey;
            noStep=(noStepX>noStepY ? noStepX*2 : noStepY*2)+1;
        }
    }

    // ZS1 retail 0x0042D988..0x0042D9B5 explicitly enables alpha test
    // and programs ALPHAREF/ALPHAFUNC before drawing hardware fragments.
    // Leaving these inherited from a previous draw call makes transparent
    // hardware VID pixels depend on render order.
    Graph->SetRenderState(0x0Fu,1u);
    Graph->SetRenderState(0x18u,0u);
    Graph->SetRenderState(0x19u,5u);

    struct RetailVertex {
        float x,y,z,rhw;
        unsigned long diffuse;
        unsigned long specular;
        float tu,tv;
    };

    for (int step=0;step<noStep;++step) {
        if (PropBlur()) {
            EX_SPRITE_DATA* ex=const_cast<SPRITE*>(sprite)->ExData();
            x=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenX()+
                (ex->lastX-sprite->X())*static_cast<float>(step)/static_cast<float>(noStep));
            y=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenY()+
                (ex->lastY-ex->lastZ-sprite->Y()+sprite->Z())*static_cast<float>(step)/static_cast<float>(noStep));
            z=static_cast<int>(sprite->Z()+
                (ex->lastZ-sprite->Z())*static_cast<float>(step)/static_cast<float>(noStep));
        }

        VID_TEXCOOR* coor=&m_texCoor[frame];
        while (coor) {
            const int halfStepX=static_cast<int>(m_regionTileStepX)/2;
            const int halfStepY=static_cast<int>(m_regionTileStepY)/2;
            int shiftX=x+static_cast<int>(static_cast<float>(coor->shiftx-halfStepX)*scaleX);
            int sizeX=coor->sizex;
            int shiftY=y+static_cast<int>(static_cast<float>(coor->shifty-halfStepY)*scaleY);
            int sizeY=coor->sizey;

            // ZS1 retail 0x0042DAB8..0x0042DB13 performs viewport rejection
            // with the *scaled* fragment extents.  PORT13 tested raw sizes,
            // causing scaled hardware VIDs to disappear too early or remain
            // visible outside the viewport.  The later Z-buffer clip itself
            // deliberately remains in raw texel units, matching retail.
            const int visibleRight=shiftX+static_cast<int>(static_cast<float>(sizeX)*scaleX);
            const int visibleBottom=shiftY+static_cast<int>(static_cast<float>(sizeY)*scaleY);
            if (visibleRight<g_vidViewXMinRetail || shiftX>=g_vidViewXMaxRetail ||
                visibleBottom<g_vidViewYMinRetail || shiftY>=g_vidViewYMaxRetail) {
                coor=coor->next_fragment ? &m_texCoor[coor->next_fragment] : 0;
                continue;
            }

            if (IsZBufferType()) {
                if (shiftX+sizeX>g_vidViewXMaxRetail)
                    sizeX=g_vidViewXMaxRetail-shiftX;
                if (shiftY+sizeY>g_vidViewYMaxRetail)
                    sizeY=g_vidViewYMaxRetail-shiftY;
            }

            RECT_OLD screenRect;
            screenRect.left=shiftX;
            screenRect.top=shiftY;
            screenRect.right=shiftX+static_cast<int>(static_cast<float>(sizeX)*scaleX);
            screenRect.bottom=shiftY+static_cast<int>(static_cast<float>(sizeY)*scaleY);

            RECT_OLD textureRect;
            textureRect.left=coor->begx;
            textureRect.top=coor->begy;
            textureRect.right=coor->begx+sizeX;
            textureRect.bottom=coor->begy+sizeY;

            if (IsZBufferType()) {
                const long err=Graph->CopyToZBuffer(&screenRect,&textureRect,m_textures[coor->nsurf+1]);
                if (err)
                    Error(1,const_cast<char*>("zbuffer"),static_cast<unsigned long>(err));
            }

            float z1=static_cast<float>(z)*0.0001220703125f+0.015625f;
            if (PropWave() && m_groundOffset!=0.0f) {
                ANGLE phase(static_cast<uint8_t>((CurrentTime>>3)&0xFFu));
                z1+=m_groundOffset*0.0001220703125f*phase.Sin()/2.0f;
            }

            float z2;
            if (PropAlwaysTop() || IsZBufferType()) {
                z2=z1=0.99999988f;
                Graph->SetRenderState(0x17u,8u);
            } else if (m_hitVerticalOffset>m_footprintHeight) {
                const float d=static_cast<float>(sizeY)*0.0001220703125f;
                z2=z1-d;
                z1+=d;
                Graph->SetRenderState(0x17u,7u);
            } else {
                z2=z1;
                Graph->SetRenderState(0x17u,7u);
            }
            (void)z2;

            if (IsAlphaType()) {
                if (IsTextureType()) Graph->SetAlphaBlend(5u,6u);
                else Graph->SetAlphaBlend(9u,2u);
            } else {
                Graph->SetRenderState(0x1Bu,0u);
            }

            GAMMA spriteGamma=const_cast<SPRITE*>(sprite)->GetGamma();
            GAMMA* baseGamma=reinterpret_cast<GAMMA*>(reinterpret_cast<unsigned char*>(this)+0x2E0);
            GAMMA drawGamma=(*baseGamma)+&spriteGamma;
            if (!PropGamma()) {
                GAMMA graphGamma=Graph->GetGamma();
                drawGamma+=&graphGamma;
            }

            const float w=static_cast<float>(coor->sizex);
            const float h=static_cast<float>(coor->sizey);
            const float textureSizeX=static_cast<float>(m_textures[coor->nsurf]->SizeX());
            const float textureSizeY=static_cast<float>(m_textures[coor->nsurf]->SizeY());
            const float xx[4]={0.0f,w,0.0f,w};
            const float yy[4]={0.0f,0.0f,h,h};
            RetailVertex vertices[4];

            for (int i=0;i<4;++i) {
                vertices[i].x=static_cast<float>(coor->shiftx-halfStepX)+xx[i];
                vertices[i].y=static_cast<float>(coor->shifty-halfStepY)+yy[i];
                vertices[i].z=0.0f;
                vertices[i].tu=(static_cast<float>(coor->begx)+xx[i]+0.5f)/textureSizeX;
                vertices[i].tv=(static_cast<float>(coor->begy)+yy[i]+0.5f)/textureSizeY;
            }

            for (int i=0;i<4;++i) {
                vertices[i].x*=scaleX;
                vertices[i].y*=scaleY;
                if (PropHardwareDirect()) {
                    const float oldX=vertices[i].x;
                    const float oldY=vertices[i].y;
                    vertices[i].x=dir.RotateX(oldX,oldY);
                    vertices[i].y=dir.RotateY(oldX,oldY);
                }
                vertices[i].x+=static_cast<float>(x);
                vertices[i].y+=static_cast<float>(y);
                vertices[i].z=z1;
                vertices[i].rhw=1.0f;
                vertices[i].diffuse=drawGamma.Diffuse();
                vertices[i].specular=drawGamma.Specular();
            }

            m_textures[coor->nsurf]->SetTexture(0);
            Graph->SetRenderState(0x1Du,drawGamma.Specular()!=0 ? 1u : 0u);
            Graph->DrawPrimitive(5u,0x1C4u,vertices,0x20u,4);

            coor=coor->next_fragment ? &m_texCoor[coor->next_fragment] : 0;
        }
    }
}

void VID_HARDWARE::SetLayer()
{
    // ZS1 retail 0x0042D3C0..0x0042D4C8.  The AS1-derived PORT13 map
    // collapsed three ZS1-only ordering routes (layers 18/17/15).  This is
    // observable for editor-only/top sprites and is one source of VIDs being
    // hidden behind the wrong layer.
    if (PropGround())
        m_layer=4;
    else if (m_unknown0C==0x40u)
        m_layer=10;
    else if (!m_noSurf)
        m_layer=19;
    else if (IsZBufferType() && IsAlphaType())
        m_layer=9;
    else if (IsZBufferType())
        m_layer=0;
    else if (PropAlwaysTop() && m_noDirections==0xFFu)
        m_layer=18;
    else if (PropAlwaysTop() && m_unknown0C==0x10u)
        m_layer=17;
    else if (PropAlwaysTop())
        m_layer=14;
    else if (m_unknown0C==0x10u)
        m_layer=15;
    else if (IsAlphaType())
        m_layer=PropWave()?9:12;
    else
        m_layer=8;
}



// ZS1 retail MapEditZS1.exe 0x0042C8B0..0x0042D3BB (Exec\\vid.obj).
// Hardware SURF/DATA reader recovered against the MapEdit body.  The newer
// engine-family implementation was used only as a naming aid; format gates,
// old/new DATA layout, error routing and resource shifts below follow MapEdit.
void VID_HARDWARE::Load(RESOURCE* res)
{
    QS1_CODER* colorCoder=0;
    QS1_CODER* zCoder=0;

    if (res->GoNext(0x46525553u)) // 'SURF'
        Error(5,const_cast<char*>("SURF"),0);

    res->Read(&m_noSurf,2u);
    if (m_noSurf==0)
        return;

    m_textures=static_cast<TEXTURE**>(operator new(static_cast<unsigned int>(m_noSurf)*sizeof(TEXTURE*)));
    if (!m_textures) {
        Error(2,const_cast<char*>("textures"),static_cast<unsigned long>(m_noSurf));
        return;
    }
    for (int i=0;i<m_noSurf;++i)
        m_textures[i]=0;

    unsigned char* const unpack=static_cast<unsigned char*>(operator new(0x40008u));
    if (!unpack) {
        Error(2,const_cast<char*>("(unpack)"),0);
        return;
    }

    if (IsCompressType()) {
        colorCoder=new QS1_CODER(IsDXTType() ? 1 : 2);
        zCoder=new QS1_CODER(2);
    }

    unsigned char palette[768];
    int i=0;
    while (i<m_noSurf) {
        Graph->DrawLoadBar(Map->Vid(0));

        short width=0;
        short height=0;
        res->Read(&width,2u);
        res->Read(&height,2u);

        if (IsDXTType()) {
            const int format=(IsAlphaType() && IsTextureType()) ? 0x33545844 : 0x31545844; // DXT3 / DXT1
            m_textures[i]=new TEXTURE(width,height,format,0);
        } else if (IsPaletteType()) {
            m_textures[i]=new TEXTURE(width,height,41,0); // P8
        } else if ((m_extraTypeFlags&0x2000u)!=0u) {
            // ZS1-only hardware 32-bit surface route, retail 0x0042CB47.
            m_textures[i]=new TEXTURE(width,height,21,0); // A8R8G8B8
        } else if (IsAlphaType() && IsTextureType()) {
            m_textures[i]=new TEXTURE(width,height,26,0); // A4R4G4B4
        } else {
            // Retail asks for A1R5G5B5 here.  TEXTURE may internally fall back
            // to R5G6B5 on hardware which cannot provide the requested format;
            // selecting R5G6B5 here (PORT13) skips the alpha-bit conversion.
            m_textures[i]=new TEXTURE(width,height,25,0); // A1R5G5B5
        }

        if (!m_textures[i] || !m_textures[i]->IsExist()) {
            Error(3,const_cast<char*>("texture"),0);
            delete colorCoder;
            delete zCoder;
            operator delete(unpack);
            return;
        }

        if (IsPaletteType()) {
            Error(10,const_cast<char*>("palette %i"),static_cast<unsigned long>(m_textures[i]->IsPaletted()));
            res->Read(palette,768u);
            if (m_textures[i]->IsPaletted()) {
                COLOR colors[256];
                for (int p=0;p<256;++p)
                    colors[p]=COLOR(palette[p*3],palette[p*3+1],palette[p*3+2]);
                m_textures[i]->SetPalette(colors);
            }
        }

        int packedSize=0;
        res->Read(&packedSize,4u);
        const int decodeError=res->ReadPacked(unpack,static_cast<unsigned int>(packedSize),colorCoder);
        if (decodeError)
            Error(5,const_cast<char*>("Can't decode"),static_cast<unsigned long>(i));

        int pitch=0;
        if (IsPaletteType() || packedSize>=2*static_cast<int>(width)*static_cast<int>(height)) {
            unsigned char* dst=m_textures[i]->Lock(&pitch,0);
            if (!dst) {
                Error(0,const_cast<char*>("texture surface"),0);
                delete colorCoder;
                delete zCoder;
                operator delete(unpack);
                return;
            }

            for (int y=0;y<height;++y) {
                unsigned short* const row=reinterpret_cast<unsigned short*>(dst + pitch*y);
                if (IsPaletteType()) {
                    if (!m_textures[i]->IsPaletted()) {
                        for (int x=0;x<width;++x) {
                            const unsigned int pi=unpack[x+y*width];
                            RGB16 pixel(palette[pi*3],palette[pi*3+1],palette[pi*3+2]);
                            row[x]=pixel.color;
                        }
                    } else {
                        memcpy(row,unpack+y*width,static_cast<unsigned int>(width));
                    }
                } else {
                    const int format=m_textures[i]->Format();
                    if (format==23 || format==26) {
                        memcpy(row,reinterpret_cast<const unsigned short*>(unpack)+y*width,
                               static_cast<unsigned int>(2*width));
                    } else if (format==21) {
                        // A8R8G8B8 hardware-direct stream: four bytes/pixel.
                        // Treating this as WORD data was one of PORT13's
                        // concrete corrupt-VID paths.
                        memcpy(reinterpret_cast<unsigned char*>(row),
                               unpack+static_cast<unsigned int>(y)*static_cast<unsigned int>(width)*4u,
                               static_cast<unsigned int>(4*width));
                    } else {
                        // Requested A1R5G5B5 (or the equivalent fallback):
                        // retail converts non-zero RGB565 source pixels to
                        // RGB555 and sets the one-bit alpha flag. Zero remains
                        // transparent zero.
                        const unsigned short* const src=
                            reinterpret_cast<const unsigned short*>(unpack)+y*width;
                        for (int x=0;x<width;++x) {
                            const unsigned short value=src[x];
                            row[x]=value ? static_cast<unsigned short>(
                                0x8000u | (value&0x001Fu) | ((value>>1)&0x7FE0u)) : 0u;
                        }
                    }
                }
            }
            m_textures[i]->UnLock();
        } else {
            Error(10,const_cast<char*>("Load DXT"),0);
            unsigned char* dst=m_textures[i]->Lock(&pitch,0);
            if (dst) {
                memcpy(dst,unpack,static_cast<unsigned int>(packedSize));
                m_textures[i]->UnLock();
            } else {
                Error(0,const_cast<char*>("DXT texture surface"),0);
            }
        }

        if (IsZBufferType()) {
            ++i;
            m_textures[i]=new TEXTURE(width,height,80,2); // D16
            res->Read(&packedSize,4u);

            if (!m_textures[i] || !m_textures[i]->IsExist()) {
                Error(3,const_cast<char*>("texture z surface"),0);
                res->Shift(packedSize);
                ++i;
                continue;
            }

            unsigned char* const zdst=m_textures[i]->Lock(&pitch,0);
            if (!zdst) {
                Error(0,const_cast<char*>("texture z surface"),0);
                res->Shift(packedSize);
                ++i;
                continue;
            }

            if (packedSize==pitch*static_cast<int>(height)) {
                const int zDecodeError=res->ReadPacked(zdst,static_cast<unsigned int>(packedSize),zCoder);
                if (zDecodeError)
                    Error(5,const_cast<char*>("Can't decode z"),static_cast<unsigned long>(packedSize-zDecodeError));
            } else {
                Error(5,const_cast<char*>("ZBuffer: invalid size"),static_cast<unsigned long>(packedSize));
            }
            m_textures[i]->UnLock();
        }
        ++i;
    }

    if (res->GoNext(0x41544144u)) // 'DATA'
        Error(5,const_cast<char*>("DATA"),0);

    if (IsNewVersionType()) {
        res->SubLoad(reinterpret_cast<void**>(&m_texCoor),0);
        if (!m_texCoor)
            Error(5,const_cast<char*>("tex_coor"),0);
    } else {
        const int count=res->SubSize()/20;
        m_texCoor=static_cast<VID_TEXCOOR*>(operator new(static_cast<unsigned int>(count)*sizeof(VID_TEXCOOR)));
        for (int c=0;c<count;++c)
            m_texCoor[c].Read(res,0);
    }

    res->GoNext(0x44414853u); // 'SHAD'; return value intentionally ignored
    delete colorCoder;
    delete zCoder;
    operator delete(unpack);
}

void VID_TEXCOOR::Read(STREAM* res,int new_version)
{
    if (new_version) {
        res->Read(this,sizeof(*this));
        return;
    }

    res->Read(&shadow_shift,4u);
    short value=0;
    res->Read(&value,2u); nsurf=value;
    res->Read(&value,2u); begx=value;
    res->Read(&value,2u); begy=value;
    res->Read(&value,2u); sizex=value;
    res->Read(&value,2u); sizey=value;
    res->Read(&value,2u); shiftx=value;
    res->Read(&value,2u); shifty=value;
    res->Read(&value,2u); next_fragment=value;
}

void VID::SetChildAndLink()
{
    if (m_linkVidIndex) {
        const int linkIndex=m_linkVidIndex;
        if (linkIndex>=0 && linkIndex<Map->m_noVid && Map->VidSlot(linkIndex))
            m_linkVid=Map->VidSlot(linkIndex)->m_exchangeVid;
        else
            Error(4,const_cast<char*>("LinkVid"),static_cast<unsigned long>(linkIndex));
    }

    // Retail then walks the serialized LinkVid-index chain without mutating it.
    // The result is intentionally unused: this read-only traversal (and its
    // cycle behaviour) is present in target 0x00425907..0x00425930.
    int chainedLinkIndex=m_linkVidIndex;
    while (chainedLinkIndex>=0 &&
           chainedLinkIndex<Map->m_noVid &&
           Map->VidSlot(chainedLinkIndex) &&
           chainedLinkIndex!=0) {
        chainedLinkIndex=Map->VidSlot(chainedLinkIndex)->m_linkVidIndex;
    }

    // Retail unconditionally dereferences VID+0x468 here.  The previous
    // reconstruction-only null guard changed the owner CFG and hid the retail
    // invariant that this converted WEAP/property block must already exist.
    const unsigned char* const ex=reinterpret_cast<const unsigned char*>(m_weapon);
    int i;
    for (i=0;i<8;++i) {
        const unsigned int step=static_cast<unsigned int>(i)*4u;
        const uint32_t p80=*reinterpret_cast<const uint32_t*>(ex+0x080+step);
        const uint32_t pA0=*reinterpret_cast<const uint32_t*>(ex+0x0A0+step);
        const uint32_t pC0=*reinterpret_cast<const uint32_t*>(ex+0x0C0+step);
        const uint32_t pE0=*reinterpret_cast<const uint32_t*>(ex+0x0E0+step);
        if (p80 || pA0 || pC0 || pE0)
            m_propertyBits|=0x1u;

        const float p100=*reinterpret_cast<const float*>(ex+0x100+step);
        const float p120=*reinterpret_cast<const float*>(ex+0x120+step);
        const float p140=*reinterpret_cast<const float*>(ex+0x140+step);
        if (p100!=1.0f || p120!=1.0f || p140!=1.0f)
            m_propertyBits|=0x2u;

        const float p160=*reinterpret_cast<const float*>(ex+0x160+step);
        const float p180=*reinterpret_cast<const float*>(ex+0x180+step);
        const float p1A0=*reinterpret_cast<const float*>(ex+0x1A0+step);
        if (p160!=0.0f || p180!=0.0f || p1A0!=0.0f)
            m_propertyBits|=0x4u;

        if (ex[0x1C0+i] || ex[0x1C8+i] || ex[0x1D0+i])
            m_propertyBits|=0x8u;
    }

    if (m_defaultDeathTimer!=999999u ||
        (m_flag&0x00200000u)!=0u ||
        m_defaultMaxSpeed!=m_moveSpeedMirror ||
        (m_propertyBits&0x0Fu)!=0u ||
        (m_flag&0x00080000u)!=0u ||
        (m_flag&0x28u)!=0u)
        m_exSpriteData=1;

    for (i=0;i<17;++i) {
        const int child=m_aniSpawnMode[i];
        if (!child)
            continue;
        // Retail computes abs(child) in-owner with CDQ/XOR/SUB and then
        // validates the slot directly against MAP::m_noVid/m_vids[].
        const int childIndex=child<0
            ? static_cast<int>(0u-static_cast<unsigned int>(child))
            : child;
        if (childIndex>=0 && childIndex<Map->m_noVid && Map->VidSlot(childIndex)) {
            VID* mirror=Map->VidSlot(childIndex)->m_exchangeVid;
            m_aniChildVid[i]=mirror;
            if (mirror && (mirror->m_flag&(0x80u|0x00080000u)))
                m_exSpriteData=1;
        } else {
            Error(4,const_cast<char*>("child"),static_cast<unsigned long>(child));
        }
    }
}

// ---- MAP::CreateVid prerequisite family: ABI-proven short virtuals ----

// ZS1 retail VID_LIGHT layer = 11.
void VID_LIGHT::SetLayer()
{
    m_layer=11;
}

// ZS1 retail VID_SOFTWARE mirror copy constructor.  The generated mirror
// intentionally does not inherit link-dot/frame-start ownership from source.
VID_SOFTWARE::VID_SOFTWARE(VID_SOFTWARE* source)
    : VID()
{
    m_mirrorNext=source->m_mirrorNext;
    source->m_mirrorNext=this;
    m_layer=source->m_layer;
    m_extraTypeFlags=source->m_extraTypeFlags;
    m_dotFrameCount=source->m_dotFrameCount;
    m_phaseRandomInterval=source->m_phaseRandomInterval;
    m_regionTileStepX=source->m_regionTileStepX;
    m_regionTileStepY=source->m_regionTileStepY;
    m_cadrShift=source->m_cadrShift;
    m_cadrs=source->m_cadrs;
    m_cadrSize=source->m_cadrSize;

    // ZS1 0x00426F84..0x00426F98 explicitly clears these three base fields.
    // Mirror VIDs share the frame/cadr payload but do not clone link-dot tables.
    m_nLinkDots=0;
    m_linkDots=0;
    m_dotFrameStarts=0;
}

// ZS1 retail VID_SOFTWARE default constructor.
VID_SOFTWARE::VID_SOFTWARE()
    : VID(), m_cadrShift(0), m_cadrSize(0), m_cadrs(0)
{
}

// ZS1 retail VID_SOFTWARE destructor.
VID_SOFTWARE::~VID_SOFTWARE()
{
    if (m_mirrorNext==this) {
        if (m_cadrs)
            ::operator delete(m_cadrs);
        m_cadrs=0;
        if (m_cadrShift)
            ::operator delete(m_cadrShift);
        m_cadrShift=0;
        g_vidMemoryInUse-=m_cadrSize;
        m_cadrSize=0;
    }
}

// ZS1 retail VID_SOFTWARE::CreateMirror.
VID* VID_SOFTWARE::CreateMirror()
{
    return new VID_SOFTWARE(this);
}

// ZS1 retail tests the first frame shadow word at m_cadrs+m_cadrShift[0].
int VID_SOFTWARE::HaveShadow()
{
    if (!m_cadrs)
        return 0;
    return *reinterpret_cast<const short*>(m_cadrs+m_cadrShift[0]);
}

// ZS1 retail VID_SOFTWARE palette size = 0x400 bytes.
int VID_SOFTWARE::PaletteSize()
{
    return 1024;
}

// Retail name recovered from its own error text: SetReColorForArmy.  This is
// VID vtable slot +0x20, reached by script selector 125.  PORT13 inherited the
// base no-op, so palette marker colours were never materialized for software
// and hardware-Z VIDs.
void VID_SOFTWARE::SetScriptPackedValue125(int value)
{
    if (!IsPaletteType()) {
        Error(10,const_cast<char*>("SetReColorForArmy for non paletted vid"),0);
        return;
    }

    // Mirrors share m_cadrs/m_cadrShift.  Retail detaches the object being
    // recoloured from the circular mirror ring before touching palette bytes.
    if (m_mirrorNext!=this) {
        VID* const oldNext=m_mirrorNext;
        VID* prev=oldNext;
        while (prev->m_mirrorNext!=this)
            prev=prev->m_mirrorNext;
        prev->m_mirrorNext=oldNext;
        m_mirrorNext=this;

        unsigned char* const oldCadrs=m_cadrs;
        m_cadrs=static_cast<unsigned char*>(::operator new(static_cast<unsigned int>(m_cadrSize)));
        if (!m_cadrs) {
            Error(2,const_cast<char*>("SetGamma"),static_cast<unsigned long>(m_cadrSize));
            return;
        }
        g_vidMemoryInUse+=m_cadrSize;
        memcpy(m_cadrs,oldCadrs,static_cast<size_t>(m_cadrSize));

        int* const oldShift=m_cadrShift;
        m_cadrShift=static_cast<int*>(::operator new(static_cast<unsigned int>(m_dotFrameCount)*sizeof(int)));
        if (!m_cadrShift) {
            Error(2,const_cast<char*>("cadrShift"),static_cast<unsigned long>(m_dotFrameCount));
            return;
        }
        memcpy(m_cadrShift,oldShift,static_cast<size_t>(m_dotFrameCount)*sizeof(int));
    }

    const int paletteSize=PaletteSize();
    COLOR* const palette=reinterpret_cast<COLOR*>(
        m_cadrs + paletteSize*(IsAltGammaType()?4:1));
    const int tintR=(value>>16)&0xFF;
    const int tintG=(value>>8)&0xFF;
    const int tintB=value&0xFF;

    for (int i=0;i<256;++i) {
        const unsigned int packed=palette[i].color;
        const int red=static_cast<int>((packed>>16)&0xFFu);
        const int green=static_cast<int>((packed>>8)&0xFFu);
        const int blue=static_cast<int>(packed&0xFFu);

        // Marker-colour discriminator from 0x004285B9..0x00428623.
        // The peculiar 148/80/30 ratios are genuine retail constants.
        if (red<=20)
            continue;
        const float marker=static_cast<float>(red)*(1.0f/148.0f);
        if (green>RetailFtolLow32(marker*80.0f))
            continue;
        int rb=red-blue;
        if (rb<0) rb=-rb;
        if (rb>RetailFtolLow32(marker*30.0f))
            continue;

        const float scale=static_cast<float>(red)*(1.0f/128.0f);
        int outR=RetailFtolLow32(static_cast<float>(tintR)*scale);
        int outG=RetailFtolLow32(static_cast<float>(tintG)*scale);
        int outB=RetailFtolLow32(static_cast<float>(tintB)*scale);
        if (outR<0) outR=0; else if (outR>255) outR=255;
        if (outG<0) outG=0; else if (outG>255) outG=255;
        if (outB<0) outB=0; else if (outB>255) outB=255;

        // Retail then adds the source green component as a grey brightness
        // term, saturating each 8-bit channel independently.
        outR+=green; if (outR>255) outR=255;
        outG+=green; if (outG>255) outG=255;
        outB+=green; if (outB>255) outB=255;
        palette[i].color=0xFF000000u |
                         (static_cast<unsigned int>(outR)<<16) |
                         (static_cast<unsigned int>(outG)<<8) |
                         static_cast<unsigned int>(outB);
    }

    if (IsAltGammaType()) {
        SetGamma(&m_gammaByArmy[0],0u);
        SetGamma(&m_gammaByArmy[1],1u);
        SetGamma(&m_gammaByArmy[2],2u);
        SetGamma(&m_gammaByArmy[3],3u);
    } else {
        SetGamma(&m_gamma,4u);
    }
}

void VID_SOFTWARE::SetLayer()
{
    // ZS1 retail 0x004278D0..0x00427993.  PORT13 still carried the
    // neighbouring AS1 layer map here: no-direction ALWAYSTOP sprites were
    // sent to layer 15 instead of ZS1 layer 18, and sprite type 0x10 missed
    // its dedicated layer 16 route.  Both differences are visible in the
    // editor because ZS1 draws twenty-one layers.
    if ((m_propertyBits&0x20u)!=0u)
        m_layer=IsAlphaType()?2:1;
    else if (PropGround())
        m_layer=3;
    else if (PropAlwaysTop() && m_noDirections==0xFFu)
        m_layer=18;
    else if (PropAlwaysTop() && m_unknown0C==0x10u)
        m_layer=16;
    else if (PropAlwaysTop())
        m_layer=13;
    else if (IsAlphaType())
        m_layer=7;
    else if (PropBuildSizeToGridZ() || PropBuildVidZToGridZ())
        m_layer=5;
    else
        m_layer=6;

    // ZS1 target 0x00427993..0x00427D8F.  This grid/link-dot build is part
    // of SetLayer itself, not VID_SOFTWARE::Load.  Retail therefore rebuilds
    // the collision dots whenever this virtual is re-run (including editor
    // conversion/mirror paths) rather than only on the initial DATA load.
    // PropBuildVidZToGridZ generates collision/link dots from the packed
    // software frame.  The 2048x2048 source domain is reduced to a 256x256
    // grid (8x8 cells), exactly matching the original stack buffer walk.
    if (PropBuildVidZToGridZ() && IsPaletteType() && IsTextureType()) {
        float* const scratch=static_cast<float*>(::operator new(0x3000000u));
        m_dotFrameStarts=static_cast<int*>(::operator new(static_cast<size_t>(m_dotFrameCount)*sizeof(int)));

        for (int ncadr=0;ncadr<m_dotFrameCount;++ncadr) {
            short grid[65536];
            for (int i=0;i<65536;++i)
                grid[i]=static_cast<short>(-32000);

            m_dotFrameStarts[ncadr]=m_nLinkDots;
            unsigned char* p=m_cadrs+m_cadrShift[ncadr];
            const short noContour=*reinterpret_cast<short*>(p);
            p+=2+6*noContour;

            if (IsPaletteType() && IsTextureType() && IsZBufferType()) {
                int y=*reinterpret_cast<short*>(p); p+=2;
                const int endY=y+*reinterpret_cast<short*>(p); p+=2;
                while (y<endY) {
                    int x=0;
                    while (*reinterpret_cast<short*>(p)) {
                        x+=*p++;
                        const int noDot=*p++;
                        unsigned char* zdata=p;
                        for (int j=0;j<noDot;++j) {
                            const int z=(static_cast<int>(*reinterpret_cast<unsigned short*>(zdata))>>3)-128;
                            const int zz=z+y;
                            const int xx=x+j;
                            if (zz>=0 && zz<2048 && xx>=0 && xx<2048) {
                                short& cell=grid[(zz/8)*256+(xx/8)];
                                if (z>cell)
                                    cell=static_cast<short>(z);
                            }
                            zdata+=2;
                        }
                        p+=3*noDot;
                        x+=noDot;
                    }
                    ++y;
                    p+=2;
                }
            } else if (IsPaletteType() && IsTextureType()) {
                int y=*reinterpret_cast<short*>(p); p+=2;
                const int endY=y+*reinterpret_cast<short*>(p); p+=2;
                while (y<endY) {
                    int x=0;
                    while (*reinterpret_cast<short*>(p)) {
                        x+=*p++;
                        const int noDot=*p++;
                        for (int j=0;j<noDot;++j) {
                            const int xx=x+j;
                            if (y>=0 && y<2048 && xx>=0 && xx<2048)
                                grid[(y/8)*256+(xx/8)]=0;
                        }
                        p+=noDot;
                        x+=noDot;
                    }
                    ++y;
                    p+=2;
                }
            }

            for (int y=m_regionTileStepY/8-1;y>=0;--y) {
                for (int x=0;x<m_regionTileStepX/8;++x) {
                    const short z=grid[y*256+x];
                    if (z!=static_cast<short>(-32000)) {
                        // Original performs signed integer /2 first (CDQ/SUB/SAR),
                        // then converts the result to float.  Dividing by 2.0f would
                        // incorrectly introduce a half-cell offset for odd extents.
                        const int halfWidth=m_regionTileStepX/2;
                        const int halfHeight=m_regionTileStepY/2;
                        scratch[m_nLinkDots*3+0]=static_cast<float>(x*8-halfWidth);
                        scratch[m_nLinkDots*3+1]=static_cast<float>(y*8-halfHeight);
                        scratch[m_nLinkDots*3+2]=static_cast<float>(z);
                        ++m_nLinkDots;
                    }
                }
            }
        }

        m_linkDots=static_cast<VID_DOT*>(::operator new(static_cast<size_t>(m_nLinkDots)*sizeof(VID_DOT)));
        for (int i=0;i<m_nLinkDots;++i) {
            m_linkDots[i].x=scratch[i*3+0];
            m_linkDots[i].y=scratch[i*3+1];
            m_linkDots[i].z=scratch[i*3+2];
        }
        ::operator delete(scratch);
    }
}

// ZS1 retail 32-bit palette gamma transform, 256 COLOR entries.
void VID_SOFTWARE::SetGammaToPalette(unsigned char* palette,const GAMMA* gamma)
{
    if (!palette || (gamma->subtractive==0 && gamma->additive==0))
        return;

    // ZS1 0x00427DA0..0x00427EB2: COLOR(GAMMA,COLOR) and COLOR::operator=
    // are both folded into this owner.  Keep the packed source form explicit:
    // diffuse is ~subtractive, specular is additive, and each RGB channel is
    // multiplied by (diffuse+1)/256 before the additive term and clamp.
    const unsigned int subtractive=gamma->subtractive;
    const unsigned int additive=gamma->additive;
    const unsigned int diffuse=~subtractive;
    unsigned int* colors=reinterpret_cast<unsigned int*>(palette);
    for (int i=0;i<256;++i) {
        const unsigned int source=colors[i];
        if (subtractive==0 && additive==0) {
            colors[i]=source;
            continue;
        }

        int blue=static_cast<int>((source&0xFFu)*((diffuse&0xFFu)+1u)>>8);
        blue+=static_cast<int>(additive&0xFFu);
        int green=static_cast<int>(((source>>8)&0xFFu)*(((diffuse>>8)&0xFFu)+1u)>>8);
        green+=static_cast<int>((additive>>8)&0xFFu);
        int red=static_cast<int>(((source>>16)&0xFFu)*(((diffuse>>16)&0xFFu)+1u)>>8);
        red+=static_cast<int>((additive>>16)&0xFFu);

        if (blue<0) blue=0; else if (blue>255) blue=255;
        if (green<0) green=0; else if (green>255) green=255;
        if (red<0) red=0; else if (red>255) red=255;

        colors[i]=0xFF000000u |
                  (static_cast<unsigned int>(red)<<16) |
                  (static_cast<unsigned int>(green)<<8) |
                  static_cast<unsigned int>(blue);
    }
}

VID_SOFTWARE16::VID_SOFTWARE16()
    : VID_SOFTWARE()
{
}

VID_SOFTWARE16::VID_SOFTWARE16(VID_SOFTWARE16* source)
    : VID_SOFTWARE(source)
{
}

VID_SOFTWARE16::~VID_SOFTWARE16()
{
}

VID* VID_SOFTWARE16::CreateMirror()
{
    return new VID_SOFTWARE16(this);
}

// 16-bit sibling of SetReColorForArmy.  The marker test is performed after
// expanding RGB16, but the two colour terms are packed separately and then
// saturated in native RGB16 component space, exactly as retail does.
void VID_SOFTWARE16::SetScriptPackedValue125(int value)
{
    if (!IsPaletteType()) {
        Error(10,const_cast<char*>("SetReColorForArmy for non paletted vid"),0);
        return;
    }

    if (m_mirrorNext!=this) {
        VID* const oldNext=m_mirrorNext;
        VID* prev=oldNext;
        while (prev->m_mirrorNext!=this)
            prev=prev->m_mirrorNext;
        prev->m_mirrorNext=oldNext;
        m_mirrorNext=this;

        unsigned char* const oldCadrs=m_cadrs;
        m_cadrs=static_cast<unsigned char*>(::operator new(static_cast<unsigned int>(m_cadrSize)));
        if (!m_cadrs) {
            Error(2,const_cast<char*>("SetGamma"),static_cast<unsigned long>(m_cadrSize));
            return;
        }
        g_vidMemoryInUse+=m_cadrSize;
        memcpy(m_cadrs,oldCadrs,static_cast<size_t>(m_cadrSize));

        int* const oldShift=m_cadrShift;
        m_cadrShift=static_cast<int*>(::operator new(static_cast<unsigned int>(m_dotFrameCount)*sizeof(int)));
        if (!m_cadrShift) {
            Error(2,const_cast<char*>("cadrShift"),static_cast<unsigned long>(m_dotFrameCount));
            return;
        }
        memcpy(m_cadrShift,oldShift,static_cast<size_t>(m_dotFrameCount)*sizeof(int));
    }

    const int paletteSize=PaletteSize();
    RGB16* const palette=reinterpret_cast<RGB16*>(
        m_cadrs + paletteSize*(IsAltGammaType()?4:1));
    const int tintR=(value>>16)&0xFF;
    const int tintG=(value>>8)&0xFF;
    const int tintB=value&0xFF;

    for (int i=0;i<256;++i) {
        COLOR source(&palette[i]);
        const int red=static_cast<int>(source.Red());
        const int green=static_cast<int>(source.Green());
        const int blue=static_cast<int>(source.Blue());
        if (red<=20)
            continue;
        const float marker=static_cast<float>(red)*(1.0f/148.0f);
        if (green>RetailFtolLow32(marker*80.0f))
            continue;
        int rb=red-blue;
        if (rb<0) rb=-rb;
        if (rb>RetailFtolLow32(marker*30.0f))
            continue;

        const float scale=static_cast<float>(red)*(1.0f/128.0f);
        int tr=RetailFtolLow32(static_cast<float>(tintR)*scale);
        int tg=RetailFtolLow32(static_cast<float>(tintG)*scale);
        int tb=RetailFtolLow32(static_cast<float>(tintB)*scale);
        if (tr<0) tr=0; else if (tr>255) tr=255;
        if (tg<0) tg=0; else if (tg>255) tg=255;
        if (tb<0) tb=0; else if (tb>255) tb=255;

        COLOR tint(tr,tg,tb);
        COLOR grey(green,green,green);
        RGB16 tint16(&tint);
        RGB16 grey16(&grey);
        const unsigned int a=tint16.color;
        const unsigned int b=grey16.color;
        unsigned int bluePart=(a&0x1Fu)+(b&0x1Fu);
        if (bluePart>0x1Fu) bluePart=0x1Fu;
        unsigned int greenPart=(a&RGB16::gMask)+(b&RGB16::gMask);
        if (greenPart>RGB16::gMask) greenPart=RGB16::gMask;
        unsigned int redPart=(a&RGB16::rMask)+(b&RGB16::rMask);
        if (redPart>RGB16::rMask) redPart=RGB16::rMask;
        unsigned int out=redPart|greenPart|bluePart;
        if (RGB16::rMask==0x7C00u)
            out|=0x8000u;
        palette[i].color=static_cast<unsigned short>(out);
    }

    if (IsAltGammaType()) {
        SetGamma(&m_gammaByArmy[0],0u);
        SetGamma(&m_gammaByArmy[1],1u);
        SetGamma(&m_gammaByArmy[2],2u);
        SetGamma(&m_gammaByArmy[3],3u);
    } else {
        SetGamma(&m_gamma,4u);
    }
}

int VID_SOFTWARE16::PaletteSize()
{
    return (IsAlphaType()?4:2)<<8;
}

// ZS1 retail 16-bit/alpha palette gamma transform.
void VID_SOFTWARE16::SetGammaToPalette(unsigned char* palette,const GAMMA* gamma)
{
    if (!palette || (gamma->subtractive==0 && gamma->additive==0))
        return;

    const unsigned int subtractive=gamma->subtractive;
    const unsigned int additive=gamma->additive;
    const unsigned int diffuse=~subtractive;

    // ZS1 0x00427EC0..0x00427FD6: alpha palettes use the exact same direct
    // 32-bit transform as VID_SOFTWARE; retail emits no COLOR/GAMMA calls.
    if ((m_extraTypeFlags&0x0002u)!=0) {
        unsigned int* colors=reinterpret_cast<unsigned int*>(palette);
        for (int i=0;i<256;++i) {
            const unsigned int source=colors[i];
            if (subtractive==0 && additive==0) {
                colors[i]=source;
                continue;
            }

            int blue=static_cast<int>((source&0xFFu)*((diffuse&0xFFu)+1u)>>8);
            blue+=static_cast<int>(additive&0xFFu);
            int green=static_cast<int>(((source>>8)&0xFFu)*(((diffuse>>8)&0xFFu)+1u)>>8);
            green+=static_cast<int>((additive>>8)&0xFFu);
            int red=static_cast<int>(((source>>16)&0xFFu)*(((diffuse>>16)&0xFFu)+1u)>>8);
            red+=static_cast<int>((additive>>16)&0xFFu);

            if (blue<0) blue=0; else if (blue>255) blue=255;
            if (green<0) green=0; else if (green>255) green=255;
            if (red<0) red=0; else if (red>255) red=255;

            colors[i]=0xFF000000u |
                      (static_cast<unsigned int>(red)<<16) |
                      (static_cast<unsigned int>(green)<<8) |
                      static_cast<unsigned int>(blue);
        }
        return;
    }

    // ZS1 0x00427FD6..0x00428135: non-alpha palettes inline both RGB16->COLOR
    // expansion and COLOR->RGB16 repacking using the runtime 555/565 masks.
    const int rShift=RGB16::rShift;
    const int gShift=RGB16::gShift;
    const unsigned int rMask=RGB16::rMask;
    const unsigned int gMask=RGB16::gMask;
    unsigned short* colors=reinterpret_cast<unsigned short*>(palette);
    for (int i=0;i<256;++i) {
        const unsigned int packed=colors[i];
        unsigned int source=0xFF000000u |
            ((packed<<(16-rShift))&0x00FF0000u) |
            ((packed<<(8-gShift))&0x0000FF00u) |
            ((packed&0x1Fu)<<3);

        unsigned int transformed=source;
        if (subtractive!=0 || additive!=0) {
            int blue=static_cast<int>((source&0xFFu)*((diffuse&0xFFu)+1u)>>8);
            blue+=static_cast<int>(additive&0xFFu);
            int green=static_cast<int>(((source>>8)&0xFFu)*(((diffuse>>8)&0xFFu)+1u)>>8);
            green+=static_cast<int>((additive>>8)&0xFFu);
            int red=static_cast<int>(((source>>16)&0xFFu)*(((diffuse>>16)&0xFFu)+1u)>>8);
            red+=static_cast<int>((additive>>16)&0xFFu);

            if (blue<0) blue=0; else if (blue>255) blue=255;
            if (green<0) green=0; else if (green>255) green=255;
            if (red<0) red=0; else if (red>255) red=255;

            transformed=0xFF000000u |
                (static_cast<unsigned int>(red)<<16) |
                (static_cast<unsigned int>(green)<<8) |
                static_cast<unsigned int>(blue);
        }

        colors[i]=static_cast<unsigned short>(
            ((transformed>>(16-rShift))&rMask) |
            ((transformed>>(8-gShift))&gMask) |
            ((transformed>>3)&0x1Fu));
    }
}

VID_HARDWARE_Z::VID_HARDWARE_Z()
    : VID_SOFTWARE()
{
}

VID_HARDWARE_Z::VID_HARDWARE_Z(VID_HARDWARE_Z* source)
    : VID_SOFTWARE(source)
{
}

VID_HARDWARE_Z::~VID_HARDWARE_Z()
{
}

// ZS1 retail VID_HARDWARE_Z::CreateMirror.
VID* VID_HARDWARE_Z::CreateMirror()
{
    return new VID_HARDWARE_Z(this);
}

// ZS1 retail body is an exact no-op (ret 0x10).
void VID_HARDWARE_Z::DrawToVid(const SPRITE* /*sprite*/,const VID_TEXCOOR* /*coor*/,TEXTURE* /*video_tex*/,TEXTURE* /*z_tex*/)
{
}

// ZS1 retail wrapper forwards directly to VID::SetGamma.
void VID_HARDWARE_Z::SetGamma(const GAMMA* gamma,unsigned int n_gamma)
{
    VID::SetGamma(gamma,n_gamma);
}

// ZS1 retail VID_HARDWARE_Z layer selection.
void VID_HARDWARE_Z::SetLayer()
{
    if (PropGround())
        m_layer=4;
    else if (m_unknown0C==0x40u)
        m_layer=10;
    else if (IsZBufferType() && IsAlphaType())
        m_layer=12;
    else if (IsAlphaType())
        m_layer=PropWave()?9:12;
    else
        m_layer=8;
}

// ZS1 size 0x494, original vtable 0x004B5324.
VID_FONT::VID_FONT()
    : VID(), m_font(0)
{
}

// Retail copy constructor intentionally only constructs VID and installs the
// VID_FONT vtable; it does not copy/initialize +0x490.
VID_FONT::VID_FONT(VID_FONT* /*source*/)
    : VID()
{
}

VID_FONT::~VID_FONT()
{
}

VID* VID_FONT::CreateMirror()
{
    return new VID_FONT(this);
}

// These are genuine no-op bodies in MapEdit.exe, not reconstruction stubs.
void VID_FONT::Load(RESOURCE* /*res*/) {}
void VID_FONT::SetLayer() {}
void VID_FONT::Draw(const SPRITE* /*sprite*/) {}
void VID_FONT::RestoreDeviceObjects() {}
void VID_FONT::InvalidateDeviceObjects() {}

// ---------------------------------------------------------------------------
// VID_LIGHT — complete retail owner used by MAP::CreateVid.
// Vtable 0x004B52FC, ZS1 sizeof 0x498.
// ---------------------------------------------------------------------------

VID_LIGHT::VID_LIGHT()
    : VID(), m_cadrSize(0), m_cadrs(0)
{
}

// ZS1 retail VID_LIGHT copy constructor.
VID_LIGHT::VID_LIGHT(VID_LIGHT* source)
    : VID()
{
    m_mirrorNext=source->m_mirrorNext;
    source->m_mirrorNext=this;
    m_layer=source->m_layer;
    m_extraTypeFlags=source->m_extraTypeFlags;
    m_dotFrameCount=source->m_dotFrameCount;
    m_phaseRandomInterval=source->m_phaseRandomInterval;
    m_regionTileStepX=source->m_regionTileStepX;
    m_regionTileStepY=source->m_regionTileStepY;
    m_cadrs=source->m_cadrs;
    m_cadrSize=source->m_cadrSize;
}

// ZS1 retail VID_LIGHT destructor.
VID_LIGHT::~VID_LIGHT()
{
    if (m_mirrorNext==this) {
        if (m_cadrs)
            operator delete(m_cadrs);
        m_cadrs=0;
        g_vidMemoryInUse-=m_cadrSize;
        m_cadrSize=0;
    }
}

// ZS1 retail VID_LIGHT::CreateMirror.
VID* VID_LIGHT::CreateMirror()
{
    return new VID_LIGHT(this);
}

// ZS1 retail VID_LIGHT DATA loader.
void VID_LIGHT::Load(RESOURCE* res)
{
    if (res->GoNext(0x41544144u))
        Error(5,const_cast<char*>("DATA"),0);

    // Retail executes the VC6 _ftol helper (0x0049909C) and stores AX.
    // Keep its truncate/overflow boundary semantics instead of relying on the
    // current compiler's float-to-int lowering.
    m_regionTileStepX=static_cast<short>(RetailFtolLow32(m_footprintWidth));
    m_regionTileStepY=static_cast<short>(RetailFtolLow32(m_footprintHeight));
    m_cadrSize=res->SubLoad(reinterpret_cast<void**>(&m_cadrs),0);
    if (!m_cadrSize)
        Error(5,const_cast<char*>("cadr"),0);
    g_vidMemoryInUse+=m_cadrSize;
}

namespace {
void SetRetailLightTextureStageColorOp(unsigned long value)
{
    void* const device=Graph->D3DDevice();
    void** const vtable=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *SetTextureStageStateFn)(void*,unsigned long,unsigned long,unsigned long);
    reinterpret_cast<SetTextureStageStateFn>(vtable[0x94/4])(device,0u,1u,value);
}
}

// ZS1 retail VID_LIGHT draw/gamma owner.
void VID_LIGHT::Draw(const SPRITE* sprite)
{
    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    COLOR color=m_cadrs[spr->CurrentCadr()];
    if (PropHide())
        return;

    COLOR transparentBlack(0,0,0,0);
    if (color.operator==(&transparentBlack))
        return;
    COLOR opaqueBlack(0,0,0);
    if (color.operator==(&opaqueBlack))
        return;

    if (PropDblLight())
        SetRetailLightTextureStageColorOp(5u);

    GAMMA* const vidGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x2E0);
    if (PropGamma()) {
        GAMMA spriteGamma=spr->GetGamma();
        GAMMA sum=*vidGamma + &spriteGamma;
        COLOR transformed(&sum,&color);
        color.operator=(&transformed);
    } else {
        GAMMA graphGamma=Graph->GetGamma();
        GAMMA spriteGamma=spr->GetGamma();
        GAMMA first=*vidGamma + &spriteGamma;
        GAMMA sum=first + &graphGamma;
        COLOR transformed(&sum,&color);
        color.operator=(&transformed);
    }

    Graph->DrawLightSource(spr->ScreenX(),spr->ScreenY(),spr->Z(),
                           m_footprintWidth,m_footprintHeight,color);

    if (PropDblLight())
        SetRetailLightTextureStageColorOp(4u);
}

// This is the editor-era software VID loader.  In particular, unlike the
// newer portable branch it does not add a per-frame alignment reserve to
// m_cadrSize and it advances frame payloads by the exact packed size.
void VID_SOFTWARE::Load(RESOURCE* res)
{
    COLOR palette[256];
    QS1_CODER* comp=IsCompressType() ? new QS1_CODER(1) : 0;

    if (IsPaletteType()) {
        if (!res->GoNext(0x204C4150u)) { // 'PAL '
            if (IsNewVersionType()) {
                res->Read(palette,1024u);
            } else {
                unsigned char palette24[768];
                res->Read(palette24,768u);
                for (int i=0;i<256;++i) {
                    COLOR color(palette24[i*3],palette24[i*3+1],palette24[i*3+2]);
                    palette[i].operator=(&color);
                }
            }

            GAMMA* const vidGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x2E0);
            for (int i=0;i<256;++i) {
                COLOR transformed(vidGamma,&palette[i]);
                palette[i].operator=(&transformed);
            }
        } else {
            Error(5,const_cast<char*>("PAL "),0);
        }
    }

    if (res->GoNext(0x41544144u)) // 'DATA'
        Error(5,const_cast<char*>("DATA"),0);

    m_cadrSize=res->ResSize();
    if (IsPaletteType())
        m_cadrSize+=2*PaletteSize();

    m_cadrs=static_cast<unsigned char*>(::operator new(static_cast<size_t>(m_cadrSize)));
    if (!m_cadrs) {
        Error(2,const_cast<char*>("cadr"),static_cast<unsigned long>(m_cadrSize));
        return;
    }

    m_cadrShift=static_cast<int*>(::operator new(static_cast<size_t>(m_dotFrameCount)*sizeof(int)));
    if (!m_cadrShift) {
        Error(2,const_cast<char*>("cadrShift"),static_cast<unsigned long>(m_dotFrameCount));
        return;
    }

    int shift=0;
    if (IsPaletteType()) {
        for (int i=0;i<256;++i) {
            if (PaletteSize()==1024) {
                reinterpret_cast<COLOR*>(m_cadrs)[i].operator=(&palette[i]);
            } else {
                RGB16 packed(&palette[i]);
                reinterpret_cast<RGB16*>(m_cadrs)[i].color=packed.color;
            }
        }
        shift=2*PaletteSize();
        memcpy(m_cadrs+PaletteSize(),m_cadrs,static_cast<size_t>(PaletteSize()));
    }

    for (int i=0;i<m_dotFrameCount;++i) {
        int size=0;
        res->Read(&size,4u);
        const int err=res->ReadPacked(m_cadrs+shift,static_cast<unsigned int>(size),comp);
        if (err)
            Error(5,const_cast<char*>("Can't decode software"),static_cast<unsigned long>(size-err));

        if (size!=2) {
            // ZS1 retail 0x004274A9..0x004277C9: direct (non-paletted,
            // non-alpha) frame pixels need a conversion pass when either the
            // active surface is RGB555 *or* this VID has a non-default gamma.
            // PORT13 only handled the RGB555 case and performed a raw 565->555
            // bit shift, so gamma on direct-color VID data was silently lost.
            if (!IsPaletteType() && !IsAlphaType() &&
                (Graph->Is15Bit() || !m_gamma.IsDefault())) {
                unsigned char* p=m_cadrs+shift;
                const short noContour=*reinterpret_cast<short*>(p);
                p+=2+6*noContour;
                int y=*reinterpret_cast<short*>(p); p+=2;
                const int endY=y+*reinterpret_cast<short*>(p); p+=2;
                while (y<endY) {
                    while (*reinterpret_cast<short*>(p)) {
                        ++p; // retail scanline skip byte
                        int noDot=*p++;
                        for (int j=0;j<noDot;++j) {
                            const unsigned short packed=*reinterpret_cast<unsigned short*>(p);
                            const unsigned int expanded=
                                0xFF000000u |
                                ((static_cast<unsigned int>(packed)&0xF800u)<<8) |
                                ((static_cast<unsigned int>(packed)&0x07E0u)<<5) |
                                ((static_cast<unsigned int>(packed)&0x001Fu)<<3);
                            COLOR source;
                            source.color=expanded;
                            COLOR corrected(&m_gamma,&source);
                            const unsigned int c=corrected.color;
                            unsigned short out;
                            if (Graph->Is15Bit()) {
                                out=static_cast<unsigned short>(((c>>9)&0x7C00u) |
                                                               ((c>>6)&0x03E0u) |
                                                               ((c>>3)&0x001Fu));
                            } else {
                                out=static_cast<unsigned short>(((c>>8)&0xF800u) |
                                                               ((c>>5)&0x07E0u) |
                                                               ((c>>3)&0x001Fu));
                            }
                            *reinterpret_cast<unsigned short*>(p)=out;
                            p+=2;
                        }
                    }
                    p+=2;
                    ++y;
                }
            }
            m_cadrShift[i]=shift;
            shift+=size;
        } else {
            const short frame=*reinterpret_cast<short*>(m_cadrs+shift);
            m_cadrShift[i]=m_cadrShift[frame];
        }
        res->GoNextSub(0x41544144u);
    }

    delete comp;
    g_vidMemoryInUse+=m_cadrSize;
    SetLayer();


}


namespace {
// Retail vid.obj work globals used by the software raster helpers.
// MapEdit.exe addresses: Z vector 0x004BF120, active palette 0x004BF138.
// The recovered C++ keeps these as module-private state; VID_SOFTWARE::Draw
// supplies the values before dispatching a scanline helper.
short g_softwareRasterZ[4]={0,0,0,0};
void* g_softwareRasterPalette=0;
}

void DrawOpaqueSpan32(void* data,void* zbuffer,void* video,int no_dot)
{
    unsigned char* src=static_cast<unsigned char*>(data);
    short* z=static_cast<short*>(zbuffer);
    COLOR* dst=static_cast<COLOR*>(video);
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const short depth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        if (depth>z[i]) {
            z[i]=depth;
            dst[i]=palette[src[i]];
        }
    }
}

void DrawAlphaBlendedSpan32(unsigned char* data,unsigned short* zbuffer,COLOR* video,int no_dot)
{
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const unsigned short depth=static_cast<unsigned short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        if (depth>=zbuffer[i]) {
            COLOR source=palette[data[i]];
            video[i].AlphaAdd(source,source.Alpha());
        }
    }
}

void DrawOpaqueSpan16(void* data,void* zbuffer,void* video,int no_dot)
{
    unsigned char* src=static_cast<unsigned char*>(data);
    short* z=static_cast<short*>(zbuffer);
    unsigned short* dst=static_cast<unsigned short*>(video);
    unsigned short* palette=static_cast<unsigned short*>(g_softwareRasterPalette);
    const short depth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        if (depth>z[i]) {
            z[i]=depth;
            dst[i]=palette[src[i]];
        }
    }
}

void DrawAlphaBlendedSpan16(unsigned char* data,unsigned short* zbuffer,RGB16* video,int no_dot)
{
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const unsigned short depth=static_cast<unsigned short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        if (depth>=zbuffer[i]) {
            COLOR source=palette[data[i]];
            COLOR destination(&video[i]);
            destination.AlphaAdd(source,source.Alpha());
            RGB16 result(&destination);
            video[i].color=result.color;
        }
    }
}

void DrawAlphaBlendedMappedSpan16(unsigned char* data,unsigned short* zbuffer,RGB16* video,int no_dot,int max_left_shift)
{
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const int depth=static_cast<unsigned short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        int destinationIndex=i;
        const int destinationDepth=static_cast<unsigned short>(zbuffer[i]);
        if (depth<destinationDepth) {
            const int leftShift=(destinationDepth-depth)/16;
            if (leftShift>=max_left_shift)
                continue;
            destinationIndex=i-leftShift;
        }
        COLOR source=palette[data[i]];
        COLOR destination(&video[destinationIndex]);
        destination.AlphaAdd(source,source.Alpha());
        RGB16 result(&destination);
        video[destinationIndex].color=result.color;
    }
}

// Retail uses MMX and processes four 16-bit lanes at a time.  This source
// preserves the per-lane compare/update result; exact VC6 MMX codegen remains
// a reccmp tuning item rather than being faked with inline assembly.
void DrawDepthMaskedAlphaSpan16(void* data,void* zbuffer,void* surface,int no_dot)
{
    unsigned short* src=static_cast<unsigned short*>(data);
    short* z=static_cast<short*>(zbuffer);
    unsigned short* dst=static_cast<unsigned short*>(surface);
    for (int i=0;i<no_dot;++i) {
        const short depth=g_softwareRasterZ[i&3];
        if (depth>z[i]) {
            z[i]=depth;
            dst[i]=src[i];
        } else {
            dst[i]=0;
        }
    }
}

void DrawAlphaSpanWithDepthOffsets16(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,unsigned short* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    const int baseDepth=static_cast<short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        const int depth=baseDepth+static_cast<short>(zdelta[i]);
        const int destinationDepth=static_cast<short>(zbuffer[i]);
        const unsigned int source=data ? reinterpret_cast<unsigned short*>(data)[i] : 0u;
        if (depth<destinationDepth) {
            video[i]=0;
        } else if (depth>destinationDepth+0x7F) {
            video[i]=static_cast<unsigned short>(source);
        } else {
            int alpha=((depth-destinationDepth)*static_cast<int>(source))>>7;
            if (alpha>0xFFFF)
                alpha=0xF000;
            else
                alpha&=0xF000;
            video[i]=static_cast<unsigned short>((source&0x0FFFu)+static_cast<unsigned int>(alpha));
        }
    }
}

void DrawLightSpanWithDepthOffsets16(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,unsigned short* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    unsigned short* source=reinterpret_cast<unsigned short*>(data);
    const int baseDepth=static_cast<short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        const int depth=baseDepth+static_cast<short>(zdelta[i]);
        const int destinationDepth=static_cast<short>(zbuffer[i]);
        if (depth<destinationDepth) {
            video[i]=0;
        } else if (depth>destinationDepth+0x7F) {
            video[i]=static_cast<unsigned short>(source[i]|0xF000u);
        } else {
            const int alpha=(depth-destinationDepth)/8;
            video[i]=static_cast<unsigned short>(source[i]|(alpha<<12));
        }
    }
}

void DrawOpaqueSpanWithDepthOffsets32(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,COLOR* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const short baseDepth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        const short depth=static_cast<short>(baseDepth+zdelta[i]);
        if (depth>static_cast<short>(zbuffer[i])) {
            zbuffer[i]=static_cast<unsigned short>(depth);
            video[i]=palette[data[i]];
        }
    }
}

void DrawOpaqueSpanWithDepthOffsets16(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,void* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    unsigned short* dst=static_cast<unsigned short*>(video);
    unsigned short* palette=static_cast<unsigned short*>(g_softwareRasterPalette);
    const short baseDepth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        const short depth=static_cast<short>(baseDepth+zdelta[i]);
        if (depth>static_cast<short>(zbuffer[i])) {
            zbuffer[i]=static_cast<unsigned short>(depth);
            dst[i]=palette[data[i]];
        }
    }
}

// Retail software 32-bit raster owner.  This deliberately follows the
// editor-era packed-frame paths: there is no portable-branch UI scaling.
void VID_SOFTWARE::Draw(const SPRITE* sprite)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);
    int shiftx=static_cast<int>(spr->X())-static_cast<int>(Map->m_shiftX)-width/2;
    int shifty=static_cast<int>(spr->Y()-spr->Z())-static_cast<int>(Map->m_shiftY)-height/2;
    if (shiftx+width<g_vidViewXMinRetail || shiftx>=g_vidViewXMaxRetail ||
        shifty+height<g_vidViewYMinRetail || shifty>=g_vidViewYMaxRetail)
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wave_z=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wave_z;
        // VC6 signed division by 8 is truncation toward zero.
        shifty-=wave_z/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+contours*6;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    int no_row=*reinterpret_cast<short*>(p); p+=2;
    int max_y=y+no_row;
    if (y>=g_vidViewYMaxRetail || max_y<g_vidViewYMinRetail)
        return;
    if (max_y>g_vidViewYMaxRetail)
        max_y=g_vidViewYMaxRetail;

    int z_pitch=0;
    unsigned short* z_buffer=Graph->LockZ(&z_pitch);
    int pitch=0;
    COLOR* video=static_cast<COLOR*>(Graph->Lock(&pitch));

    unsigned char palette[1024];
    if (spr->HaveUniqueGamma()) {
        g_softwareRasterPalette=palette;
        const int palette_size=PaletteSize();
        const int base_offset=IsAltGammaType()?4*palette_size:0;
        memcpy(palette,m_cadrs+base_offset,static_cast<size_t>(palette_size));
        if (PropGamma()) {
            GAMMA gamma=spr->GetGamma();
            SetGammaToPalette(palette,&gamma);
        } else {
            GAMMA sprite_gamma=spr->GetGamma();
            GAMMA graph_gamma=Graph->GetGamma();
            GAMMA gamma=sprite_gamma+&graph_gamma;
            SetGammaToPalette(palette,&gamma);
        }
    } else {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
    }

    // ZS1 retail VID_SOFTWARE::Draw 0x00429244..0x0042934D.
    // UNIT-class editor placement previews use a private palette rebuilt from
    // the VID gamma and the exact CanPlace result.  This is deliberately
    // software32-only: VID_SOFTWARE16 has a different retail body.
    if (spr->Vid()->m_spriteClass==2u) {
        GAMMA placementGamma=m_gamma;
        SPRITE* const blocker=spr->CanPlace(spr->X(),spr->Y(),spr->Z());
        if (blocker==Mouse) {
            GAMMA blockedGround;
            blockedGround.subtractive=0x00BEBE00u;
            blockedGround.additive=0u;
            blockedGround.SetBlue(255);
            placementGamma=blockedGround;
        } else if (blocker) {
            GAMMA blockedSprite;
            blockedSprite.subtractive=0x0000BE00u;
            blockedSprite.additive=0x00FF0000u;
            blockedSprite.SetBlue(-190);
            placementGamma=blockedSprite;
        }

        g_softwareRasterPalette=palette;
        const int paletteSize=PaletteSize();
        const int paletteOffset=IsAltGammaType()?4*paletteSize:0;
        memcpy(palette,m_cadrs+paletteOffset,static_cast<size_t>(paletteSize));
        SetGammaToPalette(palette,&placementGamma);
    }

    // Alpha + texture + palette path.
    if (IsAlphaType() && IsTextureType() && IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        COLOR* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawAlphaBlendedSpan32(data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            g_softwareRasterZ[0]=static_cast<short>(g_softwareRasterZ[0]+dz);
        }
        return;
    }

    if (!IsTextureType())
        return;

    // Palette + per-pixel Z-delta stream.
    if (IsPaletteType() && IsZBufferType()) {
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=3*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
            }
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        COLOR* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=p[0];
                int nodot=p[1];
                p+=2;
                unsigned char* const zdata=p;
                unsigned char* data=p+2*nodot;
                p=data+nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    data+=source_offset;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawOpaqueSpanWithDepthOffsets32(zdata+2*source_offset,data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
        }
        return;
    }

    // Direct RGB16 packed pixels expanded into the 32-bit software surface.
    if (!IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=2*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        COLOR* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=2*nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    if (shiftz>=static_cast<int>(z_buffer[draw_x+i])) {
                        z_buffer[draw_x+i]=static_cast<unsigned short>(shiftz);
                        const unsigned int c=*reinterpret_cast<unsigned short*>(data+2*(source_offset+i));
                        video[draw_x+i].color=0xFF000000u |
                            ((c&0x1Fu)<<3) |
                            ((c<<(8-RGB16::gShift))&0x0000FF00u) |
                            ((c<<(16-RGB16::rShift))&0x00FF0000u);
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
        return;
    }

    // Normal paletted software frame.
    shiftz+=0x400;
    if (shiftz>0x7FFF)
        shiftz=0x7FFF;
    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
            --no_row;
        }
    }
    int dz=0;
    if (m_hitVerticalOffset>m_footprintHeight) {
        shiftz+=8*no_row;
        dz=-8;
    }
    g_softwareRasterZ[0]=static_cast<short>(shiftz);

    COLOR* max_video=video+max_y*pitch;
    video+=y*pitch;
    z_buffer+=y*z_pitch;
    const int unclipped=(shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail);
    while (video<max_video) {
        int x=shiftx;
        if (unclipped) {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                g_softwareRasterZ[0]=static_cast<short>(shiftz);
                DrawOpaqueSpan32(data,z_buffer+x,video+x,nodot);
                x+=nodot;
            }
        } else {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0) {
                    g_softwareRasterZ[0]=static_cast<short>(shiftz);
                    DrawOpaqueSpan32(data,z_buffer+draw_x,video+draw_x,nodot);
                }
            }
        }
        p+=2;
        video+=pitch;
        z_buffer+=z_pitch;
        shiftz+=dz;
    }
}

// Retail 16-bit raster path mirrors the packed-frame control flow of the
// 32-bit owner but writes RGB16 pixels and uses the dedicated 16-bit helpers.
void VID_SOFTWARE16::Draw(const SPRITE* sprite)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);
    int shiftx=static_cast<int>(spr->X())-static_cast<int>(Map->m_shiftX)-width/2;
    int shifty=static_cast<int>(spr->Y()-spr->Z())-static_cast<int>(Map->m_shiftY)-height/2;
    if (shiftx+width<g_vidViewXMinRetail || shiftx>=g_vidViewXMaxRetail ||
        shifty+height<g_vidViewYMinRetail || shifty>=g_vidViewYMaxRetail)
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wave_z=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wave_z;
        // VC6 signed division by 8 is truncation toward zero.
        shifty-=wave_z/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+contours*6;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    int no_row=*reinterpret_cast<short*>(p); p+=2;
    int max_y=y+no_row;
    if (y>=g_vidViewYMaxRetail || max_y<g_vidViewYMinRetail)
        return;
    if (max_y>g_vidViewYMaxRetail)
        max_y=g_vidViewYMaxRetail;

    int z_pitch=0;
    unsigned short* z_buffer=Graph->LockZ(&z_pitch);
    int pitch=0;
    RGB16* video=static_cast<RGB16*>(Graph->Lock(&pitch));

    unsigned char palette[1024];
    if (spr->HaveUniqueGamma()) {
        g_softwareRasterPalette=palette;
        const int palette_size=PaletteSize();
        const int base_offset=IsAltGammaType()?4*palette_size:0;
        memcpy(palette,m_cadrs+base_offset,static_cast<size_t>(palette_size));
        if (PropGamma()) {
            GAMMA gamma=spr->GetGamma();
            SetGammaToPalette(palette,&gamma);
        } else {
            GAMMA sprite_gamma=spr->GetGamma();
            GAMMA graph_gamma=Graph->GetGamma();
            GAMMA gamma=sprite_gamma+&graph_gamma;
            SetGammaToPalette(palette,&gamma);
        }
    } else {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
    }

    // Alpha + texture + palette path.
    if (IsAlphaType() && IsTextureType() && IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawAlphaBlendedSpan16(data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            g_softwareRasterZ[0]=static_cast<short>(g_softwareRasterZ[0]+dz);
        }
        return;
    }

    if (!IsTextureType())
        return;

    // Palette + per-pixel Z-delta stream.
    if (IsPaletteType() && IsZBufferType()) {
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=3*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
            }
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=p[0];
                int nodot=p[1];
                p+=2;
                unsigned char* const zdata=p;
                unsigned char* data=p+2*nodot;
                p=data+nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    data+=source_offset;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawOpaqueSpanWithDepthOffsets16(zdata+2*source_offset,data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
        }
        return;
    }

    // Direct RGB16 packed pixels expanded into the 32-bit software surface.
    if (!IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=2*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=2*nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    if (shiftz>=static_cast<int>(z_buffer[draw_x+i])) {
                        z_buffer[draw_x+i]=static_cast<unsigned short>(shiftz);
                        const unsigned int c=*reinterpret_cast<unsigned short*>(data+2*(source_offset+i));
                        video[draw_x+i].color=static_cast<unsigned short>(c);
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
        return;
    }

    // Normal paletted software frame.
    shiftz+=0x400;
    if (shiftz>0x7FFF)
        shiftz=0x7FFF;
    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
            --no_row;
        }
    }
    int dz=0;
    if (m_hitVerticalOffset>m_footprintHeight) {
        shiftz+=8*no_row;
        dz=-8;
    }
    g_softwareRasterZ[0]=static_cast<short>(shiftz);

    RGB16* max_video=video+max_y*pitch;
    video+=y*pitch;
    z_buffer+=y*z_pitch;
    const int unclipped=(shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail);
    while (video<max_video) {
        int x=shiftx;
        if (unclipped) {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                g_softwareRasterZ[0]=static_cast<short>(shiftz);
                DrawOpaqueSpan16(data,z_buffer+x,video+x,nodot);
                x+=nodot;
            }
        } else {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0) {
                    g_softwareRasterZ[0]=static_cast<short>(shiftz);
                    DrawOpaqueSpan16(data,z_buffer+draw_x,video+draw_x,nodot);
                }
            }
        }
        p+=2;
        video+=pitch;
        z_buffer+=z_pitch;
        shiftz+=dz;
    }
}

// Current MapEdit lineage: 0x004357DE..0x00436CE3.
// Retail 16-bit prerender path used by VID_HARDWARE::DrawVidToVid.
void VID_SOFTWARE16::DrawToVid(const SPRITE* sprite,const VID_TEXCOOR* coor,TEXTURE* video_tex,TEXTURE* z_tex)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);

    int shiftx=static_cast<int>(spr->X()-static_cast<float>(width/2)-
                                static_cast<float>(coor->shiftx)-static_cast<float>(coor->begx));
    int shifty=static_cast<int>(spr->Y()-spr->Z()-static_cast<float>(height/2)-
                                static_cast<float>(coor->shifty)-static_cast<float>(coor->begy));
    if (shiftx+width<g_vidViewXMinRetail || shiftx>=g_vidViewXMaxRetail ||
        shifty+height<g_vidViewYMinRetail || shifty>=g_vidViewYMaxRetail)
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wave_z=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wave_z;
        // VC6 signed divide by 8 truncates toward zero.
        shifty-=wave_z/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+contours*6;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    int no_row=*reinterpret_cast<short*>(p); p+=2;
    int max_y=y+no_row;
    if (y>=g_vidViewYMaxRetail || max_y<g_vidViewYMinRetail)
        return;
    if (max_y>g_vidViewYMaxRetail)
        max_y=g_vidViewYMaxRetail;

    int z_pitch=0;
    unsigned short* z_buffer=reinterpret_cast<unsigned short*>(z_tex->Lock(&z_pitch,0));
    z_pitch/=2;
    int pitch=0;
    RGB16* video=reinterpret_cast<RGB16*>(video_tex->Lock(&pitch,0));
    pitch/=2;

    // From the first raster-type dispatch through the direct-RGB tail,
    // retail shares a single texture-unlock epilogue at 0x0042C14F.
    do {
    // ZS1 0x0042B5C7..0x0042B844: alpha/palette has two distinct
    // horizontal owners in retail.  Keep the full-width fast path separate
    // from the clipped path instead of folding both into one portable loop.
    if (IsAlphaType() && IsTextureType() && IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
        g_softwareRasterZ[0]=static_cast<short>(shiftz);

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        if (shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail) {
            while (video<max_video) {
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int nodot=*p++;
                    DrawAlphaBlendedSpan16(p,z_buffer+x,video+x,nodot);
                    p+=nodot;
                    x+=nodot;
                }
                p+=2;
                video+=pitch;
                z_buffer+=z_pitch;
                shiftz+=dz;
                g_softwareRasterZ[0]=static_cast<short>(shiftz);
            }
        } else {
            while (video<max_video) {
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int nodot=*p++;
                    unsigned char* const data=p;
                    p+=nodot;
                    const int end=x+nodot;

                    // ZS1 0x0042B773..0x0042B807: retail keeps two
                    // clipping call sites.  A segment beginning left of the
                    // viewport clips from the source head; a segment already
                    // inside clips only its tail.  Do not collapse these into
                    // one normalized draw_x/nodot call site.
                    if (x<g_vidViewXMinRetail) {
                        if (end>g_vidViewXMinRetail) {
                            const int draw_end=(end>g_vidViewXMaxRetail)?g_vidViewXMaxRetail:end;
                            const int draw_count=draw_end-g_vidViewXMinRetail;
                            if (draw_count>0)
                                DrawAlphaBlendedSpan16(data+(g_vidViewXMinRetail-x),
                                                   z_buffer+g_vidViewXMinRetail,
                                                   video+g_vidViewXMinRetail,draw_count);
                        }
                    } else if (x<g_vidViewXMaxRetail) {
                        const int draw_count=(end>g_vidViewXMaxRetail)?
                                             (g_vidViewXMaxRetail-x):nodot;
                        if (draw_count>0)
                            DrawAlphaBlendedSpan16(data,z_buffer+x,video+x,draw_count);
                    }
                    x=end;
                }
                p+=2;
                video+=pitch;
                z_buffer+=z_pitch;
                shiftz+=dz;
                g_softwareRasterZ[0]=static_cast<short>(shiftz);
            }
        }
        break;
    }

    // ZS1 0x0042B849: every remaining raster owner first requires texture.
    if (!IsTextureType())
        break;

    // ZS1 0x0042B857..0x0042BC5D: palette + per-pixel-Z is expanded in
    // this owner in retail; it keeps its dedicated depth-offset raster path.
    if (IsPaletteType() && IsZBufferType()) {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        unsigned short* const palette=reinterpret_cast<unsigned short*>(m_cadrs+palette_offset);
        g_softwareRasterPalette=palette;
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        g_softwareRasterZ[1]=static_cast<short>(shiftz);
        g_softwareRasterZ[2]=static_cast<short>(shiftz);
        g_softwareRasterZ[3]=static_cast<short>(shiftz);

        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=3*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
            }
        }

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        if (shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail) {
            while (video<max_video) {
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=p[0];
                    const int nodot=p[1];
                    p+=2;
                    short* const zdata=reinterpret_cast<short*>(p);
                    unsigned char* const data=p+2*nodot;
                    p=data+nodot;
                    for (int i=0;i<nodot;++i) {
                        const int dst=x+i;
                        const short depth=static_cast<short>(g_softwareRasterZ[0]+zdata[i]);
                        if (depth>static_cast<short>(z_buffer[dst])) {
                            z_buffer[dst]=static_cast<unsigned short>(depth);
                            video[dst].color=palette[data[i]];
                        }
                    }
                    x+=nodot;
                }
                p+=2;
                video+=pitch;
                z_buffer+=z_pitch;
            }
        } else {
            while (video<max_video) {
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=p[0];
                    int nodot=p[1];
                    p+=2;
                    short* const zdata=reinterpret_cast<short*>(p);
                    unsigned char* const data=p+2*nodot;
                    p=data+nodot;
                    const int end=x+nodot;
                    int draw_x=x;
                    int source_offset=0;
                    x=end;
                    if (draw_x<g_vidViewXMinRetail) {
                        source_offset=g_vidViewXMinRetail-draw_x;
                        nodot-=source_offset;
                        draw_x=g_vidViewXMinRetail;
                    }
                    if (end>g_vidViewXMaxRetail)
                        nodot=g_vidViewXMaxRetail-draw_x;
                    for (int i=0;i<nodot;++i) {
                        const int dst=draw_x+i;
                        const short depth=static_cast<short>(g_softwareRasterZ[0]+zdata[source_offset+i]);
                        if (depth>static_cast<short>(z_buffer[dst])) {
                            z_buffer[dst]=static_cast<unsigned short>(depth);
                            video[dst].color=palette[data[source_offset+i]];
                        }
                    }
                }
                p+=2;
                video+=pitch;
                z_buffer+=z_pitch;
            }
        }
        break;
    }

    // ZS1 0x0042BC62..0x0042BEF2: paletted constant-Z owner.  Retail again
    // emits a full-width loop and a separately clipped loop.
    if (IsPaletteType()) {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        unsigned short* const palette=reinterpret_cast<unsigned short*>(m_cadrs+palette_offset);

        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        if (shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail) {
            while (video<max_video) {
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int nodot=*p++;
                    unsigned char* const data=p;
                    p+=nodot;
                    for (int i=0;i<nodot;++i) {
                        const int dst=x+i;
                        if (shiftz>=static_cast<int>(z_buffer[dst])) {
                            z_buffer[dst]=static_cast<unsigned short>(shiftz);
                            video[dst].color=palette[data[i]];
                        }
                    }
                    x+=nodot;
                }
                p+=2;
                video+=pitch;
                z_buffer+=z_pitch;
                shiftz+=dz;
            }
        } else {
            while (video<max_video) {
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    int nodot=*p++;
                    unsigned char* const data=p;
                    p+=nodot;
                    const int end=x+nodot;
                    int draw_x=x;
                    int source_offset=0;
                    x=end;
                    if (draw_x<g_vidViewXMinRetail) {
                        source_offset=g_vidViewXMinRetail-draw_x;
                        nodot-=source_offset;
                        draw_x=g_vidViewXMinRetail;
                    }
                    if (end>g_vidViewXMaxRetail)
                        nodot=g_vidViewXMaxRetail-draw_x;
                    for (int i=0;i<nodot;++i) {
                        const int dst=draw_x+i;
                        if (shiftz>=static_cast<int>(z_buffer[dst])) {
                            z_buffer[dst]=static_cast<unsigned short>(shiftz);
                            video[dst].color=palette[data[source_offset+i]];
                        }
                    }
                }
                p+=2;
                video+=pitch;
                z_buffer+=z_pitch;
                shiftz+=dz;
            }
        }
        break;
    }

    // ZS1 0x0042BEF7..0x0042C14A: direct RGB16 constant-Z owner, preserving
    // the same retail fast/clipped split as the paletted path.
    shiftz+=0x400;
    if (shiftz>0x7FFF)
        shiftz=0x7FFF;
    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=2*static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
            --no_row;
        }
    }
    int dz=0;
    if (m_hitVerticalOffset>m_footprintHeight) {
        shiftz+=8*no_row;
        dz=-8;
    }

    RGB16* max_video=video+max_y*pitch;
    video+=y*pitch;
    z_buffer+=y*z_pitch;
    if (shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail) {
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                const int nodot=*p++;
                unsigned short* const data=reinterpret_cast<unsigned short*>(p);
                p+=2*nodot;
                for (int i=0;i<nodot;++i) {
                    const int dst=x+i;
                    if (shiftz>=static_cast<int>(z_buffer[dst])) {
                        z_buffer[dst]=static_cast<unsigned short>(shiftz);
                        video[dst].color=data[i];
                    }
                }
                x+=nodot;
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
    } else {
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned short* const data=reinterpret_cast<unsigned short*>(p);
                p+=2*nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    const int dst=draw_x+i;
                    if (shiftz>=static_cast<int>(z_buffer[dst])) {
                        z_buffer[dst]=static_cast<unsigned short>(shiftz);
                        video[dst].color=data[source_offset+i];
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
    }

    } while (0);

    video_tex->UnLock();
    z_tex->UnLock();
}

void VID_SOFTWARE::DrawShadow(const SPRITE* sprite)
{
    struct SHADOWVERTEX {
        float x,y,z,rhw;
        unsigned int diffuse;
        unsigned int specular;
    };

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    float shadow_z=spr->Z();
    if (!m_cadrs || !m_cadrShift || PropHide())
        return;

    const float shiftx=spr->ScreenX()-static_cast<float>(m_regionTileStepX/2);
    float shifty=spr->ScreenY()-static_cast<float>(m_regionTileStepY/2);
    if (static_cast<int>(shiftx)+m_regionTileStepX+200<g_vidViewXMinRetail ||
        static_cast<int>(shiftx)>=g_vidViewXMaxRetail ||
        static_cast<int>(shifty)+m_regionTileStepY+100<g_vidViewYMinRetail ||
        static_cast<int>(shifty)>=g_vidViewYMaxRetail)
        return;

    shifty+=shadow_z;
    if (PropWave())
        shadow_z=FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset+shadow_z;

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int no_dot=*reinterpret_cast<short*>(p);
    p+=2;
    if (!no_dot)
        return;

    SHADOWVERTEX pnt[514];
    int i=0;
    for (;i<2*no_dot;i+=2) {
        pnt[i].x=static_cast<float>(*reinterpret_cast<short*>(p))+shiftx; p+=2;
        const float y=static_cast<float>(*reinterpret_cast<short*>(p))+shifty; p+=2;
        const float z=static_cast<float>(*reinterpret_cast<short*>(p))+shadow_z; p+=2;
        pnt[i].y=y-z;
        pnt[i].z=z*0.0001220703125f+0.015625f;
        pnt[i].rhw=1.0f;
        pnt[i].diffuse=0x00A4A4A4u;
        pnt[i].specular=0;
        pnt[i+1].x=z*0.35f+pnt[i].x;
        pnt[i+1].y=y-z*0.70f;
        pnt[i+1].z=0.015625f;
        pnt[i+1].rhw=1.0f;
        pnt[i+1].diffuse=0x00A4A4A4u;
        pnt[i+1].specular=0;
    }
    pnt[i]=pnt[0];
    pnt[i+1]=pnt[1];

    Graph->SetRenderState(0x1Du,0u); // D3DRS_SPECULARENABLE
    void* const device=Graph->D3DDevice();
    if (device) {
        void** const vtable=*reinterpret_cast<void***>(device);
        typedef long (__stdcall *SetTextureFn)(void*,unsigned long,void*);
        reinterpret_cast<SetTextureFn>(vtable[0x8C/4])(device,0u,0);
    }
    Graph->SetRenderState(0x16u,3u); // D3DRS_CULLMODE = D3DCULL_CCW
    Graph->SetAlphaBlend(1u,3u);
    const int total=2*no_dot+2;
    for (i=0;i<total;++i)
        pnt[i].diffuse=0x00A4A4A4u;
    Graph->DrawPrimitive(5u,0xC4u,pnt,0x18u,total);

    Graph->SetRenderState(0x16u,2u); // D3DCULL_CW
    Graph->SetAlphaBlend(9u,2u);
    for (i=0;i<total;++i)
        pnt[i].diffuse=0x008F8F8Fu;
    Graph->DrawPrimitive(5u,0xC4u,pnt,0x18u,total);
    Graph->SetRenderState(0x16u,3u);
}

// ZS1 retail hardware-Z alpha/light-mask composite owner.
// Hardware-Z owner: builds the temporary A4R4G4B4 alpha/light mask against
// the software Z buffer, then composites that mask through GRAPH::AlphaBuffer.
void VID_HARDWARE_Z::Draw(const SPRITE* sprite)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);
    int shiftx=static_cast<int>(spr->X()-Map->m_shiftX-static_cast<float>(width/2));
    int shifty=static_cast<int>(spr->Y()-spr->Z()-Map->m_shiftY-static_cast<float>(height/2));
    if (shiftx+width<g_vidViewXMinRetail || shiftx>=g_vidViewXMaxRetail ||
        shifty+height<g_vidViewYMinRetail || shifty>=g_vidViewYMaxRetail)
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wavez=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wavez;
        shifty-=wavez/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+6*contours;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    const int rows=*reinterpret_cast<short*>(p); p+=2;
    int maxy=y+rows;
    if (y>=g_vidViewYMaxRetail || maxy<g_vidViewYMinRetail)
        return;
    if (maxy>g_vidViewYMaxRetail)
        maxy=g_vidViewYMaxRetail;

    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=4*static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
        }
    }

    RECT_OLD screenRect;
    screenRect.left=shiftx;
    screenRect.top=y;
    screenRect.right=shiftx+width;
    screenRect.bottom=maxy;

    RECT_OLD texRect;
    texRect.left=0;
    texRect.top=0;
    texRect.right=width;
    texRect.bottom=maxy-y;

    int zPitch=0;
    unsigned short* zBuffer=Graph->LockZ(&zPitch);
    TEXTURE* alpha=Graph->AlphaBuffer();
    int alphaPitchBytes=0;
    unsigned short* alphaPixels=reinterpret_cast<unsigned short*>(alpha->Lock(&alphaPitchBytes,&texRect));
    const int alphaPitch=alphaPitchBytes/2;
    g_softwareRasterZ[0]=static_cast<short>(shiftz);

    unsigned short* alphaRow=alphaPixels;
    // ZS1 0x0042E3BA increments the row count before computing the end pointer:
    // this packed owner processes (maxy-y)+1 scanlines.
    unsigned short* const alphaEnd=alphaPixels+alphaPitch*((maxy-y)+1);
    unsigned short* zRow=zBuffer+y*zPitch;

    // ZS1 0x0042E3E5 branches the two packed owners before entering the
    // raster loop.  Keep their full-width/clipped loops separate: the alpha
    // path calls 0x0042DF90 and finishes with SetAlphaBlend(5,6), while the
    // light path calls 0x0042E050 and finishes with SetAlphaBlend(5,2).
    if (IsAlphaType() && IsTextureType()) {
        if (shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail) {
            while (alphaRow<alphaEnd) {
                memset(alphaRow,0,static_cast<unsigned int>(alphaPitchBytes));
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int count=*p++;
                    unsigned char* const zdata=p;
                    unsigned char* const data=p+2*count;
                    DrawAlphaSpanWithDepthOffsets16(zdata,data,zRow+x,alphaRow+(x-shiftx),count);
                    p+=4*count;
                    x+=count;
                }
                p+=2;
                alphaRow+=alphaPitch;
                zRow+=zPitch;
            }
        } else {
            while (alphaRow<alphaEnd) {
                memset(alphaRow,0,static_cast<unsigned int>(alphaPitchBytes));
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int count=*p++;
                    unsigned char* const zdata=p;
                    unsigned char* const data=p+2*count;
                    p+=4*count;
                    const int runEnd=x+count;
                    int drawX=x;
                    int drawEnd=runEnd;
                    if (drawX<g_vidViewXMinRetail)
                        drawX=g_vidViewXMinRetail;
                    if (drawEnd>g_vidViewXMaxRetail)
                        drawEnd=g_vidViewXMaxRetail;
                    if (drawEnd>drawX) {
                        const int sourceOffset=drawX-x;
                        DrawAlphaSpanWithDepthOffsets16(zdata+2*sourceOffset,
                                          data+2*sourceOffset,
                                          zRow+drawX,
                                          alphaRow+(drawX-shiftx),
                                          drawEnd-drawX);
                    }
                    x=runEnd;
                }
                p+=2;
                alphaRow+=alphaPitch;
                zRow+=zPitch;
            }
        }
        Graph->SetAlphaBlend(5u,6u);
    } else {
        if (shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail) {
            while (alphaRow<alphaEnd) {
                memset(alphaRow,0,static_cast<unsigned int>(alphaPitchBytes));
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int count=*p++;
                    unsigned char* const zdata=p;
                    unsigned char* const data=p+2*count;
                    DrawLightSpanWithDepthOffsets16(zdata,data,zRow+x,alphaRow+(x-shiftx),count);
                    p+=4*count;
                    x+=count;
                }
                p+=2;
                alphaRow+=alphaPitch;
                zRow+=zPitch;
            }
        } else {
            while (alphaRow<alphaEnd) {
                memset(alphaRow,0,static_cast<unsigned int>(alphaPitchBytes));
                int x=shiftx;
                while (*reinterpret_cast<short*>(p)!=0) {
                    x+=*p++;
                    const int count=*p++;
                    unsigned char* const zdata=p;
                    unsigned char* const data=p+2*count;
                    p+=4*count;
                    const int runEnd=x+count;
                    int drawX=x;
                    int drawEnd=runEnd;
                    if (drawX<g_vidViewXMinRetail)
                        drawX=g_vidViewXMinRetail;
                    if (drawEnd>g_vidViewXMaxRetail)
                        drawEnd=g_vidViewXMaxRetail;
                    if (drawEnd>drawX) {
                        const int sourceOffset=drawX-x;
                        DrawLightSpanWithDepthOffsets16(zdata+2*sourceOffset,
                                          data+2*sourceOffset,
                                          zRow+drawX,
                                          alphaRow+(drawX-shiftx),
                                          drawEnd-drawX);
                    }
                    x=runEnd;
                }
                p+=2;
                alphaRow+=alphaPitch;
                zRow+=zPitch;
            }
        }
        Graph->SetAlphaBlend(5u,2u);
    }
    alpha->UnLock();
    Graph->SetRenderState(0x17u,8u); // D3DRS_ZFUNC / D3DCMP_ALWAYS

    // ZS1 0x0042E862 branches before gamma construction.  Retail does not
    // share the sprite/VID gamma temporary or TEXTURE::Draw across PropGamma:
    // both branches own their own temporary lifetime/call graph.
    GAMMA* const vidGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x2E0);
    if (PropGamma()) {
        GAMMA spriteGamma=spr->GetGamma();
        // ZS1 0x0042E87B..0x0042E8A4: retail constructs the first by-value
        // argument through GAMMA(const GAMMA*) and then invokes the two-GAMMA
        // constructor.  This is not GAMMA::operator+ in the target.
        GAMMA drawGamma(GAMMA(vidGamma),spriteGamma);
        alpha->Draw(&screenRect,&texRect,&drawGamma);
        return;
    }

    GAMMA graphGamma=Graph->GetGamma();
    GAMMA spriteGamma=spr->GetGamma();
    GAMMA drawGamma(GAMMA(vidGamma),spriteGamma);
    // ZS1 0x0042E91C..0x0042E93D repeats the same constructor pair for the
    // graph-gamma combine; retail does not call GAMMA::operator+= here.
    GAMMA finalGamma(GAMMA(&drawGamma),graphGamma);
    alpha->Draw(&screenRect,&texRect,&finalGamma);
}

void VID::SetAltGammaType()
{
    m_extraTypeFlags=static_cast<unsigned short>(m_extraTypeFlags|0x0400u);
}

void VID_SOFTWARE::SetGamma(const GAMMA* gamma,unsigned int n_gamma)
{
    const int paletteSize=PaletteSize();
    if (!m_cadrs || !IsPaletteType())
        return;

    GAMMA* const storedGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x3F0);

    if (n_gamma==4u) {
        if (IsAltGammaType()) {
            SetGamma(&storedGamma[0],0u);
            SetGamma(&storedGamma[1],1u);
            SetGamma(&storedGamma[2],2u);
            SetGamma(&storedGamma[3],3u);
        } else {
            memcpy(m_cadrs,m_cadrs+paletteSize,static_cast<size_t>(paletteSize));
            if (m_spriteClass!=8u && !PropGamma())
                SetGammaToPalette(m_cadrs,gamma);
        }
        return;
    }

    if (n_gamma>=4u) {
        Error(4,const_cast<char*>("n_gamma in VID_SOFTWARE::SetGamma"),n_gamma);
        return;
    }

    storedGamma[n_gamma].operator=(gamma);

    if (!IsAltGammaType()) {
        unsigned char* const oldCadrs=m_cadrs;
        m_cadrSize+=3*paletteSize;
        g_vidMemoryInUse+=3*paletteSize;
        m_cadrs=static_cast<unsigned char*>(operator new(static_cast<size_t>(m_cadrSize)));
        if (!m_cadrs) {
            Error(2,const_cast<char*>("SetGamma"),static_cast<unsigned long>(m_cadrSize));
            return;
        }

        memcpy(m_cadrs+4*paletteSize,
               oldCadrs+paletteSize,
               static_cast<size_t>(m_cadrSize-4*paletteSize));
        memcpy(m_cadrs,m_cadrs+4*paletteSize,static_cast<size_t>(paletteSize));
        memcpy(m_cadrs+paletteSize,m_cadrs+4*paletteSize,static_cast<size_t>(paletteSize));
        memcpy(m_cadrs+2*paletteSize,m_cadrs,static_cast<size_t>(2*paletteSize));
        operator delete(oldCadrs);

        if (m_cadrShift) {
            for (int i=0;i<m_dotFrameCount;++i)
                m_cadrShift[i]+=3*paletteSize;
        }

        SetAltGammaType();
        for (VID* mirror=m_mirrorNext;mirror!=this;mirror=mirror->m_mirrorNext) {
            mirror->SetAltGammaType();
            static_cast<VID_SOFTWARE*>(mirror)->m_cadrs=m_cadrs;
        }
    }

    memcpy(m_cadrs+static_cast<int>(n_gamma)*paletteSize,
           m_cadrs+4*paletteSize,
           static_cast<size_t>(paletteSize));

    unsigned char* const target=m_cadrs+static_cast<int>(n_gamma)*paletteSize;
    if (m_spriteClass==8u || PropGamma()) {
        SetGammaToPalette(target,gamma);
    } else {
        GAMMA graphGamma=Graph->GetGamma();
        GAMMA sum=const_cast<GAMMA*>(gamma)->operator+(&graphGamma);
        SetGammaToPalette(target,&sum);
    }
}


// ZS1 retail 0x00426770..0x00426864; chain-wide HP coefficient update.
void VID::SetHpCoeff(int army,int hpPercent)
{
    army&=3;
    const int oldMax=m_maxHp[army];
    if (hpPercent>=0) m_maxHp[army]=hpPercent*m_baseHp/100;
    if (m_baseHp) {
        int iter=Map->m_layers[m_layer].m_no-1;
        while (iter>=0) {
            SPRITE* spr=0;
            while (iter>=0 && !spr) {
                spr=Map->m_layers[m_layer].m_data[iter];
                if (!spr) --iter;
            }
            if (!spr) break;
            if (spr->m_vid==this && ((spr->m_flag>>12)&3u)==static_cast<unsigned int>(army)) {
                // Retail has no oldMax==0 safety guard before IDIV.
                const int scaledHp=((spr->m_hp*m_maxHp[army])*16/oldMax)/16;
                spr->ChangeHp(scaledHp);
            }
            --iter;
        }
    }
    if (m_linkVid) m_linkVid->SetHpCoeff(army,hpPercent);
}

// ZS1 retail 0x00426870..0x00426953; chain-wide absolute max-HP update.
void VID::SetMaxHp(int army,int newHp)
{
    army&=3;
    const int oldMax=m_maxHp[army];
    if (newHp>=0) m_maxHp[army]=newHp;
    if (m_baseHp) {
        int iter=Map->m_layers[m_layer].m_no-1;
        while (iter>=0) {
            SPRITE* spr=0;
            while (iter>=0 && !spr) {
                spr=Map->m_layers[m_layer].m_data[iter];
                if (!spr) --iter;
            }
            if (!spr) break;
            if (spr->m_vid==this && ((spr->m_flag>>12)&3u)==static_cast<unsigned int>(army)) {
                // Retail has no oldMax==0 safety guard before IDIV.
                const int scaledHp=((spr->m_hp*m_maxHp[army])*256/oldMax)/256;
                spr->ChangeHp(scaledHp);
            }
            --iter;
        }
    }
    if (m_linkVid) m_linkVid->SetMaxHp(army,newHp);
}
int VID::Killed(int army) { return m_killed[army&3]; }
int VID::Killed() { return m_killed[0]+m_killed[1]+m_killed[2]+m_killed[3]; }
// ZS1 retail VID::GetRecolors(int).
int VID::ReColored(int army) { return m_reColored[army&3]; }
int VID::ReColored() { return m_reColored[0]+m_reColored[1]+m_reColored[2]+m_reColored[3]; }
// ZS1 retail setter for NotCreateAsChild.
void VID::SetPropNotCreateAsChild(int on) { m_prop=static_cast<uint32_t>(on); }
