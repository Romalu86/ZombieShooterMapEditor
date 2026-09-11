#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"

// Source-integrated shared closure owners recovered directly from MapEdit.exe.
// These are intentionally grouped because they are common dependencies of
// MAP::CreateSprite, MAP::Load, MAP::ExecFunc and MAP_EDIT::WorkWndMessage.

// and returns the same-index sprite from the second list, or null when absent.
SPRITE* RELATION::Decode(SPRITE* old_sprite)
{
    const int index=oldSprites.Location(reinterpret_cast<SPRITE* const*>(&old_sprite));
    if(index<0)
        return 0;
    return *newSprites[index];
}

// the parallel old/new relation lists using the same list growth/copy path.
void RELATION::Insert(SPRITE* old_sprite,SPRITE* new_sprite)
{
    if(!old_sprite)
        return;
    oldSprites.Insert(old_sprite);
    newSprites.Insert(new_sprite);
}

SPRITE* MAP::Decode(SPRITE* old_sprite)
{
    return m_relation.Decode(old_sprite);
}

// ZS1 target 0x00417CF0..0x00417D24.
int MAP::ScriptRun(int n_func,const SPRITE* var1,const SPRITE* var2,int var3)
{
    if(m_flags&0x80000u)
        return 0;
    return m_logic.CallFunction(n_func,"ppi",var1,var2,var3);
}

// ZS1 0x00417FC0 / 0x00418050 / 0x0041C010 / 0x0041C070:
// MAP+0x4B18 is a raw, non-owning LIST<SPRITE*> containing sprites whose
// EX_SPRITE_DATA name is non-empty.  Retail performs the list mechanics
// inside each public owner; keep them inline instead of routing through
// reconstruction-only helpers, because those helpers alter the emitted call graph.

// ZS1 target 0x0041C010..0x0041C05E.
// Remove only from MAP+0x4B18 named-sprite tail list.  Unlike
// RemoveSpriteFromLayer this owner does not touch the render layer.
void MAP::RemoveNamedSprite(SPRITE* spr)
{
    const int no=m_zs1TailList.m_no;
    if(!no)
        return;

    int index=no;
    do {
        --index;
        if(m_zs1TailList.m_data[index]==spr)
            break;
    } while(index!=0);

    if(m_zs1TailList.m_data[index]!=spr || index<0 || index>=no)
        return;

    --m_zs1TailList.m_no;
    m_zs1TailList.m_data[index]=m_zs1TailList.m_data[m_zs1TailList.m_no];
}

// ZS1 target 0x0041C070..0x0041C12B.
// Insert only when EX_SPRITE_DATA::name is non-empty; the retail owner is a
// raw LIST<SPRITE*> insertion and deliberately does not AddRef the sprite.
void MAP::InsertNamedSprite(SPRITE* spr)
{
    if(!spr->m_exData || spr->m_exData->name.m_buf[0]==0)
        return;

    if(m_zs1TailList.m_no>=m_zs1TailList.m_max) {
        const int oldMax=m_zs1TailList.m_max;
        const int newMax=oldMax*2+4;
        if(newMax>oldMax) {
            SPRITE** const oldData=m_zs1TailList.m_data;
            SPRITE** const newData=static_cast<SPRITE**>(
                ::operator new(static_cast<unsigned int>(newMax)*sizeof(SPRITE*)));
            m_zs1TailList.m_data=newData;
            if(!newData)
                MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",newMax);

            if(oldData) {
                for(int i=0;i<oldMax;++i)
                    newData[i]=oldData[i];
                ::operator delete(oldData);
            }
            m_zs1TailList.m_max=newMax;
        }
    }

    m_zs1TailList.m_data[m_zs1TailList.m_no]=spr;
    ++m_zs1TailList.m_no;
}

// ZS1 target 0x00417FC0..0x0041804C.
void MAP::RemoveSpriteFromLayer(SPRITE* spr)
{
    if(spr->m_exData && spr->m_exData->name.m_buf[0]!=0) {
        const int no=m_zs1TailList.m_no;
        if(no) {
            int index=no;
            do {
                --index;
                if(m_zs1TailList.m_data[index]==spr)
                    break;
            } while(index!=0);

            if(m_zs1TailList.m_data[index]==spr && index>=0 && index<no) {
                --m_zs1TailList.m_no;
                m_zs1TailList.m_data[index]=m_zs1TailList.m_data[m_zs1TailList.m_no];
            }
        }
    }

    const int layer=spr->m_vid->m_layer;
    for(int i=m_layers[layer].m_no-1;i>=0;--i) {
        if(m_layers[layer].m_data[i]==spr) {
            m_layers[layer].m_data[i]=0;
            break;
        }
    }
}

// ZS1 target 0x00418050..0x0041816D.
void MAP::InsertSpriteToLayer(SPRITE* spr)
{
    if(spr->m_exData && spr->m_exData->name.m_buf[0]!=0) {
        if(m_zs1TailList.m_no>=m_zs1TailList.m_max) {
            const int oldMax=m_zs1TailList.m_max;
            const int newMax=oldMax*2+4;
            if(newMax>oldMax) {
                SPRITE** const oldData=m_zs1TailList.m_data;
                SPRITE** const newData=static_cast<SPRITE**>(
                    ::operator new(static_cast<unsigned int>(newMax)*sizeof(SPRITE*)));
                m_zs1TailList.m_data=newData;
                if(!newData)
                    MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",newMax);

                if(oldData) {
                    for(int i=0;i<oldMax;++i)
                        newData[i]=oldData[i];
                    ::operator delete(oldData);
                }
                m_zs1TailList.m_max=newMax;
            }
        }

        m_zs1TailList.m_data[m_zs1TailList.m_no]=spr;
        ++m_zs1TailList.m_no;
    }

    m_layers[spr->m_vid->m_layer].Insert(spr);
    spr->Release();
}

EX_SPRITE_DATA* SPRITE::ExData()
{
    return m_exData;
}

SPRITE* SPRITE::Goal()
{
    return m_goal;
}

void SPRITE::SetGoal(SPRITE* new_goal)
{
    if(m_goal==new_goal)
        return;
    if(m_goal)
        m_goal->Release();
    m_goal=new_goal;
    if(m_goal)
        m_goal->AddRef();
}

// ZS1 target 0x00455EA0..0x00455FAE.
// Name mutation has a second responsibility in retail: keep MAP+0x4B18 in
// sync when the empty/non-empty state changes.  This owner is also used by
// Action(81) while loading map format version 13+.
// or the retail empty STRING when m_exData (+0x64) is null. Semantic name inferred from use.
STRING SPRITE::Name() const
{
    return m_exData ? m_exData->name : STRING("");
}

void SPRITE::SetName(const STRING* name)
{
    STRING newName(name);
    int insertAfter=0;

    if(m_exData && m_exData->name.m_buf[0]!=0 && newName.m_buf[0]==0)
        Map->RemoveNamedSprite(this);

    if((!m_exData || m_exData->name.m_buf[0]==0) && newName.m_buf[0]!=0)
        insertAfter=1;

    if(!m_exData && newName.m_buf[0]!=0)
        m_exData=new EX_SPRITE_DATA(this);

    if(m_exData)
        m_exData->name=newName;

    if(insertAfter)
        Map->InsertNamedSprite(this);
}

// ZS1 target 0x00451050..0x0045106C.
void SPRITE::Remove()
{
    if(m_child)
        m_child->Remove();
    Map->RemoveSpriteFromLayer(this);
}

// ZS1 target 0x00451070..0x0045108C.
void SPRITE::Insert()
{
    if(m_child)
        m_child->Insert();
    Map->InsertSpriteToLayer(this);
}

int SPRITE::AddLink(SPRITE* additional)
{
    if(!additional || additional->m_parent)
        return 1;
    if(m_child) {
        m_child->m_parent=0;
        additional->AddLinkToLast(m_child);
    }
    m_child=additional;
    additional->m_parent=this;
    return 0;
}

int SPRITE::AddLinkToLast(SPRITE* additional)
{
    if(!additional || additional->m_parent)
        return 1;
    SPRITE* last=this;
    while(last->m_child)
        last=last->m_child;
    last->m_child=additional;
    additional->m_parent=last;
    return 0;
}

int PROFILE::Load(const STRING* filename)
{
    FileName=STRING::EMPTY;
    FileName=*filename;
    return 0;
}

int PROFILE::GetInt(const STRING* section,const STRING* keyword,int defaultValue)
{
    return static_cast<int>(GetPrivateProfileIntA(section->m_buf,keyword->m_buf,defaultValue,FileName.m_buf));
}

// ZS1 target 0x0044C360..0x0044C413.
STRING PROFILE::GetString(const STRING* section,const STRING* keyword,const STRING* defaultString)
{
    char buffer[32767];
    GetPrivateProfileStringA(section->m_buf,keyword->m_buf,defaultString->m_buf,buffer,32767u,FileName.m_buf);
    return STRING(buffer);
}

// MapEditZS1.exe 0x0044D0A0..0x0044D0B0. Reads the current menu sprite VID index.
int MENU::NVidUnderCursor()
{
    return sprite ? sprite->Vid()->m_idx : 0;
}

// MapEditZS1.exe 0x0044D0C0..0x0044D0DA. Converts the current menu sprite direction through VID::RealDirection.
int MENU::NDirUnderCursor()
{
    return sprite ? sprite->RealDirection() : 0;
}

int SPRITE::SetCommand(int command,SPRITE* new_goal)
{
    if(IsCommand(18) && command!=18)
        m_unknown50=0;
    SetGoal(new_goal);
    if(HaveFightLink() && Link())
        Link()->SetCommand(command,new_goal);
    if(command<16 && !Goal()) {
        m_flag&=~0x7cu;
        return 1;
    }
    m_flag=(m_flag&~0x7cu)|((static_cast<unsigned int>(command)&0x1fu)<<2);
    return 0;
}

// ZS1 target 0x00453080..0x004530D9.
int SPRITE::SetCommandWithoutLink(int command,SPRITE* new_goal)
{
    if(((m_flag>>2)&0x1fu)==0x12u && command!=0x12)
        m_unknown50=0;
    SetGoal(new_goal);
    if(command<0x10 && !m_goal) {
        m_flag&=0xffffff83u;
        return 1;
    }
    m_flag=(m_flag&0xffffff83u)|((static_cast<unsigned int>(command)&0x1fu)<<2);
    return 0;
}

namespace {
int& SpriteHpStorage(SPRITE* sprite)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(sprite)+0x68);
}
int& VidMaxHpForArmy(VID* vid,int army)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid)+0x3E0+4*(army&3));
}
int& VidDeathCounterForArmy(VID* vid,int army)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid)+0x3C0+4*(army&3));
}
VID*& VidBreakLinkTarget(VID* vid)
{
    return *reinterpret_cast<VID**>(reinterpret_cast<unsigned char*>(vid)+0x28C);
}
unsigned long& VidLastEntityTime(VID* vid)
{
    return *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(vid)+0x464);
}
}

int SPRITE::IsInUndo()
{
    return static_cast<int>((m_flag>>8)&1u);
}

int SPRITE::Hp()
{
    return SpriteHpStorage(this);
}

int VID::GetMaxHp(int army)
{
    return VidMaxHpForArmy(this,army);
}

void VID::IncreaseNoSprites(int army)
{
    VidLastEntityTime(this)=RealCurrentTime;
    ++m_entitiesNumber[army];
}

void VID::DecreaseNoSprites(int army)
{
    if (m_entitiesNumber[army])
        --m_entitiesNumber[army];
}

int SPRITE::MaxHp()
{
    return m_vid->GetMaxHp(Army());
}

// ZS1 target 0x00453480..0x004534CD.
int SPRITE::DestroyLink(const VID* destroyed)
{
    SPRITE* owner=this;
    while (owner->m_child) {
        if (owner->m_child->m_vid==destroyed) {
            SPRITE* child=owner->m_child;
            SPRITE* grandChild=child->m_child;
            owner->m_child=grandChild;
            if (grandChild)
                grandChild->m_parent=owner;
            child->m_child=0;
            child->m_parent=0;
            child->ScalarDeletingDestructor(1u);
            return 1;
        }
        owner=owner->m_child;
    }
    return 0;
}

// MapEditZS1.exe 0x00454020..0x0045412D.
void SPRITE::ChangeHp(int newHp)
{
    int& hp=SpriteHpStorage(this);
    if (newHp<=0 && m_vid->m_baseHp!=0) {
        if (!IsDying()) {
            ++VidDeathCounterForArmy(m_vid,Army());
            const int damage=hp-newHp;
            const int threshold=MaxHp()*3/2;
            if (damage>threshold &&
                (m_vid->m_aniChildVid[16]!=0 || m_vid->m_noAnimCadr[16]!=0)) {
                ChangeAnimation(16);
            }
            else {
                ChangeAnimation(15);
            }
        }
        return;
    }

    const int half=MaxHp()/2;
    if (newHp>half && hp<=half)
        DestroyLink(VidBreakLinkTarget(m_vid));

    if (newHp<=half && hp>half) {
        if (m_vid->m_noAnimCadr[13]!=0 && (m_ani==0 || m_ani==2)) {
            ChangeAnimation(13);
            hp=newHp;
            return;
        }
        CreateChildAndPlaySFX(13,0);
    }

    hp=newHp;
}

// Retail bitfield owner: army occupies m_flag bits 12..13 (mask 0x3000).
// Historical MapEdit owner: 0x0045E6F0..0x0045E7F7.
void SPRITE::ChangeArmy(int army)
{
    const int oldArmy=static_cast<int>((m_flag>>12)&3u);
    const int newArmy=army&3;
    m_flag=(m_flag&~0x3000u)|(static_cast<unsigned int>(newArmy)<<12);
    const int currentArmy=static_cast<int>((m_flag>>12)&3u);

    if(m_child)
        m_child->ChangeArmy(currentArmy);

    const int newMax=m_vid->m_maxHp[currentArmy];
    const int oldMax=m_vid->m_maxHp[oldArmy];
    if(newMax!=oldMax) {
        const int divisor=oldMax>1 ? oldMax : 1;
        const int scaled=((m_hp*256)/divisor*newMax)/256;
        ChangeHp(scaled);
    }

    if(m_vid->m_entitiesNumber[oldArmy])
        --m_vid->m_entitiesNumber[oldArmy];
    m_vid->m_lastEntityTime=RealCurrentTime;
    ++m_vid->m_entitiesNumber[currentArmy];
}

// ZS1 target 0x00417960..0x00417AC8: DWORD pointer token + nvid, versioned
// XYZ, DWORD serialized ANGLE (low byte retained), DWORD army, CreateSprite,
// relation insertion, then ChangeArmy only for a successfully created sprite.
SPRITE* MAP::LoadSprite(STREAM* stream,int version)
{
    int pointerToken;
    int nvid;
    float x;
    float y;
    float z;
    int directionRaw;
    unsigned char directionByte;
    int army;
    SPRITE* sprite=0;

    stream->Read(&pointerToken,4u);
    if (pointerToken==-1)
        return reinterpret_cast<SPRITE*>(-1);
    stream->Read(&nvid,4u);
    if (version>9) {
        stream->Read(&x,4u);
        stream->Read(&y,4u);
        stream->Read(&z,4u);
    } else {
        int coordinate;
        stream->Read(&coordinate,4u); x=static_cast<float>(coordinate);
        stream->Read(&coordinate,4u); y=static_cast<float>(coordinate);
        stream->Read(&coordinate,4u); z=static_cast<float>(coordinate);
    }
    // Retail reads the serialized direction DWORD in this owner, keeps only
    // its low byte, then materializes the by-value ANGLE through ANGLE(const ANGLE*).
    stream->Read(&directionRaw,4u);
    directionByte=static_cast<unsigned char>(directionRaw);
    stream->Read(&army,4u);

    if (nvid>=0 && nvid<m_noVid && VidSlot(nvid))
        sprite=CreateSprite(VidSlot(nvid),x,y,z,
                            ANGLE(reinterpret_cast<const ANGLE*>(&directionByte)),0);
    else if (::Error)
        MYERROR::Error(::Error,"MAP",3,"sprite, this vid not exist",static_cast<unsigned long>(nvid));

    m_relation.Insert(reinterpret_cast<SPRITE*>(pointerToken),sprite);
    if (sprite)
        sprite->ChangeArmy(army);
    return sprite;
}

void MENU::Error(TYPE_ERROR type,char* text,unsigned long err)
{
    MYERROR::Error(::Error,"MENU",static_cast<int>(type),text,err);
}

namespace {

// MapEdit.exe helper 0x0049909C (VC6 _ftol). MENU::Save consumes the low
// DWORD of the signed 64-bit truncation.  NaN/overflow produces the x87
// integer-indefinite value, whose low DWORD is zero.
inline int RetailFtolLow32ForMenu(float value)
{
    const double d=static_cast<double>(value);
    if (!(d>=-9223372036854775808.0 && d<9223372036854775808.0))
        return 0;
    const __int64 converted=static_cast<__int64>(d);
    return static_cast<int>(static_cast<unsigned int>(converted));
}

}

// MapEditZS1.exe 0x0044C8A0..0x0044CBAF. ZS1 MENU resource loader.
int MENU::Load(const STRING* name)
{
    RESOURCE file;
    if (file.OpenForRead(name,0x554E454Du)) {
        Error(E_OPEN,name->m_buf,0);
        return 1;
    }
    if (file.GoBegin(0x44414548u)) {
        Error(E_SECTION,const_cast<char*>("'HEAD'in menu"),0);
        return 1;
    }

    int version;
    int screenX;
    int screenY;
    int shiftX;
    int shiftY;
    file.Read(&version,4u);
    file.Read(&screenX,4u);
    file.Read(&screenY,4u);
    file.Read(&shiftX,4u);
    file.Read(&shiftY,4u);

    if (!file.GoNext(0x20525053u)) {
        for (;;) {
            SPRITE* spr=Map->LoadSprite(&file,version);
            if (spr==reinterpret_cast<SPRITE*>(-1))
                break;
            if (spr) {
                const float x=spr->X()-static_cast<float>(shiftX)-static_cast<float>(screenX/2)+Graph->SizeX()*0.5f;
                const float y=spr->Y()-static_cast<float>(shiftY)-static_cast<float>(screenY/2)+Graph->SizeY()*0.5f;
                spr->ChangeCoor(x,y,spr->Z());
                spr->Action(0x51,reinterpret_cast<long>(&file),version,0);
            }
            file.GoNextSub(0x20525053u);
        }
    } else if (!file.GoBegin(0x49525053u)) {
        for (;;) {
            SPRITE* spr=Map->LoadSprite(&file,version);
            if (spr==reinterpret_cast<SPRITE*>(-1))
                break;
            if (spr) {
                const float x=spr->X()-static_cast<float>(shiftX)-static_cast<float>(screenX/2)+Graph->SizeX()*0.5f;
                const float y=spr->Y()-static_cast<float>(shiftY)-static_cast<float>(screenY/2)+Graph->SizeY()*0.5f;
                spr->ChangeCoor(x,y,spr->Z());
            }
        }
    } else {
        Error(E_SECTION,const_cast<char*>("'SPR ' or 'SPRI' in menu"),0);
        return 1;
    }
    file.Close();
    return 0;
}

// MapEditZS1.exe 0x0044CBB0..0x0044CE8F. ZS1 menu-layer removal owner.
int MENU::DeleteFromFile(const STRING* name)
{
    RESOURCE file;
    if (file.OpenForRead(name,0x554E454Du)) {
        Error(E_OPEN,name->m_buf,0);
        return 1;
    }
    if (file.GoBegin(0x44414548u)) {
        Error(E_SECTION,const_cast<char*>("'HEAD'in menu"),0);
        return 1;
    }

    int version;
    int screenX;
    int screenY;
    int shiftX;
    int shiftY;
    file.Read(&version,4u);
    file.Read(&screenX,4u);
    file.Read(&screenY,4u);
    file.Read(&shiftX,4u);
    file.Read(&shiftY,4u);

    if (file.GoNext(0x20525053u)) {
        Error(E_SECTION,const_cast<char*>("'SPR ' in MENU::DeleteFromFile"),0);
        return 1;
    }

    for (;;) {
        int pointerToken;
        file.Read(&pointerToken,4u);
        if (pointerToken==-1)
            break;

        int nvid;
        float x;
        float y;
        float z;
        file.Read(&nvid,4u);
        file.Read(&x,4u);
        file.Read(&y,4u);
        file.Read(&z,4u);
        (void)z; // serialized by retail but not part of the DeleteFromFile match

        // ZS1 0x0044CC90..0x0044CDEE compares the serialized menu
        // position in SCREEN space, not raw world XYZ.  In particular Y is
        // SPRITE::ScreenY() == Y-Z-mapShiftY; there is no separate Z equality
        // test.  The older MapEdit owner compared world Y and Z independently,
        // so DeleteFromFile failed after viewport/map shifts and left stale menu
        // sprites behind for the next .men layer.
        const float targetX=x-static_cast<float>(shiftX)-static_cast<float>(screenX/2)+Graph->SizeX()*0.5f;
        const float targetY=y-static_cast<float>(shiftY)-static_cast<float>(screenY/2)+Graph->SizeY()*0.5f;

        for (int i=0;i<No();++i) {
            SPRITE* spr=*Item(i);
            if (spr->Vid()->m_idx!=nvid)
                continue;
            if (fabsf(spr->ScreenX()-targetX)>=0.001f)
                continue;
            if (fabsf(spr->ScreenY()-targetY)>=0.001f)
                continue;

            DeleteSpriteNumber(i);
            --i;
        }
        file.GoNextSub(0x20525053u);
    }

    file.Close();
    return 0;
}

// Retail MapEditZS1 MENU::Save. ZS1 menu HEAD version is 13 (AS1 used 12).
int MENU::Save(const STRING* name)
{
    RESOURCE file;
    int version=13;
    if (file.OpenForWrite(name,0x554E454Du)) {
        Error(E_CREATE,name->m_buf,0);
        return 1;
    }

    file.PreAppend(0x44414548u,0);
    file.Write(&version,4u);
    int screenX=RetailFtolLow32ForMenu(Graph->SizeX());
    file.Write(&screenX,4u);
    int screenY=RetailFtolLow32ForMenu(Graph->SizeY());
    file.Write(&screenY,4u);
    int shiftX=RetailFtolLow32ForMenu(Map->FromScreenX(0.0f));
    file.Write(&shiftX,4u);
    int shiftY=RetailFtolLow32ForMenu(Map->FromScreenY(0.0f));
    file.Write(&shiftY,4u);
    file.PostAppend();

    for (int i=0;i<No();++i) {
        SPRITE* spr=*Item(i);
        if (spr->IsLinked() || spr->Vid()==EmptyVid || spr->IsInUndo())
            continue;
        file.PreAppend(0x20525053u,0);
        spr->Save(&file);
        spr->Action(0x50,reinterpret_cast<long>(&file),0,0);
        file.PostAppend();
    }
    file.PreAppend(0x20525053u,0);
    int end=-1;
    file.Write(&end,4u);
    file.PostAppend();
    file.Close();
    return 0;
}

namespace {
template<class T>
T& EngineRaw(ENGINE* engine,unsigned int offset)
{
    return *reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(engine)+offset);
}
}

void ENGINE::SetCommandToTrain(int command,int x,int y)
{
    SetCommandToTrain(command,0,RailMap.GetNearestDot(x,y),0);
}

// ENGINE is deliberately represented as a raw facade in runtime.hpp: this body
// uses the exact retail field offsets instead of inventing a derived layout.
// rail-dot fields and fight-link command paths match the four-argument retail body.
void ENGINE::SetCommandToTrain(int command,SPRITE* target,R_DOT* dot_target,R_DOT* dot_target2)
{
    int suppressFightLinkCommand=0;
    if (command==30) {
        command=0;
        suppressFightLinkCommand=1;
    }

    if (!target && !dot_target && command!=29)
        command=0;

    if (command==24) {
        ENGINE* engine=FirstEngine();
        while (engine) {
            SPRITE* const sprite=reinterpret_cast<SPRITE*>(engine);
            if (sprite->Vid()->m_idx==0x55 && reinterpret_cast<UNIT*>(engine)->Ammo()>0)
                break;
            engine=engine->NextEngine();
        }
        if (!engine) {
            command=0;
            target=0;
            dot_target=0;
        }
    }

    R_DOT* pathDot=0;
    R_DOT* pathDot2=0;
    if (command==25) {
        pathDot=dot_target2 ? dot_target2 :
            RailMap.GetNearestDot(
                static_cast<int>(reinterpret_cast<SPRITE*>(this)->X()),
                static_cast<int>(reinterpret_cast<SPRITE*>(this)->Y()),
                static_cast<int>(reinterpret_cast<SPRITE*>(this)->Z()));
        pathDot2=dot_target;
    }

    ENGINE* engine=FirstEngine();
    while (engine) {
        SPRITE* const sprite=reinterpret_cast<SPRITE*>(engine);

        SPRITE*& commandOwner=EngineRaw<SPRITE*>(engine,0x9Cu);
        if (commandOwner) {
            commandOwner->Release();
            commandOwner=0;
        }

        if ((command==28 || command==29) && !IsCommandToAllTrain()) {
            commandOwner=reinterpret_cast<SPRITE*>(this);
            reinterpret_cast<SPRITE*>(this)->AddRef();
        }

        sprite->SetBestTarget(0);
        if (sprite->HaveFightLink())
            sprite->Link()->SetBestTarget(0);

        sprite->SetCommandWithoutLink(command,target);
        EngineRaw<int>(engine,0xB0u)=0;
        EngineRaw<R_DOT*>(engine,0xA0u)=dot_target;
        EngineRaw<R_DOT*>(engine,0xA4u)=pathDot;
        EngineRaw<R_DOT*>(engine,0xA8u)=pathDot2;
        EngineRaw<int>(engine,0xF0u)=0;
        EngineRaw<int>(engine,0xECu)=0;

        if (sprite->HaveFightLink()) {
            if (!suppressFightLinkCommand) {
                if (IsCommandToAllTrain() || engine==this) {
                    if (reinterpret_cast<SPRITE*>(this)->CanAttackThisSprite(target)) {
                        if (command==28) {
                            sprite->Link()->SetCommandWithoutLink(3,target);
                        } else if (command==29) {
                            if (sprite->AttackedSpriteType()==8u && target) {
                                SPRITE* const marker=new SPRITE(
                                    EmptyVid,
                                    target->X(),
                                    target->Y()+70.0f,
                                    target->Z()+70.0f,
                                    ANGLE(static_cast<uint8_t>(0)),
                                    0);
                                sprite->Link()->SetCommandWithoutLink(4,marker);
                            } else {
                                sprite->Link()->SetCommandWithoutLink(4,target);
                            }
                        } else {
                            sprite->Link()->SetCommandWithoutLink(0,static_cast<SPRITE*>(0));
                        }
                    } else {
                        sprite->Link()->SetCommandWithoutLink(0,static_cast<SPRITE*>(0));
                    }
                } else {
                    sprite->Link()->SetCommandWithoutLink(0,static_cast<SPRITE*>(0));
                }
            }
        }

        engine=engine->NextEngine();
    }

    if (command==23 || command==26 || command==27 || command==25) {
        reinterpret_cast<SPRITE*>(FirstEngine())->StartMove();
    } else if (command==24) {
        if (EngineRaw<R_DOT*>(this,0xB8u)==EngineRaw<R_DOT*>(this,0xA0u)) {
            FirstEngine()->Stop();
            ENGINE* item=FirstEngine();
            while (item) {
                SPRITE* const sprite=reinterpret_cast<SPRITE*>(item);
                if (sprite->Vid()->m_idx==0x55 && reinterpret_cast<UNIT*>(this)->Ammo()!=0)
                    EngineRaw<int>(item,0xECu)=1;
                item=item->NextEngine();
            }
        } else {
            reinterpret_cast<SPRITE*>(FirstEngine())->StartMove();
        }
    } else if (command==0) {
        FirstEngine()->Stop();
    }
}

// ZS1 target 0x0044A050..0x0044A080. ABI-neutral STRING-return wrapper for
// retail MAP::GetVariableStr; forwards into the embedded LOGIC owner.
STRING MAP::ScriptVariable(STRING name)
{
    return m_logic.GetVariableStr(&name);
}

VID* MAP::ReadVid(STREAM* res)
{
    int nvid;
    res->Read(&nvid,4);
    return ValidateVid(nvid) ? VidSlot(nvid) : 0;
}

void MAP::WriteVid(STREAM* res,const VID* vid)
{
    int empty=-1;
    res->Write(vid ? static_cast<const void*>(&vid->m_idx) : static_cast<const void*>(&empty),4);
}

int MAP::OptEnemyAttackNeutralTrains()
{
    return static_cast<int>((m_flags>>1)&1u);
}


// -----------------------------------------------------------------------------
// script_exec/map helper ring, recovered from MapEdit.exe 0x00456BA0..0x00456E1D.
// -----------------------------------------------------------------------------
// through 0x0041C1C0 and draws it at ScreenX/ScreenY in GRAPH::WHITE. Semantic name inferred.
void MAP::DrawSpriteNames()
{
    for (int i=0;i<m_zs1TailList.No();++i) {
        SPRITE* sprite=*m_zs1TailList[i];
        STRING name=sprite->Name();
        Graph->PutsXY(sprite->ScreenX(),sprite->ScreenY(),&name,GRAPH::WHITE);
    }
}

STRING MAP::FileName() { return m_mapName; }
PLAYER* MAP::Player() { return m_player[m_curArmy]; }

// Current MapEdit lineage: 0x0041B00C.
void MAP::PauseOn()
{
    if (!(m_flags&0x10u)) PauseOldClock=CurrentTime;
    m_flags|=0x10u;
}
// Current MapEdit lineage: 0x0041B042.
void MAP::PauseOff()
{
    if (m_flags&0x10u) {
        for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
            int index=0;
            for (SPRITE* spr=FirstSprite(layer,&index);spr;spr=NextSprite(layer,&index))
                spr->SetTime(PauseOldClock);
        }
        CurrentTime=PauseOldClock;
        PrevCurrentTime=PauseOldClock-10u;
    }
    m_flags&=~0x10u;
}

// ZS1 target 0x004181E0..0x00418264.
SPRITE* MAP::GetSprite(int type,float x,float y,SPRITE* prev)
{
    SPRITE* spr=FindNearestSprite(type,x,y,300.0f,prev);
    if (!spr) return 0;
    VID* vid=spr->Vid();
    if (!InSegment(x,spr->X(),vid->m_snapOffsetX)) return 0;
    if (!InSegment(y,spr->Y(),vid->m_snapOffsetY)) return 0;
    return spr;
}
