#include "mapedit/runtime.hpp"
#include "../zs1/zUserMngr.h"

namespace {
int g_mouseWasDisabledBeforeDeactivate = 0;
int g_cursorRefreshPending = 0;
}

// ZS1.exe 0x00417AD0..0x00417CE0. Direct retail owner revalidated in P26.
int MAP::StartTact()
{
    unsigned long deltaTime;
    do {
        deltaTime = timeGetTime() - RealCurrentTime;
    } while (!deltaTime);

    PrevRealCurrentTime = RealCurrentTime;
    RealCurrentTime = timeGetTime();
    PrevCurrentTime = CurrentTime;
    if (deltaTime > 0x47u)
        deltaTime = 71u;
    CurrentTime += (int)(deltaTime * m_speed);
    ++m_fpsCnt;

    if (RealCurrentTime - prev_second_time >= 1000u) {
        prev_second_time = RealCurrentTime;
        m_fps = m_fpsCnt;
        m_fpsCnt = 0;
    }

    m_input.Tact();

    if (!(CurrentTime & 3u)) {
        for (int layerIndex=0; layerIndex<MAPEDIT_MAP_LAYER_COUNT; ++layerIndex) {
            SPRITE_LIST& layer = m_layers[layerIndex];
            const int count = layer.No();
            for (int hole=0; hole<count; ++hole) {
                if (*layer[hole])
                    continue;

                int deleted = 1;
                for (int i=hole+1; i<layer.No(); ++i) {
                    SPRITE* sprite = *layer[i];
                    if (!sprite) {
                        ++deleted;
                    } else {
                        *layer[i-deleted] = sprite;
                    }
                }
                layer.DeleteFrom(layer.No()-deleted);
                break;
            }
        }
    }

    if (m_flags & 0x40u) {
        m_flags &= ~0x40u;
        Load(m_startupLoad);
    }

    MSG_OLD msg;
    while (PeekMessageA(&msg,0,0,0,1)) {
        if (msg.message == 0x12u)
            return 1;
        if (m_hWnd && TranslateAcceleratorA(m_hWnd,m_hAccel,&msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

// ZS1 target 0x00414DB0..0x0041504B. Direct window-message owner.
int MAP::WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam)
{
    if ((m_flags & 8u) && m_input.WorkWndMessage(hwnd,msg,wParam,lParam))
        return 1;

    switch (msg) {
    case 0x000Fu: // WM_PAINT
        MYERROR::Log(::Error,"WM_PAINT");
        break;

    case 0x001Cu: { // WM_ACTIVATEAPP
        if (Sound) {
            if (wParam) {
                if (!(m_flags & 8u))
                    Sound->Resume();
            } else if (m_flags & 8u) {
                Sound->Pause();
            }
        }

        if (Mouse) {
            if (wParam) {
                if (!(m_flags & 8u)) {
                    if (!g_mouseWasDisabledBeforeDeactivate)
                        Mouse->Enable();
                    else
                        ShowCursor(0);
                }
            } else if (m_flags & 8u) {
                g_mouseWasDisabledBeforeDeactivate = Mouse->Visible == 0;
                Mouse->Disable();
            }
        }

        m_flags = (m_flags & ~8u) | (wParam ? 8u : 0u);
        g_cursorRefreshPending = 1;
        break;
    }

    case 0x0020u: // WM_SETCURSOR
        if ((m_flags & 8u) && g_cursorRefreshPending) {
            g_cursorRefreshPending = 0;
            if (Mouse && Mouse->Visible == 0 && GetForegroundWindow())
                ShowCursor(0);
        }
        break;

    case 0x0002u: { // WM_DESTROY
        if (!Graph->CapsFullScreen()) {
            RECT_OLD rect;
            GetWindowRect(m_hWnd,&rect);
            if (zs1::g_UserMngr) {
                zs1::g_UserMngr->SetInt(true,"WindowPositionX",rect.left);
                zs1::g_UserMngr->SetInt(true,"WindowPositionY",rect.top);
            }
        }
        m_hWnd = 0;
        PostQuitMessage(0);
        break;
    }

    case 0x0231u: // WM_ENTERSIZEMOVE
    case 0x0211u: // WM_ENTERMENULOOP
        Graph->BeginPause();
        Sound->Pause();
        break;

    case 0x0232u: // WM_EXITSIZEMOVE
    case 0x0212u: // WM_EXITMENULOOP
        Graph->EndPause();
        Sound->Resume();
        break;

    case 0x0112u: // WM_SYSCOMMAND
        switch (wParam) {
        case 0xF000u: // SC_SIZE
        case 0xF010u: // SC_MOVE
        case 0xF030u: // SC_MAXIMIZE
        case 0xF170u: // SC_MONITORPOWER
            if (Graph->CapsFullScreen())
                return 1;
            break;
        }
        break;
    }
    return 0;
}

// ZS1 target 0x00414910..0x004149ED.
void MAP::DeletePointerToSprite(SPRITE* sprite)
{
    for (int i=0;i<4;++i) {
        void** vtable=*reinterpret_cast<void***>(m_player[i]);
        typedef void (__thiscall *DeletePointerMethod)(void*,SPRITE*);
        reinterpret_cast<DeletePointerMethod>(vtable[1])(m_player[i],sprite);
    }

    m_logic.DeletePointerToObject(sprite);
    m_groups.DeletePointerToSprite(sprite);

    if (sprite->NoRef()>1) {
        int index=0;
        for (SPRITE* unit=Hash->FirstUnit(&index);unit;unit=Hash->NextUnit(&index)) {
            void** vtable=*reinterpret_cast<void***>(unit);
            typedef void (__thiscall *DeletePointerMethod)(void*,SPRITE*);
            reinterpret_cast<DeletePointerMethod>(vtable[4])(unit,sprite);
        }
    }

    for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
        if (sprite->NoRef()<=1)
            continue;
        int index=0;
        for (SPRITE* unit=FirstSprite(layer,&index);unit;unit=NextSprite(layer,&index)) {
            void** vtable=*reinterpret_cast<void***>(unit);
            typedef void (__thiscall *DeletePointerMethod)(void*,SPRITE*);
            reinterpret_cast<DeletePointerMethod>(vtable[4])(unit,sprite);
        }
    }
}

int MAP::NPlayer() { return m_curArmy; }

// ZS1 target 0x0044A080..0x0044A08F.
PLAYER* MAP::Player(int narmy) { return m_player[narmy&3]; }

int MAP::GetScrollType() { return static_cast<int>(m_shiftFlag); }

float MAP::GetTimeCoeff() { return m_speed; }

void MAP::SetSelectSpriteUnderCursor(int flag)
{
    m_flags=(m_flags&~0x00100000u)|(flag ? 0x00100000u : 0u);
}

void MAP::LoadInEndTact(const STRING* filename)
{
    m_flags|=0x40u;
    m_startupLoad=*filename;
}

// ZS1 target 0x00417090..0x004170AF.
void MAP::SetFlagman(int army,SPRITE* sprite)
{
    m_player[army&3]->SetFlagman(sprite);
}

// ZS1 target 0x0041AD50..0x0041AF91. Direct VID reload owner.
void MAP::ReloadVid()
{
    RESOURCE res;
    MYERROR::Log(::Error,"Reload vids");
    for (int i=0;i<m_noVid;++i) {
        VID* v=m_vids[i];
        if (v && v->m_exchangeVid!=v)
            ExchangeVid(v->m_exchangeVid,v);
    }
    if (res.OpenForRead(&m_resName,0x41544144u)) {
        char text[]="resource file";
        Error(7,text,0);
        return;
    }
    LoadWeapon(&res);
    if (res.GoBegin(0x204A424Fu)) {
        char text[]="load 'VID'";
        Error(11,text,0);
        res.Close();
        return;
    }
    do {
        int idx=0;
        res.Read(&idx,4);
        if (idx>=MAPEDIT_MAX_VID) {
            char text[]="nvid > MAX_VID";
            Error(4,text,(unsigned long)idx);
        }
        VID* current=m_vids[idx];
        idx=current->m_exchangeVid->m_idx;
        VID* v=m_vids[idx];
        v->m_name.Read(&res);
        v->LoadParameters(&res);
        if (v->m_weaponIndex<m_noWeapon)
            v->m_weapon=reinterpret_cast<WEAPON*>(reinterpret_cast<unsigned char*>(m_weapon)+0x23Cu*(unsigned int)v->m_weaponIndex);
        else
            v->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);
    } while (!res.GoNextSub(0x204A424Fu));
    for (int i=0;i<m_noVid;++i) {
        VID* v=m_vids[i];
        if (v && !v->IsExtraType())
            v->SetChildAndLink();
    }
    res.Close();
}
