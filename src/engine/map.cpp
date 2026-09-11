#include "mapedit/runtime.hpp"
#include "../zs1/zDebugLog.h"
#include "../zs1/zUserMngr.h"

extern "C" unsigned int __stdcall timeEndPeriod(unsigned int period);

namespace {
unsigned int g_demoOldCurrentTime=0;
unsigned int g_demoOldAbsoluteTime=0;
}

int MAP::IsInitSuccess()
{
    return (m_flags >> 2) & 1;
}

// No ZS1 owner is assigned here in P61: the earlier 0x40B8E0/0x40B8F0 tags
// were corrected to SPRITE::Y/Z after direct call-site/layout verification.
float MAP::SizeX() { return m_w; }
float MAP::SizeY() { return m_h; }


int MAP::DemoTact()
{
    int result=1;

    if (m_flags&0x200u) {
        int time=-1;
        m_resource.Read(&time,4u);

        if (time==-1 || m_input.key!=0 || m_input.lClick || m_input.rClick) {
            STRING nextMap;
            m_resource.GoNext(0x4F4D4544u); // DEMO
            m_resource.Shift(m_resource.ResSize());
            nextMap.Read(&m_resource);

            if (nextMap=="") {
                PostMessageA(m_hWnd,0x10u,0,0); // WM_CLOSE
            } else {
                m_resource.Close();
                Mouse->Enable();
                m_flags&=~0x200u;
                LoadInEndTact(&nextMap);
            }
            return 0;
        }

        m_input.Load(&m_resource);

        const unsigned int demoTime=static_cast<unsigned int>(time);
        if (demoTime-g_demoOldCurrentTime>0x47u)
            g_demoOldCurrentTime=demoTime-0x47u;

        const unsigned int target=demoTime-g_demoOldCurrentTime;
        const unsigned int elapsed=timeGetTime()-g_demoOldAbsoluteTime;
        if (target>elapsed) {
            // Retail MapEdit intentionally busy-spins; there is no Sleep call.
            while (target>=timeGetTime()-g_demoOldAbsoluteTime) {
            }
        } else {
            result=(CurrentTime-demoTime<=0x14u) ? 1 : 0;
        }

        CurrentTime=demoTime;
        g_demoOldCurrentTime=CurrentTime;
        g_demoOldAbsoluteTime=timeGetTime();
    }

    if (m_flags&0x100u) {
        m_resource.Write(&CurrentTime,4u);
        m_input.Save(&m_resource);
    }

    return result;
}

void MAP::NetworkTact()
{
}

int MAP::ValidateXY(float x, float y)
{
    return x >= 0.0f && x < m_w && y >= 0.0f && y < m_h;
}

// Current MapEdit lineage: 0x0040F670..0x0040F6CD.
SPRITE* MAP::NextSprite(int layer, int* index)
{
    --*index;
    if (*index < 0)
        return 0;

    SPRITE_LIST* const sprites=&m_layers[layer];
    while (!sprites->m_data[*index]) {
        --*index;
        if (*index < 0)
            return 0;
    }
    return sprites->m_data[*index];
}

STRING MAP::OpenDialog(const char* filter)
{
    return OpenSaveDialog(0,filter);
}

STRING MAP::SaveDialog(const char* filter)
{
    return OpenSaveDialog(1,filter);
}

// The retail owner uses the Win32 4.00 (0x4C-byte) OPENFILENAMEA layout,
// pauses the renderer around the modal common dialog and returns an empty
// STRING when the user cancels.
STRING MAP::OpenSaveDialog(int save,const char* filter)
{
    char fileName[4096];
    memset(fileName,0,sizeof(fileName));

    OPENFILENAMEA_OLD ofn = {};
    ofn.lStructSize=0x4C;
    ofn.hwndOwner=m_hWnd;
    ofn.hInstance=m_instance;
    ofn.lpstrFilter=filter;
    ofn.nFilterIndex=1;
    ofn.lpstrFile=fileName;
    ofn.nMaxFile=0x1000;
    ofn.lpstrInitialDir="maps";
    ofn.Flags=0x0008080Cu;
    if (!save)
        ofn.Flags|=0x00001000u;
    ofn.lpstrDefExt="map";

    Graph->BeginPause();
    const int accepted=save ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
    Graph->EndPause();

    return STRING(accepted ? fileName : "");
}

void REGION::SetSize(float width,float height)
{
    sizeX=width;
    sizeY=height;
}

// ZS1 target 0x00419880..0x0041993B.
float MAP::GetGroundZ(float x,float y)
{
    if (x < 0.0f)
        x=0.0f;
    else if (x >= m_w)
        x=(m_w-1.0f)/8.0f;
    else
        x/=8.0f;

    if (y < 0.0f)
        y=0.0f;
    else if (y >= m_h)
        y=(m_h-1.0f)/8.0f;
    else
        y/=8.0f;

    const int index=(int)x + (int)y*m_groundW;
    const short ground=m_groundz[index];
    const short temporary=m_tempGroundz[index];
    return (float)(ground > temporary ? ground : temporary);
}

// scans rows back toward the base cell and returns the maximum ground/temp-ground sample.
float MAP::GetGroundZScr(float screenX,float screenY)
{
    if (screenX<0.0f)
        screenX=0.0f;
    else if (screenX>=m_w)
        screenX=(m_w-1.0f)/8.0f;
    else
        screenX/=8.0f;

    if (screenY<0.0f)
        screenY=0.0f;
    else if (screenY>=m_h)
        screenY=(m_h-1.0f)/8.0f;
    else
        screenY/=8.0f;

    const int baseY=static_cast<int>(screenY);
    int row=baseY+32;
    if (row>=m_groundH)
        row=m_groundH-1;

    int index=static_cast<int>(screenX)+row*m_groundW;
    for (;row>=baseY;--row,index-=m_groundW) {
        const int z=(row-baseY)*8;
        if (static_cast<int>(m_groundz[index])>=z ||
            static_cast<int>(m_tempGroundz[index])>=z)
            return static_cast<float>(z);
    }
    return 0.0f;
}

int MAP::IsMapEdit()
{
    return m_flags & 1u;
}

int MAP::OptLoad()
{
    return (m_flags >> 5) & 1u;
}

MENU* MAP::Menu()
{
    return &m_menu;
}




int MAP::ValidateVid(int nvid)
{
    return nvid >= 0 && nvid < m_noVid && m_vids[nvid] != 0;
}

// ZS1 target 0x00443F50..0x00443F79.
VID* MAP::Vid(int nvid)
{
    if (nvid < 0 || nvid >= m_noVid || !m_vids[nvid])
        return EmptyVid;
    return m_vids[nvid];
}

float MAP::FromScreenX(float screenX) { return screenX + m_shiftX; }
float MAP::FromScreenY(float screenY) { return screenY + m_shiftY; }


// ZS1 target 0x00417D30..0x00417ED8; direct render-layer consumer used by
// GRAPH::Tact/Recalc.  Layers 0/10 bypass coarse culling; ordinary layers use
// the 0x800/0x400 integer window and the software-mouse chain follows last.
// Retail performs coarse integer culling for ordinary layers, but layers 0
// and 10 are drawn without that cull.  The software mouse/link chain is
// drawn after the map layer when it is not a hardware/always-top cursor.
void MAP::DrawLayer(int layer)
{
    int index=0;
    int centerX=0;
    int centerY=0;

    if (layer!=0 && layer!=10) {
        centerX=static_cast<int>(FromScreenX(Graph->SizeX()/2.0f));
        centerY=static_cast<int>(FromScreenY(Graph->SizeY()/2.0f));
    }

    for (SPRITE* sprite=FirstSprite(layer,&index); sprite; sprite=NextSprite(layer,&index)) {
        if (sprite->IsInvisible())
            continue;

        int draw=(layer==0 || layer==10);
        if (!draw) {
            const int dx=static_cast<int>(sprite->X())-centerX+0x400;
            const int yz=static_cast<int>(sprite->Y()-sprite->Z())-centerY+0x200;
            draw=(((dx & ~0x7ff)==0) && ((yz & ~0x3ff)==0)) ||
                 (static_cast<int>(sprite->Y())-centerY>=0x200);
        }

        if (draw)
            sprite->Draw();
    }

    if (!Mouse->IsHardware() && !Mouse->Vid()->PropAlwaysTop()) {
        for (SPRITE* sprite=Mouse; sprite; sprite=sprite->Link()) {
            if (sprite->Vid()->m_layer==layer && !sprite->IsInvisible())
                sprite->Draw();
        }
    }
}

// ZS1 target 0x004150C0..0x0041526E: viewport clamp, effect-2 capture, then
// shift MENU, MOUSE and INPUT world coordinates.
void MAP::SetShiftCoor(float center_x,float center_y,int effect)
{
    float shift_x=center_x-Graph->SizeX()/2.0f;
    float shift_y=center_y-Graph->SizeY()/2.0f;

    if ((effect & 0x10000000) == 0) {
        const float minX=m_shiftX1-Graph->ViewXMin();
        if (shift_x < minX)
            shift_x=minX;

        const float minY=m_shiftY1-Graph->ViewYMin();
        if (shift_y < minY)
            shift_y=minY;

        const float maxX=m_shiftX2-Graph->ViewXMax();
        if (shift_x > maxX)
            shift_x=maxX;

        const float maxY=m_shiftY2-Graph->ViewYMax();
        if (shift_y > maxY)
            shift_y=maxY;
    }

    if (m_shiftX == shift_x && m_shiftY == shift_y)
        return;

    if (effect == 2) {
        Graph->Effect(2,static_cast<int>(center_x),static_cast<int>(center_y),0);
        return;
    }

    const float dx=shift_x-m_shiftX;
    const float dy=shift_y-m_shiftY;
    m_shiftX=shift_x;
    m_shiftY=shift_y;

    for (int i=0;i<m_menu.No();++i) {
        SPRITE* sprite=*m_menu[i];
        sprite->ChangeCoor(sprite->X()+dx,sprite->Y()+dy,sprite->Z());
    }

    Mouse->ChangeCoor(Mouse->X()+dx,Mouse->Y()+dy,Mouse->Z());
    m_input.mouseX+=dx;
    m_input.mouseY+=dy;
}

// m_groundz and m_tempGroundz against z. Semantic name inferred from the body.
int MAP::IsGroundPointBlocked(float x,float y,float z)
{
    int ix;
    if (x<0.0f)
        ix=0;
    else if (x>=m_w)
        ix=m_groundW-1;
    else
        ix=static_cast<int>(x)/8;

    int iy;
    if (y<0.0f)
        iy=0;
    else if (y>=m_h)
        iy=m_groundH-1;
    else
        iy=static_cast<int>(y)/8;

    const int index=iy*m_groundW+ix;
    if (static_cast<float>(m_groundz[index])>=z)
        return 1;
    if (static_cast<float>(m_tempGroundz[index])>=z)
        return 1;
    return 0;
}

// 0x00419E60 and applies the retail fast-distance near-origin test on first hit.
// Semantic name inferred; constants are read directly from target .rdata.
int MAP::IsGroundSegmentStartBlocked(float x1,float y1,float z1,
                                     float x2,float y2,float z2)
{
    const float dx=x2-x1;
    const float dy=y2-y1;
    const float dz=z2-z1;
    float ax=dx<0.0f?-dx:dx;
    float ay=dy<0.0f?-dy:dy;
    const float maxxy=ax<ay?ay:ax;
    const int steps=static_cast<int>(maxxy*(1.0f/12.0f));
    if (!steps)
        return IsGroundPointBlocked(x1,y1,z1);

    const float sx=dx/static_cast<float>(steps);
    const float sy=dy/static_cast<float>(steps);
    const float sz=dz/static_cast<float>(steps);
    float x=x1;
    float y=y1;
    float z=z1;
    for (int i=steps-1;i>=0;--i) {
        x+=sx; y+=sy; z+=sz;
        if (!IsGroundPointBlocked(x,y,z))
            continue;
        // Retail inlines the two fast-distance reductions in this owner.
        float dxa=x-x1; if (dxa<0.0f) dxa=-dxa;
        float dya=y-y1; if (dya<0.0f) dya=-dya;
        if (dxa<dya) { const float t=dxa; dxa=dya; dya=t; }
        const float xy=dxa*0.9610000252723694f+dya*0.39800000190734863f;

        float dza=z-z1; if (dza<0.0f) dza=-dza;
        float major=xy; if (major<0.0f) major=-major;
        if (major<dza) { const float t=major; major=dza; dza=t; }
        const float xyz=major*0.9610000252723694f+dza*0.39800000190734863f;
        return static_cast<int>(xyz)==0 ? 1 : 0;
    }
    return 0;
}

// MapEditZS1.exe 0x00419790..0x00419871.
// Direct owner proof: frees both Grid-Z buffers, computes ceil(size/8), allocates
// two signed-16 grids and zero-fills both exactly as retail.
void MAP::ResetGroundZ()
{
    if (m_groundz)
        ::operator delete(m_groundz);
    if (m_tempGroundz)
        ::operator delete(m_tempGroundz);

    m_groundW=(static_cast<int>(m_w+7.0f))/8;
    m_groundH=(static_cast<int>(m_h+7.0f))/8;
    const int bytes=2*m_groundW*m_groundH;

    m_groundz=static_cast<short*>(::operator new(static_cast<unsigned int>(bytes)));
    m_tempGroundz=static_cast<short*>(::operator new(static_cast<unsigned int>(bytes)));
    memset(m_groundz,0,static_cast<unsigned int>(bytes));
    memset(m_tempGroundz,0,static_cast<unsigned int>(bytes));
}

// ZS1 target 0x00419C80..0x00419D19.
void MAP::SetGroundZ(float x,float y,float newZ)
{
    // Target owns the XY range tests directly in this mutator; do not route
    // through MAP::ValidateXY because that creates a non-retail owner call.
    if (x<0.0f) return;
    if (x>=m_w) return;
    if (y<0.0f) return;
    if (y>=m_h) return;

    const int index=static_cast<int>(x)/8 + static_cast<int>(y/8.0f)*m_groundW;
    if (m_groundz[index] < static_cast<int>(newZ))
        m_groundz[index]=static_cast<short>(newZ);
}

// m_shiftY1/m_shiftY2 at MAP +0x40/+0x44/+0x48/+0x4C.
void MAP::SetScrollBox(float minX,float minY,float maxX,float maxY)
{
    m_shiftX1=minX;
    m_shiftX2=maxX;
    m_shiftY1=minY;
    m_shiftY2=maxY;
}

// Validate/skip-map-editor/sprite-type/EMPTY filtering before returning a candidate.
int MAP::NextVid(int oldVid,unsigned int spriteType)
{
    if (oldVid < 0)
        oldVid = 0;

    int current = oldVid;
    for (;;) {
        if (++current >= m_noVid)
            current = 0;
        if (current == oldVid)
            return -1;
        if (!ValidateVid(current))
            continue;
        VID* vid = m_vids[current];
        if (vid->PropSkipMapEd())
            continue;
        if (!vid->IsSpriteType(spriteType))
            continue;
        if (vid->IsEmptyType())
            continue;
        return current;
    }
}

// validation and property/type filters as the forward iterator.
int MAP::PrevVid(int oldVid,unsigned int spriteType)
{
    if (oldVid < 0)
        oldVid = 0;

    int current = oldVid;
    for (;;) {
        if (--current < 0)
            current = m_noVid - 1;
        if (current == oldVid)
            return -1;
        if (!ValidateVid(current))
            continue;
        VID* vid = m_vids[current];
        if (vid->PropSkipMapEd())
            continue;
        if (!vid->IsSpriteType(spriteType))
            continue;
        if (vid->IsEmptyType())
            continue;
        return current;
    }
}

// ZS1 target 0x00419A70..0x00419C7A.
// Terrain-type VID (sprite class 7) samples the complete footprint against both
// ground grids; every other VID uses the ordinary point ground query.
// Retail does not guard VID/grid pointers here: callers and ResetGroundZ own
// those invariants, so adding fallback/null branches changes the target path.
float MAP::GetGroundZ(const VID* vid,float x,float y)
{
    if (vid->m_spriteClass != 7u)
        return GetGroundZ(x,y);

    float maxX = x + vid->m_footprintWidth * 0.5f - 3.0f;
    float maxY = y + vid->m_footprintHeight * 0.5f - 3.0f;
    float minX = x - (vid->m_footprintWidth * 0.5f - 3.0f);
    float minY = y - (vid->m_footprintHeight * 0.5f - 3.0f);

    if (minX < 0.0f) minX = 0.0f;
    else if (minX >= m_w) minX = (m_w - 1.0f) / 8.0f;
    else minX /= 8.0f;

    if (minY < 0.0f) minY = 0.0f;
    else if (minY >= m_h) minY = (m_h - 1.0f) / 8.0f;
    else minY /= 8.0f;

    if (maxX < 0.0f) maxX = 0.0f;
    else if (maxX >= m_w) maxX = (m_w - 1.0f) / 8.0f;
    else maxX /= 8.0f;

    if (maxY < 0.0f) maxY = 0.0f;
    else if (maxY >= m_h) maxY = (m_h - 1.0f) / 8.0f;
    else maxY /= 8.0f;

    int ground = -16383;
    for (float gy=minY; gy<=maxY; gy+=1.0f) {
        for (float gx=minX; gx<=maxX; gx+=1.0f) {
            const int index = static_cast<int>(gx) + static_cast<int>(gy) * m_groundW;
            const int groundZ = m_groundz[index];
            if (groundZ > ground)
                ground = groundZ;
            const int tempGroundZ = m_tempGroundz[index];
            if (tempGroundZ > ground)
                ground = tempGroundZ;
        }
    }
    return static_cast<float>(ground);
}

// VID has the extra flag, then deletes/trims extra entries from the VID table.
void MAP::DeleteExtraVid()
{
    for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
        int index=0;
        SPRITE* sprite=FirstSprite(layer,&index);
        while (sprite) {
            if (sprite->Vid()->IsExtraType())
                sprite->ScalarDeletingDestructor(1);
            sprite=NextSprite(layer,&index);
        }
    }

    for (int i=m_noVid-1;i>=0;--i) {
        VID* vid=m_vids[i];
        if (vid && vid->IsExtraType()) {
            void** vtable=*reinterpret_cast<void***>(vid);
            typedef void* (__thiscall *ScalarDelete)(void*,unsigned int);
            reinterpret_cast<ScalarDelete>(vtable[1])(vid,1);
            m_vids[i]=0;
        }
    }

    while (m_noVid > 0 && m_vids[m_noVid-1] == 0)
        --m_noVid;
}

// ZS1 target 0x0044A020..0x0044A049.
void MAP::Error(int type,char* text,unsigned long value)
{
    if (::Error)
        MYERROR::Error(::Error,"MAP",type,text,value);
}

// ZS1 target 0x00419D20..0x00419DB9.
void MAP::SetTempGroundZ(float x,float y,float newZ)
{
    // Target owns the XY range tests directly in this mutator; do not route
    // through MAP::ValidateXY because that creates a non-retail owner call.
    if (x<0.0f) return;
    if (x>=m_w) return;
    if (y<0.0f) return;
    if (y>=m_h) return;

    const int index=static_cast<int>(x)/8 + static_cast<int>(y/8.0f)*m_groundW;
    if (m_tempGroundz[index] < static_cast<int>(newZ))
        m_tempGroundz[index]=static_cast<short>(newZ);
}

// ZS1 target 0x00419DC0..0x00419E4E.
void MAP::ClearTempGroundZ(float x,float y,float newZ)
{
    // Target owns the XY range tests directly in this mutator; do not route
    // through MAP::ValidateXY because that creates a non-retail owner call.
    if (x<0.0f) return;
    if (x>=m_w) return;
    if (y<0.0f) return;
    if (y>=m_h) return;

    const int index=static_cast<int>(x)/8 + static_cast<int>(y/8.0f)*m_groundW;
    if (m_tempGroundz[index] == static_cast<int>(newZ))
        m_tempGroundz[index]=0;
}

// The retail PLAYER stores the sprite-under-cursor PTR_SPRITE payload at +0x24.
SPRITE* PLAYER::SpriteUnderCursor()
{
    return *reinterpret_cast<SPRITE**>(reinterpret_cast<uint8_t*>(this)+0x24);
}

// The retail PLAYER stores the flagman PTR_SPRITE payload at +0x10.
SPRITE* PLAYER::Flagman()
{
    return *reinterpret_cast<SPRITE**>(reinterpret_cast<uint8_t*>(this)+0x10);
}

// Target 0x004170B0 masks army with 3, reads m_player[army]+0x10 and returns it.
SPRITE* MAP::Flagman(int army)
{
    PLAYER* player=m_player[army & 3];
    return *reinterpret_cast<SPRITE**>(reinterpret_cast<uint8_t*>(player)+0x10);
}

SPRITE* MAP::SpriteUnderCursor()
{
    return m_player[m_curArmy & 3]->SpriteUnderCursor();
}

namespace {
float ClampDirectionalAimAxis(float value,float viewportSize,float retailExtent)
{
    const float lower=viewportSize-retailExtent;
    if (value < lower)
        return lower;
    if (value > retailExtent)
        return retailExtent;
    return value;
}
}

// Direct ZS1 ASM owner 0x00415280..0x00415899. The two velocity globals are
// retail storage; m_shiftFlag 0x10 follows X, 0x40 follows Y and 0x50 follows
// both axes.
void MAP::ControlShiftCoor()
{
    if (m_shiftFlag & 0x21u) {
        if (m_input.screenMouseX <= 5.0f && (m_shiftFlag & 0x01u)) {
            if (-Const->maxShiftSpeedX < g_shiftSpeedX)
                g_shiftSpeedX -= 0.04f;
        } else if (Graph->SizeX()-5.0f <= m_input.screenMouseX && (m_shiftFlag & 0x01u)) {
            if (g_shiftSpeedX < Const->maxShiftSpeedX)
                g_shiftSpeedX += 0.04f;
        } else if (m_input.left && (m_shiftFlag & 0x20u)) {
            if (-Const->maxShiftSpeedX < g_shiftSpeedX)
                g_shiftSpeedX -= 0.04f;
        } else if (m_input.right && (m_shiftFlag & 0x20u)) {
            if (g_shiftSpeedX < Const->maxShiftSpeedX)
                g_shiftSpeedX += 0.04f;
        } else {
            g_shiftSpeedX=0.0f;
        }

        if (m_input.screenMouseY <= 5.0f && (m_shiftFlag & 0x01u)) {
            if (-Const->maxShiftSpeedY < g_shiftSpeedY)
                g_shiftSpeedY -= 0.03f;
        } else if (Graph->SizeY()-5.0f <= m_input.screenMouseY && (m_shiftFlag & 0x01u)) {
            if (g_shiftSpeedY < Const->maxShiftSpeedY)
                g_shiftSpeedY += 0.03f;
        } else if (m_input.up && (m_shiftFlag & 0x20u)) {
            if (-Const->maxShiftSpeedY < g_shiftSpeedY)
                g_shiftSpeedY -= 0.03f;
        } else if (m_input.down && (m_shiftFlag & 0x20u)) {
            if (g_shiftSpeedY < Const->maxShiftSpeedY)
                g_shiftSpeedY += 0.03f;
        } else {
            g_shiftSpeedY=0.0f;
        }
    } else {
        g_shiftSpeedY=0.0f;
        g_shiftSpeedX=0.0f;
    }

    SPRITE* flagman=Flagman(m_curArmy);
    if (flagman && (m_shiftFlag & 0x04u) && g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        g_shiftSpeedX=(flagman->ScreenX()-Graph->SizeX()/2.0f)/1000.0f;
        g_shiftSpeedY=(flagman->ScreenY()-Graph->SizeY()/2.0f)/1000.0f;
    } else if (flagman && (m_shiftFlag & 0x08u) && g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        const float cx=ClampDirectionalAimAxis(m_input.screenMouseX,Graph->SizeX(),640.0f);
        const float cy=ClampDirectionalAimAxis(m_input.screenMouseY,Graph->SizeY(),480.0f);
        const float midX=(flagman->ScreenX()+cx)/2.0f;
        const float midY=(flagman->ScreenY()+cy)/2.0f;
        g_shiftSpeedX=(Graph->SizeX()/2.0f-midX)*(-4.0f/1000.0f);
        g_shiftSpeedY=(Graph->SizeY()/2.0f-midY)*(-4.0f/1000.0f);
    } else if (flagman && (m_shiftFlag & 0x50u)==0x50u &&
               g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        SetShiftCoor(flagman->ScreenX()+m_shiftX,
                     flagman->ScreenY()+m_shiftY,0);
        return;
    } else if (flagman && (m_shiftFlag & 0x10u) &&
               g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        SetShiftCoor(flagman->ScreenX()+m_shiftX,m_shiftY,0);
        return;
    } else if (flagman && (m_shiftFlag & 0x40u) &&
               g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        SetShiftCoor(m_shiftX,flagman->ScreenY()+m_shiftY,0);
        return;
    }

    const float delta=(float)(CurrentTime-PrevCurrentTime);
    SetShiftCoor((int)(g_shiftSpeedX*delta)+Graph->SizeX()/2.0f+m_shiftX,
                 (int)(g_shiftSpeedY*delta)+Graph->SizeY()/2.0f+m_shiftY,
                 0);
}

SPRITE* MENU::SpriteUnderCursor()
{
    return sprite;
}

int MAP::GetFPS()
{
    return m_fps;
}

// Direct ZS1 ASM owner 0x004149F0..0x00414DA4.
void MAP::DrawSecondaryInfo()
{
    // Retail reads the GRAPH viewport fields directly in this owner.
    const unsigned char* const graphBytes=reinterpret_cast<const unsigned char*>(Graph);
    const float viewXMin=*reinterpret_cast<const float*>(graphBytes+0x224);
    const float viewXMax=*reinterpret_cast<const float*>(graphBytes+0x228);
    const float viewYMin=*reinterpret_cast<const float*>(graphBytes+0x22C);
    const float viewYMax=*reinterpret_cast<const float*>(graphBytes+0x230);

    if (m_flags & 0x00020000u)
        Graph->PrintfXY(viewXMin,viewYMin+1.0f,"%i",m_fps);

    // ZS1 0x00414A33..0x00414A8D: layer rectangles are outside the
    // debugMode gate and directly traverse the 20 retail SPRITE_LISTs.
    if (m_flags & 0x00008000u) {
        for (int layer=0;layer<20;++layer) {
            SPRITE_LIST& list=m_layers[layer];
            int index=list.m_no-1;
            while (index>=0) {
                while (index>=0 && !list.m_data[index])
                    --index;
                if (index<0)
                    break;
                SPRITE* const sprite=list.m_data[index];
                sprite->DrawRectangle();
                --index;
            }
        }
    }

    if (!Const->debugMode)
        return;

    if (m_flags & 0x00010000u)
        Graph->PrintfXY(viewXMax-20.0f,viewYMin+1.0f,"%2i",Sound->GetNoPlayed());

    if ((m_flags & 0x00000800u) && m_groundz) {
        for (int y=1;y<m_groundH;++y) {
            for (int x=1;x<m_groundW;++x) {
                const int i=x+y*m_groundW;
                const int z1=m_groundz[i] > m_tempGroundz[i] ? m_groundz[i] : m_tempGroundz[i];
                const int z0=m_groundz[i-1] > m_tempGroundz[i-1] ? m_groundz[i-1] : m_tempGroundz[i-1];
                const float x1=static_cast<float>(8*x+4)-m_shiftX;
                const float y1=static_cast<float>(8*y+4-z1)-m_shiftY;
                const float x0=static_cast<float>(8*x-4)-m_shiftX;
                const float y0=static_cast<float>(8*y+4-z0)-m_shiftY;
                // GRAPH_CORE::InViewPort is header/source-inline in the retail
                // owner: direct half-open viewport tests, not owner calls.
                const int p1=x1>=viewXMin && x1<viewXMax && y1>=viewYMin && y1<viewYMax;
                const int p0=x0>=viewXMin && x0<viewXMax && y0>=viewYMin && y0<viewYMax;
                if (p1 || p0)
                    Graph->Line(x1,y1,x0,y0,GRAPH::GRAY);
            }
        }
    }

    if (m_flags & 0x00001000u) {
        SPRITE* sprite=GetSpriteScr(0x00400000,m_input.mouseX,m_input.mouseY);
        if (!sprite)
            sprite=GetSpriteScr(0x00008000,m_input.mouseX,m_input.mouseY);
        if (!sprite && m_menu.sprite) {
            m_menu.sprite->DrawSecondaryInfo();
        } else if (sprite) {
            sprite->DrawSecondaryInfo();
        } else {
            // Retail deliberately evaluates Flagman twice on the non-null path.
            if (Flagman(m_curArmy))
                Flagman(m_curArmy)->DrawSecondaryInfo();
        }
    }

    if (m_flags & 0x00004000u)
        m_groups.DrawNumber();

    if (m_flags & 0x00002000u)
        RailMap.DebugDraw();
}

RELATION::~RELATION()
{
    // C++ destroys newSprites then oldSprites, exactly matching the retail calls
    // at +0x10 and +0x00.
}

// freeing their arrays and zeroing list pointers/counts in the same RELATION layout.
void RELATION::Release()
{
    oldSprites.Release();
    newSprites.Release();
}

MENU::~MENU()
{
    // SPRITE_LIST base destructor is emitted automatically.
}

GROUPS::~GROUPS()
{
    // GROUP first is destroyed automatically.
}

namespace {
void DeleteVirtualObject(void* object)
{
    if (!object)
        return;
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[0])(object,1u);
}

void DeleteVirtualObjectSlot1(void* object)
{
    if (!object)
        return;
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[1])(object,1u);
}
}

void MOUSETIPS::Tact(INPUT* input)
{
    static unsigned long lastMoveTime=0;
    static float lastMouseX=0.0f;
    static float lastMouseY=0.0f;
    static STRING cachedText;

    if (lastMouseX!=input->screenMouseX || lastMouseY!=input->screenMouseY) {
        lastMoveTime=CurrentTime;
        lastMouseX=input->screenMouseX;
        lastMouseY=input->screenMouseY;
    }

    if (CurrentTime-lastMoveTime<=static_cast<unsigned long>(Const->MouseTipsTime) ||
        input->lClick || input->key || Map->Menu()->IsLClick() ||
        !Map->OptSelectSpriteUnderCursor()) {
        Clear();
        return;
    }

    if (!tip) {
        VID* vid=Map->Vid(6);
        cachedText=Map->GetMouseTipsString();
        if (vid==EmptyVid || cachedText.Length()==0)
            return;

        float x=input->screenMouseX+5.0f;
        float y=input->screenMouseY-vid->m_footprintHeight+3000.0f-10.0f;
        const float upperEdge=vid->m_footprintHeight/2.0f+y-3000.0f;
        if (Graph->ViewYMin()>=upperEdge)
            y=vid->m_footprintHeight/2.0f+input->screenMouseY+3000.0f+10.0f;

        const float right=x+static_cast<float>(cachedText.Length()+2)*vid->m_footprintWidth;
        if (Graph->ViewXMax()<=right)
            x=Graph->ViewXMax()-static_cast<float>(cachedText.Length()+2)*vid->m_footprintWidth;

        tip=Map->CreateSprite(vid,x,y,3000.0f,ANGLE(static_cast<uint8_t>(0)),0);
        if (tip) {
            STRING actionText("{",cachedText.CharPtr());
            actionText+="}";
            reinterpret_cast<SPRITE*>(tip)->Action(0x78,reinterpret_cast<int>(&actionText),0,0);
        }
        return;
    }

    if (CurrentTime-lastMoveTime>static_cast<unsigned long>(Const->MouseTipsTime)+500u) {
        lastMoveTime+=500u;
        STRING current=Map->GetMouseTipsString();
        if (current!=&cachedText)
            Clear();
    }
}

void MOUSETIPS::Clear()
{
    if (tip)
        DeleteVirtualObject(tip);
    tip=0;
}

void MOUSETIPS::DeletePointerToSprite(SPRITE* spr)
{
    if (tip==spr)
        tip=0;
}

// ZS1 compiler-generated scalar deleting destructor: exact wrapper around Clear()+delete.
MOUSETIPS::~MOUSETIPS()
{
    Clear();
}

// PROFILE destructor is header-visible; STRING teardown folds at each delete site.

// ZS1 compiler-generated scalar deleting destructor.
// layer/player/VID/global teardown order and reverse embedded-member destruction;
// modern sized-delete/helper emission is compiler noise, not a semantic change.
MAP::~MAP()
{
    for (int i=0;i<MAPEDIT_MAP_LAYER_COUNT;++i)
        m_layers[i].DeleteAll();

    if (::Mouse)
        ::Mouse->ScalarDeletingDestructor(1u);

    { /* VC6 for-scope */ for (int i=0;i<4;++i) {
        if (m_player[i])
            delete m_player[i];
    } } /* VC6 for-scope */

    if (::Hash)
        delete ::Hash;
    if (::Profile)
        delete ::Profile;
    if (::Sound)
        delete ::Sound;
    if (::Const)
        ::operator delete(::Const);
    if (::Registry)
        delete ::Registry;

    { /* VC6 for-scope */ for (int i=m_noVid-1;i>=0;--i) {
        if (m_vids[i]) {
            delete m_vids[i];
            m_vids[i]=0;
        }
    } } /* VC6 for-scope */
    m_noVid=0;

    MYERROR::Log(::Error,"Vid    release %i %i",SURFACE::MemoryInUse(),g_vidMemoryInUse);

    if (::Graph) {
        ::Graph->GRAPH::~GRAPH();
        ::operator delete(::Graph);
    }

    if (m_groundz)
        ::operator delete(m_groundz);
    if (m_tempGroundz)
        ::operator delete(m_tempGroundz);
    if (m_weapon)
        ::operator delete(m_weapon);

    if (::Error)
        delete ::Error;

    // ZS1 0x00414742..0x00414769: MAP owns only zUserMngr and zDebugLog
    // here.  High-score/help/script-main teardown is not part of this retail
    // destructor owner and must not be folded into the MAP lifecycle.
    if (zs1::g_UserMngr) {
        delete zs1::g_UserMngr;
        zs1::g_UserMngr=0;
    }
    if (zs1::g_DebugLog) {
        delete zs1::g_DebugLog;
        zs1::g_DebugLog=0;
    }

    CoUninitialize();
    timeEndPeriod(1u);

    // Embedded MOUSETIPS/GROUPS/MENU/RELATION/RESOURCE/LOGIC/layers/STRING
    // members are destroyed automatically after this body in declaration-reverse
    // order, matching 0x00416680..0x00416723 exactly.
}

void MAP::SetScrollType(int type)
{
    m_shiftFlag=static_cast<uint32_t>(type);
}

namespace {
void PlayerReleaseVirtual(PLAYER* player)
{
    void** const vtable=*reinterpret_cast<void***>(player);
    typedef void (__thiscall *ReleaseMethod)(PLAYER*);
    reinterpret_cast<ReleaseMethod>(vtable[4])(player);
}
}

// Exact retail body: this base implementation is the vtable fallback and returns 0.
int MAP::Tact()
{
    return 0;
}

// ZS1 target 0x00415A10..0x00415DC1. Full map release owner revalidated in P39.
void MAP::Release()
{
    for (int i=0;i<m_noVid;++i) {
        // ValidateVid and both NoSprites overloads are folded into the retail
        // MAP::Release owner. Keep the exact bounds/null/count source shape.
        if (i<0 || i>=m_noVid || !m_vids[i])
            continue;
        VID* const vid=m_vids[i];
        const int total=vid->m_entitiesNumber[0]+vid->m_entitiesNumber[1]+
                        vid->m_entitiesNumber[2]+vid->m_entitiesNumber[3];
        if (!total)
            continue;
        MYERROR::Log(::Error,"NoVid[%3i]=%i %i %i %i %s Layer=%i %s",
                     i,vid->m_entitiesNumber[0],vid->m_entitiesNumber[1],
                     vid->m_entitiesNumber[2],vid->m_entitiesNumber[3],
                     vid->m_name.m_buf,vid->m_layer,vid->m_resourceName.m_buf);
    }

    m_relation.Release();
    m_speed=1.0f;
    m_fps=0;
    m_fpsCnt=0;
    Graph->StopMovie();
    Graph->SetWind(25,ANGLE(static_cast<unsigned char>(200)));
    Graph->SetEnvironment(0xFFFFFFFFu);

    if ((m_flags>>9)&1u) {
        Mouse->Enable();
        m_resource.Close();
    }
    if ((m_flags>>8)&1u) {
        int end=-1;
        m_resource.Write(&end,4u);
        m_resource.PostAppend();
        m_resource.Close();
    }
    m_flags&=~0x300u;
    m_shiftFlag=1;

    MYERROR::Log(::Error,"Player release");
    { /* VC6 for-scope */ for (int i=0;i<4;++i) {
        if (m_player[i])
            m_player[i]->Release();
    } } /* VC6 for-scope */

    MYERROR::Log(::Error,"Sprite release");
    ENGINE::globaldeleting=1;
    for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
        SPRITE_LIST& list=m_layers[layer];
        int index=list.m_no-1;
        while (index>=0) {
            while (index>=0 && !list.m_data[index])
                --index;
            if (index<0)
                break;
            SPRITE* const sprite=list.m_data[index];
            sprite->ScalarDeletingDestructor(1u);
            --index;
        }
    }
    ENGINE::globaldeleting=0;

    { /* VC6 for-scope */ for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
        SPRITE_LIST& list=m_layers[layer];
        int index=list.m_no-1;
        while (index>=0 && !list.m_data[index])
            --index;
        if (index>=0) {
            SPRITE* const sprite=list.m_data[index];
            const int vidIndex=sprite->m_vid ? sprite->m_vid->m_idx : -1;
            MYERROR::Error(::Error,"SPRITE %i",10,"Sprite exist after delete",
                           static_cast<unsigned long>(index),vidIndex);
        }
    } } /* VC6 for-scope */

    GROUP* const firstGroup=m_groups.first.nextGroup;
    if (firstGroup!=&m_groups.first && firstGroup && ::Error)
        MYERROR::Error(::Error,"MAP",10,"Incorrect delete groups in DeleteAll()",0);

    MYERROR::Log(::Error,"Menu   release");
    if (m_menu.m_no) {
        SPRITE* const sprite=m_menu.m_data[0];
        const int vidIndex=sprite->m_vid ? sprite->m_vid->m_idx : -1;
        MYERROR::Error(::Error,"SPRITE %i",10,"Menu sprite exist after delete",0,vidIndex);
        m_menu.DeleteAll();
    }

    const unsigned long elapsed=CurrentTime-m_unknown30;
    const unsigned int averageFps=elapsed ? (static_cast<unsigned int>(m_noTact)*1000u)/elapsed : 0u;
    MYERROR::Log(::Error,"Average fps=%i",averageFps);
    MYERROR::Log(::Error,"Script release");

    m_logic.Release();
    m_noTact=0;
    DeleteExtraVid();
    { /* VC6 for-scope */ for (int i=0;i<m_noVid;++i) {
        if (m_vids[i])
            m_vids[i]->ResetSprites();
    } } /* VC6 for-scope */
    { /* VC6 for-scope */ for (unsigned int i=0;i<64u;++i)
        EvFunctionNumber[i]=static_cast<int>(i)+1000000; } /* VC6 for-scope */
}

// Complete retail map loader. The newer portable engine implementation was
// used only as a naming aid; resource ordering, legacy format branches, demo
// serialization and the final load lifecycle follow target 0x00415E00..0x0041694F.
// In particular, ZS1 does NOT load .lgc/.lgd or execute post-load sprite/event
// passes inside this owner.
void MAP::Load(STRING name)
{
    RESOURCE res;
    m_flags|=0x20u;
    if (name=="")
        return;

    if (m_w!=0.0f || m_h!=0.0f) {
        if (Const->debugMode)
            Graph->DrawDebugText("Release previous map");
        Release();
    }

    // Retail owner checks RESOURCE::file at MAP+0xA50 directly.
    if (!m_resource.file && m_resource.OpenForRead(&name,0x4F4D4544u)==0) {
        name.Read(&m_resource);
        m_flags|=0x200u;
    }

    if (res.OpenForRead(&name,0x2050414Du)!=0) {
        ::Error->Window("!!!ERROR!!!LOAD: Invalid map file %s",name.m_buf);
        return;
    }

    Mouse->Disable();
    m_prevMap=&m_mapName;
    m_mapName=&name;

    // Deliberately uninitialized before the selected retail write/read path.
    unsigned int seed;
    if (m_flags&0x200u)
        m_resource.Read(&seed,4u);
    else
        seed=timeGetTime();
    srand(seed);

    if (Const->debugMode)
        Graph->DrawDebugText("Load extra vid");
    LoadVid(&res);

    int version;
    if (res.GoBegin(0x48505247u)!=0) { // old GRPH layout
        if (res.GoBegin(0x44414548u)!=0) {
            Error(11,const_cast<char*>("HEAD"),0);
            return;
        }

        int dimension;
        res.Read(&dimension,4u); m_w=static_cast<float>(dimension);
        res.Read(&dimension,4u); m_h=static_cast<float>(dimension);
        short shift;
        res.Read(&shift,2u); m_shiftX=static_cast<float>(shift);
        res.Read(&shift,2u); m_shiftY=static_cast<float>(shift);
        res.Read(&CurrentTime,4u);
        m_unknown30=CurrentTime;
        PrevCurrentTime=CurrentTime;
        res.Read(&version,4u);
        Graph->OldLoadParameters(&res);

        if (Hash)
            delete Hash;
        Hash=new HASH_MAP(m_w,m_h,m_vids,m_noVid);
        SetScrollBox(0.0f,0.0f,m_w,m_h);
        ResetGroundZ();

        if (res.GoNext(0x44495247u)==0) {
            if (m_groundz)
                ::operator delete(m_groundz);
            m_groundz=0;
            const int loaded=res.SubLoad(reinterpret_cast<void**>(&m_groundz),0);
            const int expected=2*((static_cast<int>(m_w+7.0f))/8)*((static_cast<int>(m_h+7.0f))/8);
            if (loaded!=expected) {
                Error(4,const_cast<char*>("grid"),static_cast<unsigned long>(loaded));
                ResetGroundZ();
            }
        } else {
            res.GoBegin(0x20594E41u); // 'ANY '
        }

        if (res.GoNext(0x20525053u)!=0) {
            Error(11,const_cast<char*>("SPR "),0);
            return;
        }
        while (OldLoadSprite(&res)!=reinterpret_cast<SPRITE*>(-1)) {}
        RailMap.CreateAdditionalDots();

        if (res.GoNext(0x44525053u)!=0) {
            Error(11,const_cast<char*>("SPRD"),0);
            return;
        }
        for (SPRITE* sprite=ReadPointer(&res);
             sprite!=reinterpret_cast<SPRITE*>(-1);
             sprite=ReadPointer(&res)) {
            if (sprite)
                sprite->Action(0xC8,reinterpret_cast<int>(&res),version,0);
            res.GoNextSub(0x44525053u);
        }
    } else {
        if (Const->debugMode)
            Graph->DrawDebugText("Load graph parameters");
        Graph->LoadParameters(&res);

        if (res.GoNext(0x44414548u)!=0) {
            Error(11,const_cast<char*>("HEAD"),0);
            return;
        }
        res.Read(&m_w,4u);
        res.Read(&m_h,4u);
        res.Read(&m_shiftX,4u);
        res.Read(&m_shiftY,4u);
        res.Read(&CurrentTime,4u);
        PrevCurrentTime=1;
        CurrentTime=10;

        if (!(m_flags&0x200u) && m_resource.file) {
            m_flags|=0x100u;
            m_resource.PreAppend(0x4F4D4544u,0);
            name.Write(&m_resource);
            m_resource.Write(&seed,4u);
            m_resource.Write(&CurrentTime,4u);
        }
        if (m_flags&0x200u)
            m_resource.Read(&CurrentTime,4u);
        m_unknown30=CurrentTime;

        res.Read(&version,4u);
        if (version<=9) {
            // Retail performs four direct x87 FILD/FSTP conversions in MAP::Load.
            m_w=static_cast<float>(*reinterpret_cast<int*>(&m_w));
            m_h=static_cast<float>(*reinterpret_cast<int*>(&m_h));
            m_shiftX=static_cast<float>(*reinterpret_cast<int*>(&m_shiftX));
            m_shiftY=static_cast<float>(*reinterpret_cast<int*>(&m_shiftY));
        }
        MYERROR::Log(::Error,
            "CurrentTime   =%-15u   sizeof(SPRITE)=%-8i Map version   =%i",
            CurrentTime,static_cast<int>(sizeof(SPRITE)),version);

        if (Const->debugMode)
            Graph->DrawDebugText("Create new hash table");
        if (Hash)
            delete Hash;
        Hash=new HASH_MAP(m_w,m_h,m_vids,m_noVid);
        SetScrollBox(0.0f,0.0f,m_w,m_h);
        // Retail reads GRAPH size fields directly at +0x210/+0x214 in this owner.
        const float graphSizeX=*reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(Graph)+0x210);
        const float graphSizeY=*reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(Graph)+0x214);
        SetShiftCoor(graphSizeX*0.5f+m_shiftX,graphSizeY*0.5f+m_shiftY,0);

        if (Const->debugMode)
            Graph->DrawDebugText("Load gridZ");
        ResetGroundZ();
        if (res.GoNext(0x44495247u)==0) {
            if (m_groundz)
                ::operator delete(m_groundz);
            m_groundz=0;
            const int loaded=res.SubLoad(reinterpret_cast<void**>(&m_groundz),0);
            if (loaded!=2*m_groundW*m_groundH) {
                Error(4,const_cast<char*>("grid"),static_cast<unsigned long>(loaded));
                ResetGroundZ();
            }
        } else {
            res.GoBegin(0x20594E41u); // 'ANY '
        }

        if (res.GoNext(0x20525053u)!=0) {
            Error(11,const_cast<char*>("SPR "),0);
            return;
        }
        if (Const->debugMode)
            Graph->DrawDebugText("Load hardware terrain");

        int loadedLayer=0;
        for (SPRITE* sprite=LoadSprite(&res,version);
             sprite!=reinterpret_cast<SPRITE*>(-1);
             sprite=LoadSprite(&res,version)) {
            if (Const->debugMode && sprite && loadedLayer!=sprite->m_vid->m_layer) {
                loadedLayer=sprite->m_vid->m_layer;
                if (loadedLayer==0)
                    Graph->DrawDebugText("Load hardware terrain");
                else if (loadedLayer==1)
                    Graph->DrawDebugText("Build sprites in terrain");
                else if (loadedLayer==2)
                    Graph->DrawDebugText("Build sprites with alpha in terrain");
                else
                    Graph->DrawDebugText("Load sprites");
            }
            if (Graph->DrawLoadBar(m_vids[0]))
                Sound->MusicTact();
        }
        RailMap.CreateAdditionalDots();

        if (Const->debugMode)
            Graph->DrawDebugText("Load data for sprite");
        if (res.GoNext(0x44525053u)!=0) {
            Error(11,const_cast<char*>("SPRD"),0);
            return;
        }
        for (SPRITE* sprite=ReadPointer(&res);
             sprite!=reinterpret_cast<SPRITE*>(-1);
             sprite=ReadPointer(&res)) {
            if (Graph->DrawLoadBar(m_vids[0]))
                Sound->MusicTact();
            if (sprite)
                sprite->Action(0x51,reinterpret_cast<int>(&res),version,0);
            res.GoNextSub(0x44525053u);
        }

        if (Const->debugMode)
            Graph->DrawDebugText("Load players info");
        if (res.GoNext(0x59414C50u)!=0) {
            Error(11,const_cast<char*>("PLAY"),0);
            return;
        }
        for (int i=0;i<4;++i) {
            // Retail MAP::Load issues the PLAYER virtual call directly (vtable +0x0C).
            void** const vtable=*reinterpret_cast<void***>(m_player[i]);
            typedef void (__thiscall *LoadMethod)(PLAYER*,STREAM*);
            reinterpret_cast<LoadMethod>(vtable[3])(m_player[i],&res);
        }

        if (Const->debugMode)
            Graph->DrawDebugText("Load groups info");
        if (res.GoNext(0x554F5247u)!=0) {
            Error(11,const_cast<char*>("GROU"),0);
            return;
        }
        m_groups.Load(&res);
    }

    m_flags&=~0x20u;
    res.Close();
    m_relation.Release();
    MYERROR::Log(::Error,"Vid    release %i %i",SURFACE::MemoryInUse(),g_vidMemoryInUse);

    // ZS1 MAP::Load ends its post-container lifecycle here. Script-file
    // loading/event rebinding/create-script walks belong to other runtime paths
    // and are not present in target owner 0x00415E00..0x0041694F.
    RealCurrentTime=timeGetTime();
    if (!(m_flags&0x200u))
        Mouse->Enable();
    if (Const->debugMode)
        Graph->DrawDebugText("");
}

// MapEditZS1.exe MAP::SaveMap target owner; version 13 container route.
void MAP::Save(STRING name)
{
    const int version=13;
    RESOURCE res;

    if (name=="")
        return;

    if (name==&m_mapName) {
        STRING temp("tmp_del!.map");
        FRename(&m_mapName,&temp);
        m_mapName="tmp_del!.map";
    }

    // Retail ZS1 MAP::Save @ 0x00416960 calls RESOURCE::OpenForWrite, not OpenForAppend.
    if (res.OpenForWrite(&name,0x2050414Du)!=0) {
        ::Error->Window("Can't open file %s",name.m_buf);
        return;
    }

    int extraVid=0;
    for (;extraVid<m_noVid;++extraVid) {
        if (m_vids[extraVid] && m_vids[extraVid]->IsExtraType())
            break;
    }

    if (extraVid<m_noVid) {
        RESOURCE oldRes;
        if (oldRes.OpenForRead(&m_mapName,0x2050414Du)==0) {
            res.Copy(&oldRes,0x50414557u); // WEAP
            res.Copy(&oldRes,0x204A424Fu); // OBJ 
            oldRes.Close();
        } else {
            ::Error->Window("Can't open file '%s', needed for save map",m_mapName.m_buf);
        }
    }

    res.PreAppend(0x48505247u,0); // GRPH
    Graph->SaveParameters(&res);
    res.PostAppend();

    res.PreAppend(0x44414548u,0); // HEAD
    res.Write(&m_w,4u);
    res.Write(&m_h,4u);
    res.Write(&m_shiftX,4u);
    res.Write(&m_shiftY,4u);
    res.Write(&CurrentTime,4u);
    res.Write(&version,4u);
    res.PostAppend();

    int haveGrid=0;
    for (int y=0;y<m_groundH && !haveGrid;++y) {
        for (int x=0;x<m_groundW;++x) {
            if (m_groundz[x+y*m_groundW]!=0) {
                res.PreAppend(0x44495247u,0); // GRID
                res.Write(m_groundz,static_cast<unsigned int>(m_groundW*m_groundH*2));
                res.PostAppend();
                haveGrid=1;
                break;
            }
        }
    }

    res.PreAppend(0x20525053u,0); // SPR 
    // Retail MAP::Save walks the 21 layer LISTs directly, reverse-scanning
    // sparse slots and testing SPRITE::m_parent plus MENU membership in-owner.
    for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
        int index=m_layers[layer].m_no-1;
        while (index>=0) {
            SPRITE* sprite=m_layers[layer].m_data[index];
            while (!sprite && --index>=0)
                sprite=m_layers[layer].m_data[index];
            if (!sprite)
                break;

            if (!sprite->m_parent) {
                int menuIndex=m_menu.m_no-1;
                while (menuIndex>=0 && m_menu.m_data[menuIndex]!=sprite)
                    --menuIndex;
                if (menuIndex<0)
                    sprite->Save(&res);
            }
            --index;
        }
    }
    int end=-1;
    res.Write(&end,4u);
    res.PostAppend();

    { /* VC6 for-scope */ for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
        int index=m_layers[layer].m_no-1;
        while (index>=0) {
            SPRITE* sprite=m_layers[layer].m_data[index];
            while (!sprite && --index>=0)
                sprite=m_layers[layer].m_data[index];
            if (!sprite)
                break;

            if (!sprite->m_parent) {
                int menuIndex=m_menu.m_no-1;
                while (menuIndex>=0 && m_menu.m_data[menuIndex]!=sprite)
                    --menuIndex;
                if (menuIndex<0) {
                    const int before=res.Tell();
                    res.PreAppend(0x44525053u,0); // SPRD
                    res.Write(&sprite,4u);
                    sprite->Action(0x50,reinterpret_cast<long>(&res),0,0);
                    const int afterAction=res.Tell();
                    if (afterAction>before+5)
                        res.PostAppend();
                }
            }
            --index;
        }
    } } /* VC6 for-scope */

    res.PreAppend(0x44525053u,0); // SPRD terminator
    end=-1;
    res.Write(&end,4u);
    res.PostAppend();

    res.PreAppend(0x59414C50u,0); // PLAY
    for (int i=0;i<4;++i) {
        // Retail MAP::Save issues the PLAYER virtual call directly (vtable +0x08).
        void** const vtable=*reinterpret_cast<void***>(m_player[i]);
        typedef void (__thiscall *SaveMethod)(PLAYER*,STREAM*);
        reinterpret_cast<SaveMethod>(vtable[2])(m_player[i],&res);
    }
    res.PostAppend();

    res.PreAppend(0x554F5247u,0); // GROU
    m_groups.Save(&res);
    res.PostAppend();
    res.Close();

    if (m_mapName=="tmp_del!.map") {
        STRING temp("tmp_del!.map");
        FRemove(&temp);
        m_mapName=&name;
    }
}

int __cdecl SortCallBack1(const int* lhs,const int* rhs)
{
    const int aIndex=*lhs;
    const int bIndex=*rhs;
    VID* a=Map->Vid(aIndex);
    VID* b=Map->Vid(bIndex);
    if (a==EmptyVid)
        return b==EmptyVid ? 0 : 1;
    if (b==EmptyVid)
        return -1;
    STRING* aName=&a->m_name;
    STRING* bName=&b->m_name;
    if (bName->operator<(aName))
        return 1;
    if (bName->operator>(aName))
        return -1;
    return 0;
}

// resets the list box and inserts filtered VID labels/data with the same selection path.
int MAP::VidToListBox(DIALOG_LIST_BOX* list,unsigned long unitTypeMask,int selectVid,int sort)
{
    int order[MAPEDIT_MAX_VID];
    for (int i=0;i<m_noVid;++i)
        order[i]=i;
    if (sort)
        qsort(order,static_cast<unsigned int>(m_noVid),sizeof(int),reinterpret_cast<int (__cdecl *)(const void*,const void*)>(SortCallBack1));

    list->Reset();
    for (int i=0;i<m_noVid;++i) {
        const int nvid=order[i];
        VID* vid=m_vids[nvid];
        if (!vid)
            continue;
        if (vid->PropSkipMapEd())
            continue;
        if (!vid->IsSpriteType(unitTypeMask))
            continue;
        STRING label=vid->GetNumberName();
        const int listIndex=list->AddStringWithData(&label,nvid);
        if (vid->m_idx==selectVid)
            list->SetCurrent(listIndex);
    }
    return list->NoString();
}

// while applying the same VID skip and sprite-type filters.
int MAP::VidToControlBox(DIALOG_COMBO_BOX* list,unsigned long unitTypeMask,int selectVid)
{
    list->Reset();
    for (int i=0;i<m_noVid;++i) {
        VID* vid=m_vids[i];
        if (vid && !vid->PropSkipMapEd() && vid->IsSpriteType(static_cast<int>(unitTypeMask))) {
            STRING label=vid->GetNumberName();
            const int listIndex=list->AddStringWithData(&label,i);
            if (vid->m_idx==selectVid)
                list->SetCurrent(listIndex);
        }
    }
    return list->NoString();
}

// MapEditZS1.exe 0x00417EE0..0x00417F09.  Retail stores the new map
// dimensions, resets the scroll box to 0..size, then rebuilds both Grid-Z
// arrays through MAP::ResetGroundZ.
void MAP::ChangeSizeXY(float newSizeX,float newSizeY)
{
    m_w=newSizeX;
    m_h=newSizeY;
    SetScrollBox(0.0f,0.0f,m_w,m_h);
    ResetGroundZ();
}

// ZS1 target 0x0041AA00..0x0041AB01.
void MAP::ExchangeVid(VID* vid1,VID* vid2)
{
    if (!vid1 || !vid2 || vid1==vid2 || vid1==EmptyVid || vid2==EmptyVid)
        return;


    for (int i=0;i<m_noVid;++i) {
        VID* const vid=m_vids[i];
        if (!vid)
            continue;
        if (vid->m_linkVid==vid1)
            vid->m_linkVid=vid2;
        else if (vid->m_linkVid==vid2)
            vid->m_linkVid=vid1;
        for (int child=0;child<17;++child) {
            if (vid->m_aniChildVid[child]==vid1)
                vid->m_aniChildVid[child]=vid2;
            else if (vid->m_aniChildVid[child]==vid2)
                vid->m_aniChildVid[child]=vid1;
        }
    }

    m_vids[vid1->m_idx]=vid2;
    m_vids[vid2->m_idx]=vid1;

    VID* tempVid=vid1->m_exchangeVid;
    vid1->m_exchangeVid=vid2->m_exchangeVid;
    vid2->m_exchangeVid=tempVid;

    int tmp=vid1->m_idx;
    vid1->m_idx=vid2->m_idx;
    vid2->m_idx=tmp;

    int* const raw1=reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid1)+0x410);
    int* const raw2=reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid2)+0x410);
    for (int i=0;i<21;++i) {
        tmp=raw1[i]; raw1[i]=raw2[i]; raw2[i]=tmp;
    }

}


SPRITE* MAP::Flagman()
{
    return Flagman(m_curArmy);
}

// ZS1 target 0x004170D0..0x00417420. Constructor dispatch is exact for
// sprite classes 2,4,5,6,7,8,9,10,12,19,23; all other classes in 2..23
// route to the retail invalid-Behave error path. Create event is VID+0x448.
SPRITE* MAP::CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
{
    // ZS1 target 0x004170D8..0x004170EB rejects both NULL and the retail EmptyVid sentinel.
    if (!vid || vid==EmptyVid)
        return 0;

    if ((vid->m_propertyBits>>4)&1u)
        vid=REGION::ConvertVid(vid,x,y,z);

    SPRITE* sprite=0;
    switch (vid->m_spriteClass) {
    case 2:  sprite=new UNIT(vid,x,y,z,direction,parent); break;
    case 4:  sprite=new PLANE(vid,x,y,z,direction,parent); break;
    case 5:  sprite=new CANNON(vid,x,y,z,direction,parent); break;
    case 6:  sprite=new PRIMITIVE(vid,x,y,z,direction,parent); break;
    case 7:  sprite=new MAN(vid,x,y,z,direction,parent); break;
    case 8:  sprite=new BUILDED_TERRAIN(vid,x,y,z,direction,parent); break;
    case 9:  sprite=new SPRITE(vid,x,y,z,direction,parent); break;
    case 10: sprite=new FRAME(vid,x,y,z,direction,parent); break;
    case 12: sprite=new LINKER(vid,x,y,z,direction,parent); break;
    case 19: sprite=new STEXT(vid,x,y,z,direction,parent); break;
    case 23: sprite=new REGION(vid,x,y,z,direction,parent); break;
    default:
        Error(3,const_cast<char*>("sprite - Behave is invalidate"),static_cast<unsigned long>(vid->m_spriteClass));
        return 0;
    }

    if (!IsMapEdit() && !OptLoad() && sprite) {
        // ZS1 target 0x004173D7..0x004173EA reads VID+0x448, which is
        // m_eventFunction[14].  Slot 14 is excluded from the generic animation
        // event dispatcher because creation dispatches it here.
        const int function=sprite->Vid()->m_eventFunction[14];
        if (function>=0)
            ScriptRun(function,sprite,0,0);
    }
    return sprite;
}

int MOUSETIPS::IsOut()
{
    return tip!=0;
}

int MAP::OptSelectSpriteUnderCursor()
{
    return static_cast<int>((m_flags>>20)&1u);
}

int MAP::OptDrawHpLines(const SPRITE* spr)
{
    if (m_flags&0x00000400u)
        return 1;
    if (m_mousetips.IsOut() && m_player[0]->SpriteUnderCursor()==spr)
        return 1;
    return 0;
}

// ZS1 retail creates VID 1024 as VID_HARDWARE, leaves base m_flag/PropGamma
// untouched, assigns MAP weapon data, then creates the centered ground sprite.
void MAP::CreateEmptyHardwareGround()
{
    if (m_noVid<0x401)
        m_noVid=0x401;

    if (m_vids[1024]) {
        delete m_vids[1024];
        m_vids[1024]=0;
    }

    m_vids[1024]=new VID_HARDWARE(1024,static_cast<int>(SizeX()),static_cast<int>(SizeY()));
    if (!m_vids[1024])
        return;

    // Retail copies MAP+0x2C0 to VID+0x464 immediately after construction.
    m_vids[1024]->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);

    CreateSprite(m_vids[1024],SizeX()/2.0f,SizeY()/2.0f,0.0f,ANGLE(static_cast<uint8_t>(0)),0);
    MYERROR::Log(::Error,"Create Empty Hardware Ground");
}

RELATION::RELATION()
    : oldSprites(), newSprites()
{
}

MENU::MENU()
    : SPRITE_LIST(), clickFlags(0), sprite(0)
{
}

MOUSETIPS::MOUSETIPS()
    : tip(0)
{
}

PROFILE::PROFILE(const STRING* filename)
    : FileName()
{
    Load(filename);
}

SPRITE* MAP::ReadPointer(STREAM* res)
{
    int token;
    res->Read(&token,4u);
    if (token==-1)
        return reinterpret_cast<SPRITE*>(-1);
    return m_relation.Decode(reinterpret_cast<SPRITE*>(token));
}

// ZS1 target 0x00417830..0x00417960: DWORD pointer token, WORD nvid/x/y/z,
// BYTE direction + BYTE pad, CreateSprite through MAP vtable slot +0x20, then
// relation insertion. No army field exists in this legacy record.
SPRITE* MAP::OldLoadSprite(RESOURCE* res)
{
    int token;
    res->Read(&token,4u);
    if (token==-1)
        return reinterpret_cast<SPRITE*>(-1);

    short nvid;
    short x;
    short y;
    short z;
    unsigned char direction;
    unsigned char unused;
    res->Read(&nvid,2u);
    res->Read(&x,2u);
    res->Read(&y,2u);
    res->Read(&z,2u);
    res->Read(&direction,1u);
    res->Read(&unused,1u);

    SPRITE* sprite=0;
    if (nvid>=0 && nvid<m_noVid && m_vids[nvid]) {
        sprite=CreateSprite(m_vids[nvid],static_cast<float>(x),static_cast<float>(y),
                            static_cast<float>(z),ANGLE(direction),0);
    } else if (::Error) {
        MYERROR::Error(::Error,"MAP",3,"sprite, this vid not exist",static_cast<unsigned long>(nvid));
    }

    m_relation.Insert(reinterpret_cast<SPRITE*>(token),sprite);
    return sprite;
}

void MAP::RestoreDeviceObjects()
{
    for (int i=0;i<m_noVid;++i) {
        VID* vid=m_vids[i];
        if (vid && vid->IsFontType())
            static_cast<VID_FONT*>(vid)->RestoreDeviceObjects();
    }
}

void MAP::InvalidateDeviceObjects()
{
    for (int i=0;i<m_noVid;++i) {
        VID* vid=m_vids[i];
        if (vid && vid->IsFontType())
            static_cast<VID_FONT*>(vid)->InvalidateDeviceObjects();
    }
}

// with serialized stride 0x280, materializes runtime records and updates m_noWeapon/m_weapon.
void MAP::LoadWeapon(RESOURCE* res)
{
    if (m_weapon)
        ::operator delete(m_weapon);
    m_weapon=0;

    void* serialized=0;
    m_noWeapon=res->Load(0x50414557u,&serialized,0x280);
    if (!m_noWeapon || !serialized) {
        if (EmptyVid)
            EmptyVid->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);
        return;
    }

    const unsigned int runtimeBytes=static_cast<unsigned int>(m_noWeapon)*0x23Cu;
    m_weapon=::operator new(runtimeBytes);
    if (m_weapon) {
        unsigned char* const dstBase=reinterpret_cast<unsigned char*>(m_weapon);
        const unsigned char* const srcBase=reinterpret_cast<const unsigned char*>(serialized);
        const unsigned int sentinel=0x497423F0u; // 999999.0f

        for (int i=0;i<m_noWeapon;++i) {
            unsigned char* const dst=dstBase+static_cast<unsigned int>(i)*0x23Cu;
            const unsigned char* const src=srcBase+static_cast<unsigned int>(i)*0x280u;

            // ZS1 target copies the proven common prefix (0x87 DWORDs = 0x21C).
            for (unsigned int off=0;off<0x21Cu;off+=4u)
                *reinterpret_cast<unsigned int*>(dst+off)=*reinterpret_cast<const unsigned int*>(src+off);

            // The runtime record stores three 8-entry selector tables as bytes
            // rather than the serialized DWORD[8] form. The target keeps only
            // each source DWORD's low byte.
            for (int j=0;j<8;++j) {
                dst[0x1C0u+j]=static_cast<unsigned char>(*reinterpret_cast<const unsigned int*>(src+0x1C0u+j*4u));
                dst[0x1C8u+j]=static_cast<unsigned char>(*reinterpret_cast<const unsigned int*>(src+0x1E0u+j*4u));
                dst[0x1D0u+j]=static_cast<unsigned char>(*reinterpret_cast<const unsigned int*>(src+0x200u+j*4u));

                *reinterpret_cast<unsigned int*>(dst+0x1D8u+j*4u)=
                    *reinterpret_cast<const unsigned int*>(src+0x220u+j*4u);

                const unsigned int rawA=*reinterpret_cast<const unsigned int*>(src+0x240u+j*4u);
                float valueA=*reinterpret_cast<const float*>(src+0x240u+j*4u);
                if (rawA!=sentinel)
                    valueA*=0.001f;
                *reinterpret_cast<float*>(dst+0x1F8u+j*4u)=valueA;

                const unsigned int rawB=*reinterpret_cast<const unsigned int*>(src+0x260u+j*4u);
                float valueB=*reinterpret_cast<const float*>(src+0x260u+j*4u);
                if (rawB!=sentinel)
                    valueB*=0.001f;
                *reinterpret_cast<float*>(dst+0x218u+j*4u)=valueB;
            }
            *reinterpret_cast<int*>(dst+0x238)=7;
        }
    }

    ::operator delete(serialized);
    if (EmptyVid)
        EmptyVid->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);
}


// Zombie Shooter 1 retail MAP::CreateVid.
// Complete retail VID factory.  The temporary PICTURE conversion and path
// resolution deliberately preserve the old editor behavior, including the
// early-return paths which do not remove a converted temporary file.
VID* MAP::CreateVid(RESOURCE* res,int nvid)
{
    VID* vid=0;
    STRING name;
    STRING filename;
    VID scratch;

    name.Read(res);
    scratch.m_dotFrameCount=32000;
    const int parametersPos=res->Tell();
    scratch.LoadParameters(res);
    filename.Read(res);
    filename=filename.ToLower();
    res->Seek(parametersPos);

    // Reuse a compatible already-loaded resource through the retail mirror
    // chain.  Hardware resources intentionally ignore software gamma state.
    int i=0;
    for (;i<m_noVid;++i) {
        if (!ValidateVid(i))
            continue;
        VID* const other=m_vids[i];
        STRING* const otherFilename=reinterpret_cast<STRING*>(
            reinterpret_cast<unsigned char*>(other)+0x2F4);
        if (!otherFilename->operator==(&filename))
            continue;
        if (scratch.m_spriteClass==8) {
            if (other->m_spriteClass!=8)
                continue;
        } else if (other->m_spriteClass==8) {
            continue;
        }
        if (!other->IsHardwareType()) {
            GAMMA* const otherGamma=reinterpret_cast<GAMMA*>(
                reinterpret_cast<unsigned char*>(other)+0x2E0);
            GAMMA* const scratchGamma=reinterpret_cast<GAMMA*>(
                reinterpret_cast<unsigned char*>(&scratch)+0x2E0);
            if (!otherGamma->operator==(scratchGamma))
                continue;
        }
        if (!other->IsHardwareType() && (other->PropGamma() ^ scratch.PropGamma()))
            continue;

        vid=other->CreateMirror();
        vid->m_idx=nvid;
        vid->m_name.operator=(&name);
        vid->m_resourceName.operator=(&filename);
        vid->LoadParameters(res);
        break;
    }
    if (i<m_noVid) {
        vid->SetLayer();
        return vid;
    }

    const int isFont=filename.HaveSubStr(".fon") || filename.HaveSubStr(".ttf");
    const int isPicture=filename.HaveSubStr(".tga") || filename.HaveSubStr(".bmp") ||
                        filename.HaveSubStr(".flc") || filename.HaveSubStr(".jpg");
    int removeTemp=0;

    if (isPicture) {
        PICTURE_MAKEVID* converter=0;
        if (scratch.m_spriteClass==19)
            converter=new PICTURE_FONT();
        else
            converter=new PICTURE_MAKEVID();

        if (converter->Load(filename,STRING(STRING::EMPTY),STRING(STRING::EMPTY))) {
            MYERROR::Log(::Error,"LOAD::Can't open file %s",filename.m_buf);
        } else {
            filename=FTempFile("c:\\tmp","vid");
            removeTemp=1;
            const int makeResult=converter->MakeVid(0,filename);
            (void)makeResult;
        }
        delete converter;
    }

    // The retail compiler ends the local RESOURCE lifetime before SetLayer()
    // (MapEdit.exe 0x0041E557), so keep the resource in a nested scope rather
    // than letting its destructor run at function return.
    {
    RESOURCE vidFile;
    if (!isFont) {
        STRING resourceName=res->GetFileName().ToLower();
        if (resourceName.HaveSubStr(".map") && resourceName.HaveSubStr(":\\")) {
            const STRING sub=filename.BeforeLast("\\");
            resourceName=resourceName.BeforeLast("\\");
            resourceName=resourceName.BeforeLast(sub.m_buf);
        } else {
            resourceName=STRING::EMPTY;
        }

        const STRING path=resourceName+filename;
        const int vidOpenResult=vidFile.OpenForRead(&path,0x20444956u);
        if (vidOpenResult) {
            Graph->FlipToGDISurface();
            MYERROR::Log(::Error,"LOAD::Can't open file %s",filename.m_buf);
            return vid;
        }
        if (vidFile.GoBegin(0x44414548u))
            MYERROR::Log(::Error,"!!!ERROR!!!VID '%s': Load() not HEAD ",filename.m_buf);
        vidFile.Read(&scratch.m_extraTypeFlags,2u);
    }

    // ZS1 retail 0x0041AFA0 classifies the VID from the two-byte HEAD
    // type word it has just read.  Do not route these tests through runtime
    // VID flag/property helpers: notably VID::IsLight() reads m_flag +0x14.
    const unsigned int headType=scratch.m_extraTypeFlags;
    if (isFont) {
        vid=new VID_FONT();
    } else if (headType&0x1000u) {
        return 0;
    } else if (headType&0x0080u) {
        vid=new VID_LIGHT();
    } else if ((headType&0x0020u) && (headType&0x0002u) && (headType&0x0004u)) {
        vid=new VID_HARDWARE_Z();
    } else if (headType&0x0020u) {
        vid=new VID_HARDWARE();
    } else if (scratch.m_spriteClass==8) {
        vid=new VID_SOFTWARE16();
    } else if (Graph->BytesPerPixel()==4) {
        vid=new VID_SOFTWARE();
    } else {
        vid=new VID_SOFTWARE16();
    }

    vid->m_extraTypeFlags=scratch.m_extraTypeFlags;
    vid->m_idx=nvid;
    vid->m_name.operator=(&name);
    vid->m_resourceName.operator=(&filename);
    vidFile.Read(&vid->m_phaseRandomInterval,2u);
    vidFile.Read(&vid->m_dotFrameCount,2u);
    vidFile.Read(&vid->m_regionTileStepX,2u);
    vidFile.Read(&vid->m_regionTileStepY,2u);
    vid->LoadParameters(res);
    vid->Load(&vidFile);
    vidFile.Close();
    if (removeTemp)
        FRemove(&filename);
    } // RESOURCE::~RESOURCE() precedes SetLayer() in the canonical executable.
    vid->SetLayer();
    return vid;
}


void MAP::LoadVid(RESOURCE* res)
{
    int idx=0;
    const unsigned long start=timeGetTime();
    if (!m_weapon) {
        LoadWeapon(res);
        if (m_noWeapon)
            MYERROR::Log(::Error,"LoadWeapon::No=%-5i             sizeof(WEAPON)=%-4i",m_noWeapon,static_cast<int>(sizeof(WEAPON)));
    }

    if (res->GoBegin(0x204A424Fu)) {
        Error(11,const_cast<char*>("load 'VID'"),0);
        return;
    }

    const int hadVids=(m_noVid!=0);
    do {
        idx=-1;
        res->Read(&idx,4u);
        // A53 AS1 checked idx >= 0x800 here. ZS1 target storage is 0x1000 entries;
        // this is a capacity check, unlike the separate 0x800 VID-query type tag.
        // reject negative values here and does not continue after reporting the
        // error; preserve that behavior exactly for ASM/ABI parity.
        if (idx>=MAPEDIT_MAX_VID)
            Error(4,const_cast<char*>("nvid > MAX_VID"),static_cast<unsigned long>(idx));

        if (m_vids[idx]) {
            VID* old=m_vids[idx];
            delete old;
            m_vids[idx]=0;
            Error(5,const_cast<char*>("this VID already loaded"),static_cast<unsigned long>(idx));
        }

        m_vids[idx]=CreateVid(res,idx);
        if (!m_vids[idx])
            continue;
        if (idx>=m_noVid)
            m_noVid=idx+1;
        if (hadVids)
            m_vids[idx]->SetExtraType();

        VID* vid=m_vids[idx];
        if (vid->m_weaponIndex<m_noWeapon) {
            vid->m_weapon=reinterpret_cast<WEAPON*>(
                reinterpret_cast<unsigned char*>(m_weapon)+vid->m_weaponIndex*0x23C);
        } else {
            vid->Error(10,const_cast<char*>("nWeapon > noWeapon"),static_cast<unsigned long>(vid->m_weaponIndex));
            vid->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);
        }
        Graph->DrawLoadBar(m_vids[0]);
    } while (res->GoNextSub(0x204A424Fu)==0);

    int maxX=0,maxY=0;
    for (idx=0;idx<m_noVid;++idx) {
        VID* vid=m_vids[idx];
        if (!vid)
            continue;
        vid->SetChildAndLink();
        if (vid->m_regionTileStepX>maxX) maxX=vid->m_regionTileStepX;
        if (vid->m_regionTileStepY>maxY) maxY=vid->m_regionTileStepY;
    }
    MYERROR::Log(::Error,
        "LoadVid::No   =%-15i   sizeof(VID)   =%-5i    load time     =%ims   MaxSizeX,Y=%i,%i",
        m_noVid,0x490,timeGetTime()-start,maxX,maxY);
}

// The resource contains 26 packed 32-bit values. The retail constructor reads
// the slot at +0x28 into a throwaway local because MAP startup overwrites
// debugMode from the profile immediately afterwards.
CONSTANT::CONSTANT(RESOURCE* res)
{
    if (res->GoBegin(0x54534E43u)!=0) { // 'CNST'
        MYERROR::Log(::Error,"!!!ERROR!!! CNST Load Constant section not found");
        return;
    }

    uint32_t discardedDebug=0;
    uint32_t* words=reinterpret_cast<uint32_t*>(this);
    for (int i=0; i<10; ++i)
        res->Read(&words[i],4);
    res->Read(&discardedDebug,4);
    for (int i=11; i<26; ++i)
        res->Read(&words[i],4);

    maxShiftSpeedX/=1000.0f;
    maxShiftSpeedY/=1000.0f;
    gravity/=1000000.0f;
    gravity2/=1000000.0f;
    MasterRepairSpeed/=1000.0f;
    RailRepairSpeed/=1000.0f;
    SafeClashSpeed/=1000.0f;
}

// MapEdit.exe map.cpp:1327.
SPRITE* MAP::NextSpriteByType(int nlayer,int* i,unsigned int type)
{
    SPRITE* sprite=NextSprite(nlayer,i);
    while (sprite && !sprite->IsSpriteType(type))
        sprite=NextSprite(nlayer,i);
    return sprite;
}

// Original inline owner emitted into creature.obj from map.h:366.
SPRITE* MAP::FirstSpriteByType(int nlayer,int* i,unsigned int type)
{
    *i=m_layers[nlayer].No();
    return NextSpriteByType(nlayer,i,type);
}
