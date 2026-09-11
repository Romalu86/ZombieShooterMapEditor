#include "mapedit/runtime.hpp"

namespace {
// ZS1 target helpers 0x00407CE0 / 0x00407D10.  The 12 DWORD table begins
// at MAP_EDIT+0x4B5C and is indexed by the power-of-two sprite type.
// Retail deliberately permits index 12 when no bit matched, so use the raw
// target offset instead of imposing a safer C++ array bound here.
unsigned int RememberedVidSlot(unsigned int type)
{
    unsigned int slot=0;
    unsigned int bit=1;
    while (bit!=type && slot<12u) {
        ++slot;
        bit<<=1;
    }
    return slot;
}

VID* GetRememberedVid(MAP_EDIT* editor,unsigned int type)
{
    const unsigned int slot=RememberedVidSlot(type);
    const uint32_t raw=*reinterpret_cast<const uint32_t*>(
        reinterpret_cast<const uint8_t*>(editor)+0x4B5Cu+slot*4u);
    return reinterpret_cast<VID*>(raw);
}

void SetRememberedVid(MAP_EDIT* editor,unsigned int type,VID* vid)
{
    const unsigned int slot=RememberedVidSlot(type);
    *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(editor)+0x4B5Cu+slot*4u)=
        reinterpret_cast<uint32_t>(vid);
}
}

// This intentionally preserves several odd original branches/thresholds instead of
// normalising them into a redesigned editor mode system.
void MAP_EDIT::ChangeMouseVid(VID* nvid,unsigned int new_type)
{
    optShiftSnapX=0;
    optShiftSnapY=0;
    if (!nvid)
        return;

    // ZS1 0x004078A3..0x0040790C: each editor sprite type remembers its
    // last selected VID.  An incompatible VID first restores that remembered
    // choice; only if none exists does retail search with NextVid().  A compatible
    // VID is remembered immediately.
    const unsigned int compatible=nvid->m_unknown0C & new_type;
    if (!compatible && new_type<=0x40u) {
        VID* remembered=GetRememberedVid(this,new_type);
        if (remembered) {
            nvid=remembered;
        } else {
            nvid=Vid(NextVid(nvid->m_idx,new_type));
            if (nvid==EmptyVid)
                return;
            SetRememberedVid(this,new_type,nvid);
        }
    } else if (compatible) {
        SetRememberedVid(this,new_type,nvid);
    }

    g_previousSpriteType=spriteType;
    spriteType=(int)new_type;

    // ZS1 0x00407924..0x0040794C: tactical editor mode always displays
    // VID slot 1 (or EmptyVid when that slot is unavailable), regardless of
    // the VID passed by the caller.  Remembered-VID routing above still runs
    // first exactly as in retail.
    if (optTacticMode)
        nvid=(m_noVid>1 && m_vids[1]) ? m_vids[1] : EmptyVid;

    if (!optTacticMode) {
        if (g_previousSpriteType!=spriteType)
            selectedSprites.Release();

        // Retail ChangeMouseVid does not switch the hardware cursor here.
        // Hardware ownership is handled by MAP_EDIT ctor/dtor; rail dragging
        // has its own explicit HardwareOn/HardwareOff path in Control().
        const int crossed20=((g_previousSpriteType>=0x20 && spriteType<0x20) ||
                             (g_previousSpriteType<0x20 && spriteType>=0x20));
        if (crossed20) {
            SendMessageA(hToolBar,0x401,0x9C59,spriteType<0x20);
            SendMessageA(hToolBar,0x401,0x9C62,spriteType<0x20);
            SendMessageA(hToolBar,0x401,0x9C75,spriteType<=0x40);
            SendMessageA(hToolBar,0x401,0xB01D,spriteType<0x40);
            SendMessageA(hToolBar,0x401,0xB029,spriteType<0x20);
            SendMessageA(hToolBar,0x401,0xB02B,spriteType<0x20);
        }
    }

    if (Mouse->Vid()!=nvid || spriteType!=g_previousSpriteType) {
        MENUITEMINFOA_OLD info;
        memset(&info,0,sizeof(info));
        STRING numberName;
        info.cbSize=sizeof(info);
        info.fMask=0x10;
        info.fType=0;

        const char* modeName;
        if (optTacticMode) {
            modeName="Tactic mode";
        } else if (spriteType<=0x40) {
            numberName=nvid->GetNumberName();
            modeName=numberName.CharPtr();
        } else if (spriteType==0x40) {
            // This branch is unreachable after the signed <=0x40 test in the original
            // machine code. It is retained because the original source/codegen contains it.
            modeName="Region edit mode";
        } else {
            modeName="Unknown mode";
        }
        info.dwTypeData=(char*)modeName;
        info.cch=(uint32_t)strlen(modeName);
        SetMenuItemInfoA(GetMenu(m_hWnd),6,1,&info);
        DrawMenuBar(m_hWnd);

        Mouse->Action(0x3E,nvid->m_idx,0,0);

        if (optControlPanel)
            DialogControlPanel(hControlPanel,(unsigned int)(spriteType==g_previousSpriteType),0,0);

        if (spriteType!=g_previousSpriteType) {
            unsigned int command=0;
            switch (spriteType) {
            case 1:    command=0x9C6F; break;
            case 2:    command=0x9C70; break;
            case 4:    command=0x9C71; break;
            case 8:    command=0x9C72; break;
            case 16:   command=0x9C73; break;
            case 32:   command=0xB023; break;
            case 64:   command=0xB022; break;
            default: break;
            }
            if (command)
                SendMessageA(hToolBar,0x402,command,1);
        }
    }

    if (new_type==0x40) {
        if (!curRegion)
            Right();
        if (curRegion)
            Mouse->Action(0x3E,curRegion->Vid()->m_idx,0,0);
    }
}
