// Target MAP::ExecFunc keeps the two FOpen calls at 0x004481F5/0x00448297
// as calls to the shared mylib wrapper while other translation units fold it.
// This restores that original per-TU visibility boundary without compiler-specific no-inline attributes.
#define MAPEDIT_FOPEN_DECL_ONLY 1
#include "mapedit/runtime.hpp"
#undef MAPEDIT_FOPEN_DECL_ONLY
#include "mapedit/zs1/runtime_layouts.hpp"
#include "mapedit/zs1/script_exec.hpp"

static int g_execUnitType = 0;
static int g_execUnitIterator = 0;
static int g_execSpriteLayer = 0;
static int g_execSpriteIterator = 0;
static int g_trainArmy = 0;
static int g_trainOrdinal = 0;
static STRING g_execCommands;
static char* g_registrationInfo = STRING::EMPTY;

extern "C" void* __stdcall ShellExecuteA(HWND__*,const char*,const char*,const char*,const char*,int);

void __stdcall GetRegistrationInformation(char* newUserName)
{
    // Retail export 0x00444100 is a raw pointer assignment: no null fallback,
    // no STRING operation, and stdcall ret 4.
    g_registrationInfo = newUserName;
}

// Zombie Shooter 1 retail CountGammaByte helper (source name kept for ABI continuity).
unsigned char CountByteGamma(int b1,int b2,int time)
{
    if (b1>=0x80)
        b1-=0xFE;
    if (b2>=0x80)
        b2-=0xFE;
    return static_cast<unsigned char>(b1+(b2-b1)*time/255);
}

// Zombie Shooter 1 retail three-channel packed gamma interpolation.
int CountGamma(int g1,int g2,int time)
{
    const unsigned int a=static_cast<unsigned int>(g1);
    const unsigned int b=static_cast<unsigned int>(g2);
    unsigned int result=0;
    result|=static_cast<unsigned int>(CountByteGamma(a&0xFFu,b&0xFFu,time));
    result|=static_cast<unsigned int>(CountByteGamma((a>>8)&0xFFu,(b>>8)&0xFFu,time))<<8;
    result|=static_cast<unsigned int>(CountByteGamma((a>>16)&0xFFu,(b>>16)&0xFFu,time))<<16;
    return static_cast<int>(result);
}

ENGINE* GetTrainEng(int army,int ordinal,int* physicalOrdinal)
{
    int physical=0;
    int logical=0;
    int iterator;
    for (SPRITE* sprite=Hash->FirstUnit(&iterator);
         sprite;
         sprite=Hash->NextUnit(&iterator)) {
        if (!sprite->IsSpriteClass(0x15))
            continue;
        ENGINE* engine=static_cast<ENGINE*>(sprite);
        if (!engine->IsFirst())
            continue;

        if (army==4 || engine->Army()==army) {
            ++physical;
            ++logical;
            if (logical==ordinal) {
                *physicalOrdinal=physical;
                ENGINE* train=engine->GetTrain();
                if (train && train->Army()!=engine->Army()) {
                    --logical;
                    continue;
                }
                return train ? train : engine;
            }
        } else {
            ++physical;
        }
    }
    *physicalOrdinal=0;
    return 0;
}

// ZS1 target uses the same reverse unit iteration / first-engine semantics;
// editor ENGINE field shifts stay encapsulated by IsFirst()/GetTrain().
ENGINE* FirstTrain(int army)
{
    if (army<0)
        return 0;
    if (army>=4)
        army=3;
    g_trainArmy=army;
    g_trainOrdinal=0;
    ++g_trainOrdinal;
    int physical=0;
    return GetTrainEng(g_trainArmy,g_trainOrdinal,&physical);
}

ENGINE* NextTrain()
{
    ++g_trainOrdinal;
    int physical=0;
    return GetTrainEng(g_trainArmy,g_trainOrdinal,&physical);
}

namespace {
void SetVidChild(VID* vid,int slot,int value)
{
    if (!vid || slot<0 || slot>=17) return;
    if (!value) {
        vid->m_aniSpawnMode[slot]=0;
        vid->m_aniChildVid[slot]=0;
        return;
    }
    int idx=abs(value);
    if (!Map->ValidateVid(idx)) return;
    vid->m_aniSpawnMode[slot]=value;
    vid->m_aniChildVid[slot]=Map->Vid(idx);
    if (vid->m_aniChildVid[slot] && vid->m_aniChildVid[slot]->IsLightType())
        vid->m_exSpriteData|=1;
}
}

int ScriptExecFunc(int command)
{
    return Map->ExecFunc(command);
}

int MAP::ExecFunc(int command)
{
    static STRING execText;

    switch (command) {
    case 65: {
        SPRITE* parent=PopSprite(); int dir=PopInt(),z=PopInt(),y=PopInt(),x=PopInt(); VID* v=PopVid("for CreateSprite()");
        STRING context("return CreateSprite");
        m_logic.PushObject(v==EmptyVid ? 0 : CreateSprite(v,(float)x,(float)y,(float)z,ANGLE((uint8_t)dir),parent),&context);
        return 0;
    }
    case 66: { STRING context("return Flagman()"); m_logic.PushObject(Flagman(PopInt()),&context); return 0; }
    case 68: {
        g_execUnitType=PopInt(); SPRITE* s=Hash->units.BeginIterate(&g_execUnitIterator);
        while (s && !(g_execUnitType & (int)s->m_vid->m_unknown0C)) s=Hash->units.NextIterate(&g_execUnitIterator);
        STRING context("return FirstUnit()");
        m_logic.PushObject(s,&context); return 0;
    }
    case 69: {
        SPRITE* s=Hash->units.NextIterate(&g_execUnitIterator);
        while (s && !(g_execUnitType & (int)s->m_vid->m_unknown0C)) s=Hash->units.NextIterate(&g_execUnitIterator);
        STRING context("return NextUnit()");
        m_logic.PushObject(s,&context); return 0;
    }
    case 70: { SPRITE* prev=PopSprite(); int y=PopInt(),x=PopInt(),type=PopInt(); STRING context("return GetSprite()"); m_logic.PushObject(GetSprite(type,(float)x,(float)y,prev),&context); return 0; }
    case 71: { int y=PopInt(),x=PopInt(),type=PopInt(); STRING context("return GetSpriteScr()"); m_logic.PushObject(GetSpriteScr(type,(float)x,(float)y),&context); return 0; }
    case 72: { SPRITE* prev=PopSprite(); int radius=PopInt(),y=PopInt(),x=PopInt(),type=PopInt(); STRING context("return FindNearestSprite()"); m_logic.PushObject(FindNearestSprite(type,(float)x,(float)y,(float)radius,prev),&context); return 0; }
    case 74: { int bottom=PopInt(),right=PopInt(),top=PopInt(),left=PopInt(); STRING context("return FirstInBox()"); m_logic.PushObject(Hash->FirstInBox((float)left,(float)top,(float)right,(float)bottom),&context); return 0; }
    case 75: { STRING context("return NextInBox()"); m_logic.PushObject(Hash->NextInBox(),&context); return 0; }
    case 76: { g_execSpriteLayer=0; g_execSpriteIterator=m_layers[0].m_no; STRING context("return FirstSprite()"); m_logic.PushObject(NextSprite(0,&g_execSpriteIterator),&context); return 0; }
    case 77: {
        SPRITE* s=NextSprite(g_execSpriteLayer,&g_execSpriteIterator);
        while (!s && g_execSpriteLayer<16) { ++g_execSpriteLayer; g_execSpriteIterator=m_layers[g_execSpriteLayer].m_no; s=NextSprite(g_execSpriteLayer,&g_execSpriteIterator); }
        STRING context("return NextSprite()");
        m_logic.PushObject(s,&context); return 0;
    }
    case 78: { // ZS1 0x00444A3D -- ActionByName(name,var1,var2,var3,var4)
        const int var4=PopInt();
        const int var3=PopInt();
        const int var2=PopInt();
        const int var1=PopInt();
        const STRING groupName=*PopStr();
        for(int i=m_zs1TailList.m_no-1;i>=0;--i) {
            SPRITE* sprite=m_zs1TailList.m_data[i];
            if(!sprite) continue;
            STRING name=sprite->m_exData ? sprite->m_exData->name : STRING("");
            if(name==&groupName) sprite->Action(var1,var2,var3,var4);
        }
        return 0;
    }
    case 79: { // ZS1 0x00444C0E -- Action(unit,action,var1,var2,var3)
        int v3=PopInt(),v2=PopInt(),v1=PopInt(),action=PopInt(); SPRITE* spr=PopSprite();
        if(!spr){PushInt(0);return 0;}
        if(action<0x11){spr->ChangeAnimation(action);PushInt(0);return 0;}
        if(action==0x79 || action==0x7D || action==0x7C) {
            const STRING* result=reinterpret_cast<const STRING*>(spr->Action(action,v1,v2,v3));
            if(result) PushStr(result); else { STRING empty(""); PushStr(&empty); }
            return 0;
        }
        if(action==0x5A || action==0x9C || action==0x9B || action==0x9A || action==0x65 || action==0x67) {
            STRING context("return Action()");
            m_logic.PushObject(reinterpret_cast<const void*>(spr->Action(action,v1,v2,v3)),&context);
            return 0;
        }
        if((action==0x21 || action==0x20 || action==0x24 || action==0x22 || action==0x96 || action==0x97) &&
           spr->Vid()->m_spriteClass==0x15 && (spr->m_flag&0x7C)==0x68) {
            PushInt(0);
            return 0;
        }
        PushInt(spr->Action(action,v1,v2,v3));
        return 0;
    }
    case 80: { int y=PopInt(),x=PopInt(); SPRITE* s=PopSprite(); PushInt(s ? (int)s->NearDistanceTo((float)x-s->X(),(float)y-s->Y()) : 60000); return 0; }
    case 82: { int v3=PopInt(),v2=PopInt(),v1=PopInt(),a=PopInt(); SPRITE* s=PopSprite(); if(s)s->AddActionAfterStop(a,v1,v2,v3); return 0; }
    case 83: { SPRITE* s=PopSprite(); PushInt(s&&s->m_vid?s->m_vid->m_idx:0); return 0; }
    case 84: { SPRITE* s=PopSprite(); if(s)s->ScalarDeletingDestructor(1); return 0; }
    case 85: { SPRITE* s=PopSprite(); PushInt(s?(int)s->X():0); return 0; }
    case 86: { SPRITE* s=PopSprite(); PushInt(s?(int)s->Y():0); return 0; }
    case 87: { SPRITE* s=PopSprite(); PushInt(s?(int)s->Z():0); return 0; }
    case 88: { SPRITE* s=PopSprite(); PushInt(s?s->Direction().Int():0); return 0; }
    case 89: { SPRITE* s=PopSprite(); PushInt(s?s->Animation():0); return 0; }
    case 90: { int y=PopInt(),x=PopInt(); SPRITE* s=PopSprite(); PushInt(s?s->DirectionTo((float)x,(float)y).Int():0); return 0; }
    case 96: {
        SPRITE* s=PopSprite(); STRING out("");
        if(s){ STRING a=s->GetTextActions(); STRING i=s->GetTextItems(); out=i+a; }
        PushStr(&out); return 0;
    }
    case 97: {
        const STRING* text=PopStr(); g_execCommands=text?*text:STRING(""); SPRITE* s=PopSprite();
        if(s){ s->SetTextItems(&g_execCommands); STRING actions=g_execCommands; if(actions.HaveSubStr("\2")) actions=actions.After("\2"); s->SetTextActions(&actions); }
        return 0;
    }
    case 98: LoadInEndTact(PopStr()); return 0;
    case 99: Save(*PopStr()); return 0;
    case 100: { if(!m_resource.IsOpen())m_resource.OpenForWrite(PopStr(),0x4F4D4544u); return 0; }
    // ZS1 0x0044545E..0x004455CA: MenuFind.  Besides the normal
    // VID::RealDirection match, retail accepts 999999 as "any direction" and
    // 999000+raw ANGLE as the byte-direction selector used by menu scripts.
    case 101: {
        int ndir=PopInt(); VID* v=PopVid("for MenuFind"); SPRITE* found=0;
        if(v!=EmptyVid && v->NoSprites()!=0){
            for(int i=0;i<m_menu.m_no;++i){
                SPRITE* s=m_menu.m_data[i];
                if(!s || s->m_vid!=v)
                    continue;
                if(ndir==999999 || ndir==v->RealDirection(s->Direction()) ||
                   ndir==999000+static_cast<int>(s->Direction().value)){
                    found=s;
                    break;
                }
            }
        }
        STRING context("return MenuFind()");
        m_logic.PushObject(found,&context); return 0;
    }
    // ZS1 0x004455CF: MenuLoad pops one STRING and calls MENU::Load directly.
    case 102: { m_menu.Load(PopStr()); return 0; }
    // ZS1 0x0044560A..0x004456A3: MenuRelease copies the popped name to
    // the shared command STRING.  Empty name means release the entire menu;
    // a non-empty name removes only sprites serialized by that .men file.
    case 103: {
        g_execCommands=PopStr();
        if(g_execCommands==STRING::EMPTY)
            m_menu.DeleteAll();
        else
            m_menu.DeleteFromFile(&g_execCommands);
        return 0;
    }
    case 104: PushInt(m_menu.NVidUnderCursor()); return 0;
    case 105: PushInt(m_menu.NDirUnderCursor()); return 0;
    // ZS1 0x004456F2..0x004458C3: MenuAction uses the same real/raw
    // direction selector contract as MenuFind.
    case 106: {
        int v3=PopInt(),v2=PopInt(),v1=PopInt(),a=PopInt(),ndir=PopInt(); VID* v=PopVid("for MenuAction");
        if(v!=EmptyVid){
            for(int i=0;i<m_menu.m_no;++i){
                SPRITE* s=m_menu.m_data[i];
                if(!s || s->m_vid!=v)
                    continue;
                if(ndir!=999999 && ndir!=v->RealDirection(s->Direction()) &&
                   ndir!=999000+static_cast<int>(s->Direction().value))
                    continue;
                if(a<17)
                    s->ChangeAnimation(a);
                else
                    s->Action(a,v1,v2,v3);
            }
        }
        return 0;
    }
    case 107: { int z=PopInt(),y=PopInt(),x=PopInt(),ndir=PopInt(); VID* v=PopVid("for MenuCreate"); STRING context("return MenuCreate()"); if(v==EmptyVid){m_logic.PushObject(0,&context);return 0;} int d=ndir*256/(int)v->m_noDirections; m_logic.PushObject(CreateSprite(v,(float)x,(float)(y+z),(float)z,ANGLE((uint8_t)d),0),&context); return 0; }
    case 108: { STRING context("return MenuLClick()"); m_logic.PushObject((m_menu.clickFlags&1)?m_menu.sprite:0,&context); return 0; }
    case 109: PushInt((int)m_input.screenMouseX); return 0;
    case 110: PushInt((int)m_input.screenMouseY); return 0;
    case 111: PushInt((int)m_input.key); return 0;
    case 112: { int on=PopInt(); if(on){PauseOn();Mouse->ChangeAnimation(0);}else PauseOff(); return 0; }
    case 113: { int type=PopInt(); if(type==-1)Mouse->Disable(); else if(type==256)Mouse->HardwareOn(); else if(type==257)Mouse->HardwareOff(); else {if(!Mouse->IsHardware())Mouse->Enable(); Mouse->ChangeAnimation(type);} return 0; }
    case 114: { int y=PopInt(),x=PopInt(); Player()->PutMessage(PopStr(),(float)x,(float)y); return 0; }
    case 115: PushInt((int)m_input.GetState()); return 0;
    case 116: { int y=PopInt(),x=PopInt(); SetShiftCoor((float)x,(float)y,0); return 0; }
    case 117: m_shiftFlag=(uint32_t)PopInt(); return 0;
    case 118: PushInt((int)m_shiftFlag); return 0;
    case 119: PushInt((int)Graph->SizeX()); return 0;
    case 120: PushInt((int)Graph->SizeY()); return 0;
    case 121: { int on=PopInt(); m_flags=(m_flags&~0x80u)|(on?0x80u:0u); return 0; }
    case 122: { int on=PopInt(); if(on)Player()->StateBarOn();else Player()->StateBarOff(); return 0; }
    case 123: { STRING key=*PopStr(); STRING section=*PopStr(); STRING def(""); STRING out=Profile->GetString(&section,&key,&def); PushStr(&out); return 0; }
    case 124: PopStr(); PostMessageA(m_hWnd,0x10,0,0); return 0;
    case 125: PushInt((int)ToScreenX((float)PopInt())); return 0;
    case 126: { int z=PopInt(),y=PopInt(); PushInt((int)ToScreenY((float)y,(float)z)); return 0; }
    case 127: { STRING context("return MenuRClick()"); m_logic.PushObject((m_menu.clickFlags&2)?m_menu.sprite:0,&context); return 0; }
    case 128: { int key=PopInt(),which=PopInt(); if(which==0x400){g_inputFirstPrimary=key;g_inputFirstSecondary=key;}else if(which==0x800){g_inputSecondPrimary=key;g_inputSecondSecondary=key;} return 0; }
    case 129: { int v1=PopInt(),v2=PopInt(); Mouse->Action(0x3F,v2,v1,0); return 0; }
    case 130: Sound->VolumeSound(PopInt()); return 0;
    case 131: Sound->VolumeMusic(PopInt()); return 0;
    case 132: Sound->PlaySFX(PopInt(),0,0); return 0;
    case 133: Sound->StopSFX(PopInt()); return 0;
    case 134: Sound->StopMusic(0); return 0;
    case 135: { int y=PopInt(),x=PopInt(),sfx=PopInt(); Sound->PlaySFXFromCoor(sfx,(float)x-m_shiftX-Graph->SizeX()*0.5f,(float)y-m_shiftY-Graph->SizeY()*0.5f); return 0; }
    case 136: { int loop=PopInt(); Sound->FadeAndPlayFile(*PopStr(),loop,3000); return 0; }
    case 137: { int duration=PopInt(),v2=PopInt(),v1=PopInt(),eff=PopInt(); Graph->Effect(eff,v1,v2,duration); return 0; }
    case 138: Graph->SetEnvironment((unsigned int)PopInt()); return 0;
    case 139: {
        // Target keeps this pop in ExecFunc; unlike case 247 it does not call MAP::PopInt.
        LOGICSTACK* value=&m_logic.stack.m_data[--m_logic.stack.m_no];
        value->Int();
        return 0;
    }
    case 140: { unsigned int val=(unsigned int)PopInt(); GAMMA gamma(GAMMA::DECODE,val); Graph->SetGamma(&gamma); return 0; }
    case 141: { int angle=PopInt(),force=PopInt(); Graph->SetWind(force,ANGLE((uint8_t)angle)); return 0; }
    case 142: Graph->PlayMovie(PopStr()); return 0;
    case 143: PushInt(Graph->IsPlayMovie()); return 0;
    case 144: Graph->StopMovie(); return 0;
    case 145: PushInt(Sound->IsPlayMusic()); return 0;
    case 146: { int time=PopInt(),g2=PopInt(),g1=PopInt(); PushInt(CountGamma(g1,g2,time)); return 0; }
    case 147: { GAMMA g=Graph->GetGamma(); PushInt((int)g.EncodeToDword()); return 0; }
    case 148: PushInt(Graph->GetEffectState(PopInt())); return 0;
    case 149: PushStr(&m_prevMap); return 0;
    case 150: { STRING s(g_registrationInfo); PushStr(&s); return 0; }
    case 151: PushStr(&m_mapName); return 0;
    case 152: { // ZS1 0x00446A13 -- Exec(commandLine)
        const STRING* ps=PopStr();
        if (ps) {
            STRING cmd=*ps;
            MYERROR::Log(::Error,"Exec '%s'",cmd.CharPtr());
            STRING params=cmd.After(" ");
            STRING file=cmd.Before(" ");
            ShellExecuteA(0,0,file.CharPtr(),params.CharPtr(),0,5);
        }
        return 0;
    }
    case 153: { int index=PopInt(); const STRING* s=PopStr(); PushInt((signed char)(*const_cast<STRING*>(s))[index]); return 0; }
    case 154: { const STRING* s=PopStr(); if(s)MYERROR::Log(::Error,"%s",s->m_buf); return 0; }
    case 155: PushInt(Random(PopInt())); return 0;
    case 156: { int z=PopInt(); VID* v=PopVid("for ChangeZUnit"); if(v!=EmptyVid){int it=m_layers[v->m_layer].m_no;for(SPRITE* s=NextSprite(v->m_layer,&it);s;s=NextSprite(v->m_layer,&it))if(s->m_vid==v)s->ChangeCoor(s->X(),s->Y(),(float)z);} return 0; }
    case 157: PushInt((int)CurrentTime); return 0;
    case 158: { int y=PopInt(),x=PopInt(); PushInt((int)GetGroundZ((float)x,(float)y)); return 0; }
    case 159: case 205: PushInt(const_cast<STRING*>(PopStr())->Length()); return 0;
    case 160: { SPRITE* s=PopSprite(); SetFlagman(PopInt(),s); return 0; }
    case 161: { int z=PopInt(),y=PopInt(),x=PopInt(); VID* v=PopVid("for CanPlace"); STRING context("return CanPlace()"); m_logic.PushObject(Hash->CanPlace(v,(float)x,(float)y,(float)z),&context); return 0; }
    case 162: { // ZS1 0x00446F23 -- GetVidData
        const int type=PopInt();
        VID* vid=PopVid(type==245 ? "" : "for GetVid");
        if(vid==EmptyVid){PushInt(0);return 0;}
        zs1::video::VID_LAYOUT32* layout=reinterpret_cast<zs1::video::VID_LAYOUT32*>(vid);
        zs1::video::VID_EXDATA_PREFIX32* ex=reinterpret_cast<zs1::video::VID_EXDATA_PREFIX32*>(layout->exData);
        switch(type) {
        case 1: PushInt(layout->defaultMaxHp); return 0;
        case 22: PushInt((int)layout->flag); return 0;
        case 23: PushInt((int)ex->scriptValue1C); return 0;
        case 26: {
            VID* weapon=layout->childLinkVid ? reinterpret_cast<VID*>((unsigned long)layout->childLinkVid) : vid;
            if(!weapon || !weapon->CanFight()) weapon=vid;
            zs1::video::VID_LAYOUT32* wl=reinterpret_cast<zs1::video::VID_LAYOUT32*>(weapon);
            zs1::video::VID_EXDATA_PREFIX32* wx=reinterpret_cast<zs1::video::VID_EXDATA_PREFIX32*>(wl->exData);
            PushInt(wx ? wx->maxAmmo : 0); return 0;
        }
        case 27: { STRING value=vid->m_name; PushStr(&value); return 0; }
        case 28: PushInt(layout->entityCount[0]+layout->entityCount[1]+layout->entityCount[2]+layout->entityCount[3]); return 0;
        case 29: PushInt(layout->deaths[0]+layout->deaths[1]+layout->deaths[2]+layout->deaths[3]); return 0;
        case 30: case 31: case 32: case 33: PushInt(layout->deaths[type-30]); return 0;
        case 34: case 35: case 36: case 37: PushInt(layout->entityCount[type-34]); return 0;
        case 38: case 39: case 40: case 41: PushInt(layout->maxHp[type-38]); return 0;
        case 46: PushInt((int)layout->collisionClassFlags); return 0;
        case 47: PushInt(layout->spriteClass); return 0;
        case 48: { const uint32_t raw=*reinterpret_cast<uint32_t*>(&layout->moveSpeed); PushInt(raw==0x497423F0u?999999:(int)(layout->moveSpeed*1000.0f)); return 0; }
        case 49: PushInt(layout->lifeTime); return 0;
        case 50: PushInt((int)ex->scriptValue18); return 0;
        case 51: PushInt((int)ex->scriptValue20); return 0;
        case 52: { const zs1::video::VID_LAYOUT32* related=reinterpret_cast<const zs1::video::VID_LAYOUT32*>((unsigned long)layout->scriptRelatedVid470); PushInt(related->index); return 0; }
        case 53: PushInt(layout->noDirectionMode); return 0;
        case 54: PushInt((int)layout->collisionMask); return 0;
        case 55: PushInt(ex->buildTime); return 0;
        case 56: PushInt((layout->propertyFlags488>>6)&1); return 0;
        case 57: PushInt(layout->notCreateAsChild); return 0;
        case 58: PushInt((int)layout->frameSpeed); return 0;
        case 59: { const zs1::video::VID_LAYOUT32* linked=reinterpret_cast<const zs1::video::VID_LAYOUT32*>((unsigned long)layout->childLinkVid); PushInt(linked?linked->index:0); return 0; }
        case 124: PushInt(layout->fireDamage); return 0;
        case 125: PushInt(layout->recolors[0]+layout->recolors[1]+layout->recolors[2]+layout->recolors[3]); return 0;
        case 126: case 127: case 128: case 129: PushInt(layout->recolors[type-126]); return 0;
        case 130: PushInt(ex->scriptValue2C); return 0;
        case 134: PushInt((int)layout->scriptValue4C); return 0;
        case 239: PushInt((int)layout->footprintWidth); return 0;
        case 240: PushInt((int)layout->footprintHeight); return 0;
        case 241: PushInt((int)layout->footprintZ); return 0;
        case 242: PushInt((int)(layout->scriptFloat2E8*1000.0f)); return 0;
        case 243: PushInt((int)(layout->scriptFloat2EC*1000.0f)); return 0;
        case 244: PushInt((int)(layout->scriptFloat2F0*1000.0f)); return 0;
        case 245: PushInt(1); return 0;
        default:
            if(type>=60 && type<=76) { const int slot=type-60; const uint32_t* table=reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(layout)+0x258); const zs1::video::VID_LAYOUT32* child=reinterpret_cast<const zs1::video::VID_LAYOUT32*>((unsigned long)table[slot]); PushInt(child?child->index:0); return 0; }
            if(type>=92 && type<=108) { PushInt(layout->noChild[type-92]); return 0; }
            Error(14,(char*)"GetVid type",(unsigned long)type); return 0;
        }
    }
    case 163: { // ZS1 0x004476FC -- SetVidData
        const int value=PopInt();
        const int type=PopInt();
        VID* vid=PopVid("for SetVid");
        if(vid==EmptyVid) return 0;
        zs1::video::VID_LAYOUT32* layout=reinterpret_cast<zs1::video::VID_LAYOUT32*>(vid);
        zs1::video::VID_EXDATA_PREFIX32* ex=reinterpret_cast<zs1::video::VID_EXDATA_PREFIX32*>(layout->exData);
        switch(type) {
        case 1: layout->defaultMaxHp=value; return 0;
        case 23: ex->scriptValue1C=(float)value; return 0;
        case 26: { VID* weapon=layout->childLinkVid?reinterpret_cast<VID*>((unsigned long)layout->childLinkVid):vid; if(!weapon||!weapon->CanFight())weapon=vid; zs1::video::VID_LAYOUT32* wl=reinterpret_cast<zs1::video::VID_LAYOUT32*>(weapon); zs1::video::VID_EXDATA_PREFIX32* wx=reinterpret_cast<zs1::video::VID_EXDATA_PREFIX32*>(wl->exData); if(wx)wx->maxAmmo=value; return 0; }
        case 29: for(int i=0;i<4;++i) layout->deaths[i]=value; return 0;
        case 30: case 31: case 32: case 33: layout->deaths[type-30]=value; return 0;
        case 38: case 39: case 40: case 41: vid->SetMaxHp(type-38,value); return 0;
        case 42: case 43: case 44: case 45: vid->SetHpCoeff(type-42,value); return 0;
        case 48: { const float speed=value==999999?999999.0f:(float)value*0.001f; layout->moveSpeed=speed; layout->moveSpeedMirror=speed; int it; for(SPRITE* spr=FirstSprite(layout->layer,&it);spr;spr=NextSprite(layout->layer,&it)) if(spr->Vid()==vid && spr->ExData()) spr->ExData()->moveSpeed=speed; return 0; }
        case 49: if(layout->index!=0){layout->lifeTime=value;layout->runtimeFlags484=1;} return 0;
        case 50: ex->scriptValue18=(float)value; return 0;
        case 51: ex->scriptValue20=(float)value; return 0;
        case 52: if(ValidateVid(value)) ExchangeVid(vid,Vid(value)); else Error(4,(char*)"SetVid get_image",(unsigned long)value); return 0;
        case 54: layout->collisionMask=(uint32_t)value; return 0;
        case 55: ex->buildTime=value; return 0;
        case 56: vid->SetPropHide(value); return 0;
        case 57: layout->notCreateAsChild=value; return 0;
        case 58: layout->frameSpeed=(uint16_t)value; for(int i=0;i<17;++i)layout->animationDuration[i]=value; return 0;
        case 59: layout->childLinkVid=(uint32_t)Vid(value); return 0;
        case 124: layout->fireDamage=value; return 0;
        case 125: vid->SetScriptPackedValue125(value); return 0;
        case 130: ex->scriptValue2C=value; return 0;
        case 134: layout->scriptValue4C=(float)value; return 0;
        case 239: layout->footprintWidth=(float)value; return 0;
        case 240: layout->footprintHeight=(float)value; return 0;
        case 241: layout->footprintZ=(float)value; return 0;
        case 242: layout->scriptFloat2E8=(float)value*0.001f; return 0;
        case 243: layout->scriptFloat2EC=(float)value*0.001f; return 0;
        case 244: layout->scriptFloat2F0=(float)value*0.001f; return 0;
        default:
            if(type>=60 && type<=76) { const int slot=type-60; int* ids=reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(layout)+0x214); uint32_t* table=reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(layout)+0x258); if(value==0){ids[slot]=0;table[slot]=0;return 0;} const int idx=value<0?-value:value; if(!ValidateVid(idx)){Error(4,(char*)"SetVid child",(unsigned long)value);return 0;} ids[slot]=value; VID* child=Vid(idx); table[slot]=(uint32_t)child; if(child->IsLightType()) layout->runtimeFlags484|=1; return 0; }
            if(type>=18 && type<=21) { GAMMA gamma(GAMMA::DECODE,(unsigned)value); vid->SetGamma(&gamma,(unsigned)(type-18)); return 0; }
            if(type>=92 && type<=108) { layout->noChild[type-92]=value; return 0; }
            Error(14,(char*)"SetVid type",(unsigned long)type); return 0;
        }
    }
    case 164: { STRING s=Int2Str(PopInt()); PushStr(&s); return 0; }
    case 165: { ANGLE a((uint8_t)PopInt()); PushInt((int)(a.Sin()*1024.0f)); return 0; }
    case 166: { ANGLE a((uint8_t)PopInt()); PushInt((int)(a.Cos()*1024.0f)); return 0; }
    case 167: PushInt((int)m_w); return 0;
    case 168: PushInt((int)m_h); return 0;
    case 169: { VID* v=PopVid("for Genocide"); if(v!=EmptyVid){int it=m_layers[v->m_layer].m_no;for(SPRITE* s=NextSprite(v->m_layer,&it);s;){SPRITE* n=NextSprite(v->m_layer,&it);if(s->m_vid==v)s->ScalarDeletingDestructor(1);s=n;}} return 0; }
    case 170: { VID* nv=PopVid("for Replace Unit 2"); VID* ov=PopVid("for Replace Unit 1"); if(nv!=EmptyVid&&ov!=EmptyVid){int it=m_layers[ov->m_layer].m_no;for(SPRITE* s=NextSprite(ov->m_layer,&it);s;){SPRITE* n=NextSprite(ov->m_layer,&it);if(s->m_vid==ov){CreateSprite(nv,s->X(),s->Y(),s->Z(),s->Direction(),0);s->ScalarDeletingDestructor(1);}s=n;}} return 0; }
    case 171: {
        execText=PopStr();
        CRC32 crc(execText.CharPtr(),static_cast<unsigned int>(execText.Length()));
        PushInt(static_cast<int>(static_cast<unsigned int>(crc)));
        return 0;
    }
    case 172: {
        if(m_logic.IsLastStackString()) {
            STRING arg(PopStr());
            const STRING* format=PopStr();
            STRING out=Printf(const_cast<STRING*>(format)->CharPtr(),arg.CharPtr());
            PushStr(&out);
        } else {
            const int arg=PopInt();
            const STRING* format=PopStr();
            STRING out=Printf(const_cast<STRING*>(format)->CharPtr(),arg);
            PushStr(&out);
        }
        return 0;
    }
    case 173: ReloadVid(); return 0;
    case 174: {
        execText=PopStr();
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        if(!file) return 0;
        execText.Write(file);
        fseek(file,-1,1);
        fputs("\n",file);
        return 0;
    }
    case 175: {
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        if(m_flags&0x200u)
            execText.Read(&m_resource);
        else if(file)
            execText.Read(file);
        if(m_flags&0x100u)
            execText.Write(&m_resource);
        PushStr(&execText);
        return 0;
    }
    case 176: {
        execText=PopStr();
        FILE* file=(m_flags&0x200u) ? 0 : FOpen(&execText,"r+t");
        if(!file)
            Error(7,execText.CharPtr(),0);
        PushInt(reinterpret_cast<int>(file));
        return 0;
    }
    case 177: {
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        if(file) fclose(file);
        return 0;
    }
    case 178: {
        execText=PopStr();
        if(m_flags&0x200u)
            PushInt(0);
        else
            PushInt(reinterpret_cast<int>(FOpen(&execText,"w+t")));
        return 0;
    }
    case 179: {
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        PushInt(file ? (feof(file) ? 0x10 : 0) : 1);
        return 0;
    }
    case 182: { STRING def=*PopStr(),name=*PopStr(),path=*PopStr(); REGISTRY r(path); STRING out=r.GetString(name,def); PushStr(&out); return 0; }
    case 183: { STRING value=*PopStr(),name=*PopStr(),path=*PopStr(); REGISTRY r(path); r.SetString(name,value); return 0; }
    case 184: { STRING name=*PopStr(),path=*PopStr(); REGISTRY r(path); r.Delete(name); return 0; }
    case 185: PushStr(Registry->Path()); return 0;
    case 206: { STRING o=const_cast<STRING*>(PopStr())->ToLower(); PushStr(&o); return 0; }
    case 207: { STRING o=const_cast<STRING*>(PopStr())->ToUpper(); PushStr(&o); return 0; }
    case 208: { int key=PopInt(); STRING o=const_cast<STRING*>(PopStr())->ToBase64(key); PushStr(&o); return 0; }
    case 212: { // 0xD4 -- ZS1 TrainInfo(engine,query), target 0x00448531
        const int query=PopInt();
        ENGINE* engine=reinterpret_cast<ENGINE*>(PopInt()); // retail deliberately uses PopInt, not PopObject
        if(!engine || !engine->IsSpriteClass(0x15)) {
            PushInt(0);
            return 0;
        }
        TRAIN_INFO info(engine);
        switch(query) {
        case 1: PushInt(info.speed); return 0;
        case 2: PushInt(info.weapon); return 0;
        case 3: PushInt(info.hp*100/info.max_hp); return 0;
        case 4: PushInt(info.hp); return 0;
        case 5: PushInt(info.percentAmmo); return 0;
        case 6: PushInt(info.Acceleration()); return 0;
        case 7: PushInt(info.build_time); return 0;
        case 8: PushInt(0); return 0;
        case 9:
            // Exact retail stack behavior: every non-idle engine pushes 0,
            // then a final 1 is pushed unconditionally.
            for(ENGINE* e=engine->FirstEngine();e;e=e->NextEngine())
                if(!e->IsCommand(0))
                    PushInt(0);
            PushInt(1);
            return 0;
        case 10: PushInt(info.ammo); return 0;
        case 11: PushInt(info.maxAmmo); return 0;
        default: PushInt(0); return 0;
        }
    }
    case 213: { // 0xD5 -- ZS1 GetUnitInMap(), target 0x004486FF
        int monsterMask[0x1000]={0};
        for (int i=0;i<0x1000;++i) {
            STRING variable=STRING("MonstersVid[")+Int2Str(i)+"]";
            STRING value=m_logic.GetVariableStr(&variable);
            if (value=="")
                break;
            // Retail indexes the fixed 4096-DWORD table without a bounds check.
            monsterMask[value.Int()]=1;
        }

        int count=0;
        for (int layer=0;layer<20;++layer) {
            int iterator=0;
            for (SPRITE* sprite=FirstSprite(layer,&iterator);sprite;
                 sprite=NextSprite(layer,&iterator)) {
                zs1::video::VID_LAYOUT32* vid=
                    reinterpret_cast<zs1::video::VID_LAYOUT32*>(sprite->Vid());
                if (monsterMask[vid->index]) {
                    ++count;
                    zs1::video::VID_LAYOUT32* deathChild=
                        reinterpret_cast<zs1::video::VID_LAYOUT32*>(vid->childVid9_16[6]);
                    if (deathChild) {
                        if (monsterMask[vid->childVidSigned[15]])
                            ++count;
                        if (monsterMask[deathChild->childVidSigned[15]])
                            ++count;
                    }
                }

                zs1::engine::SPRITE_LAYOUT32* raw=
                    reinterpret_cast<zs1::engine::SPRITE_LAYOUT32*>(sprite);
                if (raw->actionsCount<=0)
                    continue;
                zs1::engine::ACT* actions=
                    reinterpret_cast<zs1::engine::ACT*>(raw->actionsData);
                if (actions[raw->actionsCount-1].command==0x49)
                    continue;

                for (int ai=raw->actionsCount-1;ai>=0;--ai) {
                    const zs1::engine::ACT action=actions[ai];
                    if (action.command==0x49)
                        break;
                    if (action.command!=0x23 || action.a<=0 || !monsterMask[action.a])
                        continue;

                    ++count;
                    zs1::video::VID_LAYOUT32* actionVid=
                        reinterpret_cast<zs1::video::VID_LAYOUT32*>(Vid(action.a));
                    zs1::video::VID_LAYOUT32* deathChild=
                        reinterpret_cast<zs1::video::VID_LAYOUT32*>(actionVid->childVid9_16[6]);
                    if (deathChild) {
                        if (monsterMask[actionVid->childVidSigned[15]])
                            ++count;
                        if (monsterMask[deathChild->childVidSigned[15]])
                            ++count;
                    }
                }
            }
        }
        PushInt(count);
        return 0;
    }
    case 231: { // 0xE7 -- ZS1 MenuFindNamed(name), target 0x00445AAF
        STRING name(*PopStr());
        SPRITE* found=m_menu.FindNamed(name);
        STRING context("return MenuFindNamed");
        m_logic.PushObject(found,&context);
        return 0;
    }
    case 232: { // 0xE8 -- ZS1 SpriteUnderCursor(), target 0x00445B28, no menu-state gate
        STRING context("return SpriteUnderCursor");
        m_logic.PushObject(m_menu.sprite,&context);
        return 0;
    }
    case 239: { // 0xEF -- BreakTrain(engine,x,y), target PopInt semantics
        const int y=PopInt();
        const int x=PopInt();
        ENGINE* engine=reinterpret_cast<ENGINE*>(PopInt());
        if(engine)
            engine->BreakTrain((float)x,(float)y);
        return 0;
    }
    case 240: { // 0xF0 -- FirstTrain(army)
        STRING context("return FirstTrain()");
        PushObject(FirstTrain(PopInt()),context);
        return 0;
    }
    case 241: { // 0xF1 -- NextTrain()
        STRING context("return NextTrain()");
        PushObject(NextTrain(),context);
        return 0;
    }
    case 242: { // 0xF2 -- SetCommandToTrain(engine,cmd,param), target PopInt semantics
        const int param=PopInt();
        const int cmd=PopInt();
        ENGINE* engine=reinterpret_cast<ENGINE*>(PopInt());
        if(engine && engine->IsSpriteClass(0x15)) {
            engine->SetCommandToTrain(25,cmd,param);
        } else {
            MYERROR::Log(::Error,"\xC1\xEE\xF0\xE8\xF1, \xF3 \xF2\xE5\xE1\xFF \xE2 PatrolTrain - train \xED\xE5\xE2\xE5\xF0\xED\xFB\xE9 %X",engine);
        }
        return 0;
    }
    case 243: { int value=PopInt(),y2=PopInt(),x2=PopInt(),y1=PopInt(),x1=PopInt(); RailMap.SetPushLine(x1,y1,x2,y2,value); return 0; }
    case 244: PushInt((int)m_input.mouseX); return 0;
    case 245: PushInt((int)m_input.mouseY); return 0;
    case 246: static_cast<PLAYER_STEAM*>(Player(1))->SetCleverEnemyAttack(PopInt()); return 0;
    case 247: PopInt(); return 0;
    case 249: { int index=PopInt(); VID* v=PopVid("for AddUnitLimit"); int limit=PopInt(); if(v!=EmptyVid){if(index==0xFF)v->m_limit394=limit;else v->m_limit398[index]=limit;} return 0; }
    case 250: { int on=PopInt();m_flags=(m_flags&~2u)|(on?2u:0u);return 0; }
    case 251: { int money=PopInt();Player(PopInt())->SetMoney(money);return 0; }
    case 252: PushInt(Player(PopInt())->GetMoney());return 0;
    case 253: PopInt();PopInt();PopInt();return 0; // 0xFD exact ZS1 stack consumption
    case 254: PopInt();PopInt();return 0;          // 0xFE exact ZS1 stack consumption
    case 255: { // ZS1 0x00448C34 -- zScriptExec bridge
        const STRING* str2s=PopStr(); const STRING* str1s=PopStr();
        const int arg2=PopInt(); const int arg1=PopInt(); const int subcommand=PopInt();
        int intResult=0; void* objectResult=0;
        const char* stringResult=zs1::ScriptExecDispatch(subcommand,arg1,arg2,
            str1s?const_cast<STRING*>(str1s)->CharPtr():"",str2s?const_cast<STRING*>(str2s)->CharPtr():"",&intResult,&objectResult);
        if(stringResult) { STRING value(stringResult); PushStr(&value); }
        else if(objectResult) { STRING context("return zScriptExec"); PushObject(objectResult,context); }
        else PushInt(intResult);
        return 0;
    }
    default: MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: Unknown extern Function %i",command); return 0;
    }
}
