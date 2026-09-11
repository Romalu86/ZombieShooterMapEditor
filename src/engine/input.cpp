#include "mapedit/runtime.hpp"

namespace {
enum : unsigned long {
    WM_MOVE_OLD = 0x0003,
    WM_ACTIVATEAPP_OLD = 0x001C,
    WM_NCHITTEST_OLD = 0x0084,
    WM_KEYDOWN_OLD = 0x0100,
    WM_KEYUP_OLD = 0x0101,
    WM_CHAR_OLD = 0x0102,
    WM_SYSKEYDOWN_OLD = 0x0104,
    WM_SYSKEYUP_OLD = 0x0105,
    WM_INITMENU_OLD = 0x0116,
    WM_LBUTTONDOWN_OLD = 0x0201,
    WM_LBUTTONUP_OLD = 0x0202,
    WM_RBUTTONDOWN_OLD = 0x0204,
    WM_RBUTTONUP_OLD = 0x0205,
    WM_MBUTTONDOWN_OLD = 0x0207,
    WM_MOUSEWHEEL_OLD = 0x020A
};

// INPUT::WorkWndMessage reads these GRAPH_CORE members directly in retail
// (ZS1 0x0043B020..0x0043B136).  Keep the local view deliberately narrow:
// it documents only the four fields this owner actually touches and avoids
// changing global GRAPH helper visibility for unrelated translation units.
struct InputGraphViewportFields {
    unsigned char reserved000[0x224];
    float viewXMin;
    float viewXMax;
    float viewYMin;
    float viewYMax;
};
}

// This is the retail Win32 input router.  The key/mouse binding globals are
// writable configuration state in the original image, so they are kept as
// globals rather than folded to their retail startup constants.
int INPUT::WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam)
{
    switch (msg) {
    case WM_MOVE_OLD: {
        RECT_OLD rect;
        GetWindowRect(hwnd,&rect);
        g_windowScreenX=static_cast<int>(rect.left);
        g_windowScreenY=static_cast<int>(rect.top);
        break;
    }
    case WM_NCHITTEST_OLD: {
        RECT_OLD rect;
        GetWindowRect(hwnd,&rect);

        // Retail masks both halves to 16 bits and does not sign-extend them.
        // Retail uses signed-dword FILD after masking each coordinate to 16 bits.
        // The values are 0..65535, so an int temporary reproduces that x87 source shape
        // without the unsigned-to-float QWORD conversion emitted by the reconstruction.
        const int rawX=static_cast<int>(lParam&0xFFFFu);
        const int rawY=static_cast<int>((lParam>>16)&0xFFFFu);
        float sx=static_cast<float>(rawX)-static_cast<float>(rect.left);
        float sy=static_cast<float>(rawY)-static_cast<float>(rect.top);
        g_windowScreenX=static_cast<int>(rect.left);
        g_windowScreenY=static_cast<int>(rect.top);
        screenMouseX=sx;
        screenMouseY=sy;

        // Keep GRAPH as a global expression, as in retail.  VC6 then keeps the
        // pre-call viewport pointer only for the clamp and reloads Graph after
        // Mouse::ChangeCoor instead of preserving a reconstruction local in EDI.
        if (sx < reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewXMin)
            sx=reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewXMin;
        if (sx >= reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewXMax)
            sx=reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewXMax-1.0f;
        if (sy < reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewYMin)
            sy=reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewYMin;
        if (sy >= reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewYMax)
            sy=reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewYMax-1.0f;

        // Retail adds MAP's current screen shift directly; there is no
        // FromScreenX/FromScreenY call boundary in this owner.
        mouseX=sx+Map->m_shiftX;
        mouseY=sy+Map->m_shiftY;
        // ZS1 0x0043B0B9..0x0043B0D8: WM_NCHITTEST can be pumped by
        // MAP::StartTact before the global Mouse object is constructed. Retail
        // reads m_z directly and only calls the virtual-coordinate owner.
        if (Mouse)
            Mouse->ChangeCoor(mouseX,mouseY,Mouse->m_z);
        if (screenMouseX>=reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewXMin &&
            screenMouseX<reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewXMax &&
            screenMouseY>=reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewYMin &&
            screenMouseY<reinterpret_cast<const InputGraphViewportFields*>(Graph)->viewYMax)
            return 1;
        break;
    }
    case WM_MOUSEWHEEL_OLD:
        mouseWheel=static_cast<short>((wParam>>16)&0xFFFFu)/120;
        break;

    case WM_LBUTTONDOWN_OLD:
        stateBits |= 0x20u; // lDown
        stateBits |= 0x01u; // lClick
        if (g_inputFirstPrimary==1)
            stateBits |= 0x4000u;
        if (g_inputSecondPrimary==1)
            stateBits |= 0x8000u;
        break;
    case WM_RBUTTONDOWN_OLD:
        stateBits |= 0x40u; // rDown
        stateBits |= 0x04u; // rClick
        if (g_inputFirstPrimary==2)
            stateBits |= 0x4000u;
        if (g_inputSecondPrimary==2)
            stateBits |= 0x8000u;
        break;
    case WM_MBUTTONDOWN_OLD:
        stateBits |= 0x02u;
        break;
    case WM_LBUTTONUP_OLD:
        stateBits &= ~0x20u;
        stateBits |= 0x08u;
        if (g_inputFirstPrimary==1 && g_inputAllowFirst)
            stateBits &= ~0x4000u;
        if (g_inputSecondPrimary==1 && g_inputAllowSecond)
            stateBits &= ~0x8000u;
        break;
    case WM_RBUTTONUP_OLD:
        stateBits &= ~0x40u;
        stateBits |= 0x10u;
        if (g_inputFirstPrimary==2 && g_inputAllowFirst)
            stateBits &= ~0x4000u;
        if (g_inputSecondPrimary==2 && g_inputAllowSecond)
            stateBits &= ~0x8000u;
        break;

    case WM_KEYDOWN_OLD:
        key=wParam<<8;
        vkKey=wParam;
        if (wParam==static_cast<unsigned long>(g_inputKeyLeftPrimary) || wParam==static_cast<unsigned long>(g_inputKeyLeftSecondary))
            stateBits |= 0x0080u;
        else if (wParam==static_cast<unsigned long>(g_inputKeyRightPrimary) || wParam==static_cast<unsigned long>(g_inputKeyRightSecondary))
            stateBits |= 0x0100u;
        else if (wParam==static_cast<unsigned long>(g_inputKeyUpPrimary) || wParam==static_cast<unsigned long>(g_inputKeyUpSecondary))
            stateBits |= 0x0400u;
        else if (wParam==static_cast<unsigned long>(g_inputKeyDownPrimary) || wParam==static_cast<unsigned long>(g_inputKeyDownSecondary))
            stateBits |= 0x0200u;
        else if (wParam==static_cast<unsigned long>(g_inputFirstPrimary) || wParam==static_cast<unsigned long>(g_inputFirstSecondary))
            stateBits |= 0x4000u;
        else if (wParam==static_cast<unsigned long>(g_inputSecondPrimary) || wParam==static_cast<unsigned long>(g_inputSecondSecondary))
            stateBits |= 0x8000u;
        else if (wParam==0x10u)
            stateBits |= 0x0800u;
        else if (wParam==0x11u)
            stateBits |= 0x1000u;
        break;
    case WM_CHAR_OLD:
        key=wParam&0xFFu;
        break;
    case WM_SYSKEYDOWN_OLD:
        if (wParam==0x12u)
            stateBits |= 0x2000u;
        break;
    case WM_KEYUP_OLD:
        if (wParam==static_cast<unsigned long>(g_inputKeyLeftPrimary) || wParam==static_cast<unsigned long>(g_inputKeyLeftSecondary))
            stateBits &= ~0x0080u;
        else if (wParam==static_cast<unsigned long>(g_inputKeyRightPrimary) || wParam==static_cast<unsigned long>(g_inputKeyRightSecondary))
            stateBits &= ~0x0100u;
        else if (wParam==static_cast<unsigned long>(g_inputKeyUpPrimary) || wParam==static_cast<unsigned long>(g_inputKeyUpSecondary))
            stateBits &= ~0x0400u;
        else if (wParam==static_cast<unsigned long>(g_inputKeyDownPrimary) || wParam==static_cast<unsigned long>(g_inputKeyDownSecondary))
            stateBits &= ~0x0200u;
        else if (wParam==static_cast<unsigned long>(g_inputFirstPrimary) || wParam==static_cast<unsigned long>(g_inputFirstSecondary))
            stateBits &= ~0x4000u;
        else if (wParam==static_cast<unsigned long>(g_inputSecondPrimary) || wParam==static_cast<unsigned long>(g_inputSecondSecondary))
            stateBits &= ~0x8000u;
        else if (wParam==0x10u)
            stateBits &= ~0x0800u;
        else if (wParam==0x11u)
            stateBits &= ~0x1000u;
        break;
    case WM_SYSKEYUP_OLD:
        if (wParam==0x12u)
            stateBits &= ~0x2000u;
        break;
    case WM_ACTIVATEAPP_OLD:
        // ZS1 0x0043B142: active-app notifications only reset on deactivation.
        // Retail deliberately falls through to the same reset body used by
        // WM_INITMENU; the 0x0116 case is what makes VC6 emit the sparse
        // 0x116..0x20A switch table seen at 0x0043B3EF.
        if (wParam!=0)
            break;
        // fall through
    case WM_INITMENU_OLD:
        stateBits &= ~0x0800u; // shift
        stateBits &= ~0x2000u; // alt
        stateBits &= ~0x1000u; // ctrl
        key=0;
        break;
    default:
        break;
    }
    return 0;
}

void INPUT::Tact()
{
    stateBits &= ~0x10u; // rUp
    stateBits &= ~0x08u; // lUp
    stateBits &= ~0x02u; // mClick
    stateBits &= ~0x04u; // rClick
    stateBits &= ~0x01u; // lClick
    key = 0;
    vkKey = 0;
    mouseWheel = 0;
    if (!g_inputAllowFirst)
        stateBits &= ~0x4000u;
    if (!g_inputAllowSecond)
        stateBits &= ~0x8000u;
}

void INPUT::ChangeCoor(float screen_x,float screen_y)
{
    SetCursorPos(static_cast<int>(screen_x)+g_windowScreenX,
                 static_cast<int>(screen_y)+g_windowScreenY);
}

// Retail clears only the low 16 input-state bits before the Win32 router begins.
INPUT::INPUT()
    : mouseWheel(0), mouseX(0.0f), mouseY(0.0f),
      screenMouseX(0.0f), screenMouseY(0.0f), key(0), vkKey(0)
{
    // ZS1 0x0043AF60 performs a read/modify/write of only the low input-word:
    // AND [this],0xFFFF0000.  Preserve the upper 16 bits instead of zeroing
    // the complete DWORD as the previous reconstruction did.
    volatile unsigned int* rawState=&stateBits;
    *rawState&=0xFFFF0000u;
}

// Packs the retail input bitfield in the script-visible historical order.
unsigned int INPUT::GetState()
{
    const unsigned int b=stateBits;
    unsigned int result=(b>>0)&1u;
    result|=((b>>2)&1u)<<1;
    result|=((b>>5)&1u)<<2;
    result|=((b>>6)&1u)<<3;
    result|=((b>>11)&1u)<<4;
    result|=((b>>12)&1u)<<5;
    result|=((b>>7)&1u)<<6;
    result|=((b>>8)&1u)<<7;
    result|=((b>>10)&1u)<<8;
    result|=((b>>9)&1u)<<9;
    result|=((b>>14)&1u)<<10;
    result|=((b>>15)&1u)<<11;
    return result;
}

void INPUT::Save(STREAM* res)
{
    res->Write(this,0x20);
}

void INPUT::Load(STREAM* res)
{
    res->Read(this,0x20);
}

void INPUT::ClearLClick()
{
    stateBits&=~0x0001u;
    if (g_inputFirstPrimary==1)
        stateBits&=~0x4000u;
    if (g_inputSecondPrimary==1)
        stateBits&=~0x8000u;
}

void INPUT::ClearRClick()
{
    stateBits&=~0x0004u;
    if (g_inputFirstPrimary==2)
        stateBits&=~0x4000u;
    if (g_inputSecondPrimary==2)
        stateBits&=~0x8000u;
}

// Converts the textual control names accepted by the retail profile into the
// Win32 virtual-key/mouse codes stored in the global bindings.
unsigned int INPUT::StringToKey(STRING key)
{
    key = key.ToUpper();
    const char* s=key.CharPtr();
    if (!strcmp(s,"LBUTTON")) return 1;
    if (!strcmp(s,"RBUTTON")) return 2;
    if (!strcmp(s,"[")) return 0xDB;
    if (!strcmp(s,"]")) return 0xDD;
    if (!strcmp(s,"LEFT")) return 0x25;
    if (!strcmp(s,"RIGHT")) return 0x27;
    if (!strcmp(s,"UP")) return 0x26;
    if (!strcmp(s,"DOWN")) return 0x28;
    if (!strcmp(s,"INSERT")) return 0x2D;
    if (!strcmp(s,"DELETE")) return 0x2E;
    if (!strcmp(s,"HOME")) return 0x24;
    if (!strcmp(s,"END")) return 0x23;
    if (!strcmp(s,"PGUP")) return 0x21;
    if (!strcmp(s,"PGDN")) return 0x22;
    if (!strcmp(s,"SHIFT")) return 0x10;
    if (!strcmp(s,"CTRL")) return 0x11;
    // ZS1 0x0043BC4E..0x0043C049 emits twelve independent STRING
    // comparisons.  Do not collapse F1..F9 into an arithmetic parser: VC6
    // retains every compare/destructor return path in the retail owner.
    if (!strcmp(s,"F1")) return 0x70;
    if (!strcmp(s,"F2")) return 0x71;
    if (!strcmp(s,"F3")) return 0x72;
    if (!strcmp(s,"F4")) return 0x73;
    if (!strcmp(s,"F5")) return 0x74;
    if (!strcmp(s,"F6")) return 0x75;
    if (!strcmp(s,"F7")) return 0x76;
    if (!strcmp(s,"F8")) return 0x77;
    if (!strcmp(s,"F9")) return 0x78;
    if (!strcmp(s,"F10")) return 0x79;
    if (!strcmp(s,"F11")) return 0x7A;
    if (!strcmp(s,"F12")) return 0x7B;
    return static_cast<unsigned int>(static_cast<signed char>(key.FirstChar()));
}
