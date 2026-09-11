#pragma once
// GRAPH_CORE/GRAPH owners. Included in ABI order by mapedit/runtime.hpp.
class GRAPH_CORE {
public:
    explicit GRAPH_CORE(const GRAPH_INIT* init);
    int Init(HWND__* hwnd);
    virtual void SetDefaultState();
    ~GRAPH_CORE();
    enum TYPE_SURFACE { S_PRIMARY=0, S_BACKBUFFER=1, S_TEXTURE=2, S_TEXTURE_SYSTEM=3, S_TEXTURE_DYNAMIC=4, S_ZBUFFER=5, S_ZBUFFER_SOFT=6, S_OFFSCREEN=7 };
    int PreTact();
    int BlitZBuffer();
    void ReLoadPalettes();
    void CheckDriverBugs();
    void RefreshBuffers();
    void* CreateSurface(int width,int height,TYPE_SURFACE type,const DDPIXELFORMAT_OLD* format);
    void* DDraw();
    void* D3DDevice();
    IDirectDrawSurface7* ZBufferSoft();
    HWND__* Wnd();
    void PostTact(int draw);
    void ClearScreen(COLOR color);
    void LockZ();
    void UnLock();
    void UnLockZ();
    void FlipToGDISurface();
    void SetViewPort(float x0,float y0,float x1,float y1);
    int RealZBuffer(float x,float y);
    int InViewPort(float x,float y);
    void* LockSurface(void* surface,int* pitch,RECT_OLD* rect);
    void Lock();
    int BytesPerPixel();
    void Error(int type,const char* text,unsigned long err);
    long SetRenderState(unsigned long state,unsigned long value);
    long CopyToZBuffer(RECT_OLD* screen_rect,RECT_OLD* tex_rect,TEXTURE* tex);
    void DrawPrimitive(unsigned long type,unsigned long vertexShader,void* vertex,unsigned int sizeOneVertex,int noVertex);
    void PutsXY(float x,float y,const char* str,COLOR color);
    void PutsXY(float x,float y,const STRING* str,COLOR color);
    void PutcXY(float x,float y,char c,COLOR color);
    void Effect(int eff,int var1,int var2,int duration);
    void SetFont(const STRING* font_name,int width,int height);
private:
    uint8_t opaque[0xC14]; // compiler vfptr + opaque = CodeView 0xC18
};
class GRAPH : public GRAPH_CORE {
public:
    explicit GRAPH(const GRAPH_INIT* init);
    int Init(HWND__* hwnd);
    void SetDefaultState() override;
    ~GRAPH();
    static COLOR BLACK;
    static COLOR BLUE;
    static COLOR LIGHTBLUE;
    static COLOR RED;
    static COLOR GREEN;
    static COLOR GRAY;
    static COLOR LIGHTRED;
    static COLOR YELLOW;
    static COLOR WHITE;
    void UpdateStartupDialog(DIALOG_COMBO_BOX*,DIALOG_COMBO_BOX*,DIALOG_BUTTON*);
    void Lock();
    void* Lock(int* pitch);
    int BytesPerPixel();
    float ViewXMin(); float ViewYMin(); float ViewXMax(); float ViewYMax();
    int BoxInViewPort(float left,float top,float right,float bottom);
    float SizeX(); float SizeY();
    int IsPaused();
    int CapsFullScreen();
    int CapsNotPalette();
    int Is15Bit();
    RGB16 ColorIntensity(int intensity);
    void Tact(int draw);
    int DrawLoadBar(VID* draw_vid,VID* second_vid=0,int shiftX=0,int shiftY=0);
    void __cdecl DrawDebugText(const char* str,...);
    void OldLoadParameters(STREAM* res);
    void LoadParameters(STREAM* res);
    void DrawEffect(int draw);
    int GetEffectState(int effect);
    void DrawFog(float fx0,float fy0,float fx1,float fy1,int fogBottom,int fogTop,COLOR fogColor,unsigned short* table,int end,int invert);
    void DrawVid(VID* vid,int ncadr,float shiftx,float shifty,float shiftz);
    int DrawPicture(const STRING* filename,float x,float y);
    void DrawLightSource(float shiftx,float shifty,float shiftz,float size_x,float size_y,COLOR color);
    void DrawGroundSnow();
    void DrawRain();
    void DrawSnow();
    void __cdecl PrintfXY(float x,float y,const char* str,...);
    void Box(float x,float y,float xx,float yy,COLOR color);
    void Bar(float x,float y,float x1,float y1,COLOR color);
    void LightBar(float x,float y,float x1,float y1,unsigned int bright);
    long SetAlphaBlend(unsigned long srcBlend,unsigned long destBlend);
    void Line(float x,float y,float x1,float y1,COLOR color);
    void WuLine(float x0,float y0,float x1,float y1,COLOR color);
    void Circle(float xx,float yy,float radius,COLOR color);
    void PutAlphaPixel(float x,float y,COLOR color,unsigned int alpha);
    void PutPixel(float x,float y,COLOR color);
    void PutPixel(int x,int y,COLOR color);
    COLOR GetPixel(float x,float y);
    void SavePict(PICTURE* pict,int pict_x,int pict_y,int x,int y,int sizex,int sizey);
    void SavePictAndZ(PICTURE_MAKEVID* pict,int pict_x,int pict_y,int x,int y,int sizex,int sizey);
    void SaveTGA(const STRING* file,int x,int y,int sizex,int sizey);
    void SaveZ(const STRING* file,int x,int y,int sizex,int sizey);
    void PutBigPixel(float x,float y,COLOR color);
    void BeginPause();
    void EndPause();
    void LockZ();
    unsigned short* LockZ(int* pitch);
    GAMMA GetGamma();
    TEXTURE* AlphaBuffer();
    void SetGamma(const GAMMA* gamma);
    void DrawSquall();
    void ShadowLine(float x,float y,float x1,float y1,int shadow);
    void ShadowBar(float x,float y,float x1,float y1,int shadow);
    void LightLine(float x,float y,float x1,float y1,unsigned int bright);
    void StopMovie();
    void PlayMovie(const STRING* file);
    int IsPlayMovie();
    float WindSpeed();
    ANGLE WindDirection();
    int IsEnvironment(unsigned int environment);
    void SetWind(int speed,ANGLE direction);
    void SetEnvironment(unsigned int env);
    void SaveParameters(STREAM* res);
private:
    STRING debugText; // +0xC18
};


// Raw offsets are the recovered GRAPH_CORE ABI fields hidden behind the opaque
// layout above; keeping these bodies header-visible lets native VC6 /Ob1 make
// the same caller-local expansions without reconstruction-only wrapper calls.
inline void GRAPH_CORE::Error(int type,const char* text,unsigned long err)
{
    if (::Error)
        MYERROR::Error(::Error,"GRAPH",type,text,err);
}

inline void* GRAPH_CORE::LockSurface(void* surfaceObject,int* pitch,RECT_OLD* rect)
{
    uint8_t desc[0x7c];
    *reinterpret_cast<uint32_t*>(desc)=0x7cu;
    void** const vtable=*reinterpret_cast<void***>(surfaceObject);
    typedef long (__stdcall *LockMethod)(void*,RECT_OLD*,void*,unsigned long,void*);
    const long hr=reinterpret_cast<LockMethod>(vtable[0x64/4])(surfaceObject,rect,desc,0,0);
    if (hr) {
        Error(0,"LockSurface",static_cast<unsigned long>(hr));
        return 0;
    }
    *pitch=*reinterpret_cast<int*>(desc+0x10);
    return *reinterpret_cast<void**>(desc+0x24);
}

inline int GRAPH_CORE::BytesPerPixel()
{
    const uint32_t flags=*reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(this)+0x04);
    return (flags&2u) ? 4 : 2;
}

inline void GRAPH_CORE::Lock()
{
    uint8_t* const raw=reinterpret_cast<uint8_t*>(this);
    void*& colorBuffer=*reinterpret_cast<void**>(raw+0x20C);
    if (colorBuffer)
        return;
    int& colorPitch=*reinterpret_cast<int*>(raw+0x218);
    void* const colorSurface=*reinterpret_cast<void**>(raw+0xC10);
    colorBuffer=LockSurface(colorSurface,&colorPitch,0);
    colorPitch/=BytesPerPixel();
}

inline void GRAPH_CORE::LockZ()
{
    uint8_t* const raw=reinterpret_cast<uint8_t*>(this);
    unsigned short*& zBuffer=*reinterpret_cast<unsigned short**>(raw+0x21C);
    if (zBuffer)
        return;
    int& zPitch=*reinterpret_cast<int*>(raw+0x220);
    void* const zSurface=*reinterpret_cast<void**>(raw+0xC04);
    zBuffer=static_cast<unsigned short*>(LockSurface(zSurface,&zPitch,0));
    zPitch/=2;
}

inline int GRAPH_CORE::InViewPort(float x,float y)
{
    const uint8_t* const raw=reinterpret_cast<const uint8_t*>(this);
    const float xMin=*reinterpret_cast<const float*>(raw+0x224);
    const float xMax=*reinterpret_cast<const float*>(raw+0x228);
    const float yMin=*reinterpret_cast<const float*>(raw+0x22C);
    const float yMax=*reinterpret_cast<const float*>(raw+0x230);
    return x>=xMin && x<xMax && y>=yMin && y<yMax;
}

inline void GRAPH::Lock()
{
    GRAPH_CORE::Lock();
}

inline void* GRAPH::Lock(int* pitch)
{
    GRAPH_CORE::Lock();
    uint8_t* const raw=reinterpret_cast<uint8_t*>(this);
    *pitch=*reinterpret_cast<int*>(raw+0x218);
    return *reinterpret_cast<void**>(raw+0x20C);
}

inline int GRAPH::BytesPerPixel()
{
    return GRAPH_CORE::BytesPerPixel();
}

inline TEXTURE* GRAPH::AlphaBuffer()
{
    // ZS1 VID_HARDWARE_Z::Draw 0x0042E349..0x0042E379 reads the
    // GRAPH +0xBE8 TEXTURE owner directly, then immediately expands SURFACE::Lock.
    return *reinterpret_cast<TEXTURE**>(reinterpret_cast<uint8_t*>(this)+0xBE8);
}

inline GAMMA GRAPH::GetGamma()
{
    // ZS1 VID_HARDWARE_Z::Draw 0x0042E8D1..0x0042E8E8 reads the two
    // GAMMA dwords at GRAPH +0xBAC directly.  Returning the object itself
    // preserves the retail bitwise value return; constructing GAMMA(pointer)
    // here creates a call boundary that is absent from the target owner.
    return *reinterpret_cast<const GAMMA*>(reinterpret_cast<const uint8_t*>(this)+0xBAC);
}

inline void GRAPH::LockZ()
{
    GRAPH_CORE::LockZ();
}

inline unsigned short* GRAPH::LockZ(int* pitch)
{
    GRAPH_CORE::LockZ();
    uint8_t* const raw=reinterpret_cast<uint8_t*>(this);
    *pitch=*reinterpret_cast<int*>(raw+0x220);
    return *reinterpret_cast<unsigned short**>(raw+0x21C);
}
