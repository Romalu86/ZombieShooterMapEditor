#pragma once
struct IDirectDrawSurface7;
// SURFACE/TEXTURE owners. Included in ABI order by mapedit/runtime.hpp.
class SURFACE {
public:
    virtual ~SURFACE();
    void* m_surface;       // +0x04 IDirectDrawSurface7*
    int m_format;          // +0x08 D3DFORMAT
    int m_sizeX;           // +0x0C
    int m_sizeY;           // +0x10
    int m_reserved14;      // +0x14

    SURFACE();
    SURFACE(int size_x,int size_y,D3DFORMAT format);
    operator IDirectDrawSurface7*();
    int IsExist();
    int IsPaletted();
    int SizeX();
    int SizeY();
    // ZS1 retail header-inline ownership (graph_dx7.h): callers such as
    // VID_SOFTWARE16::DrawToVid and VID_HARDWARE_Z::Draw contain the DirectDraw
    // Lock/Unlock bodies directly rather than calling reconstruction wrappers.
    unsigned char* Lock(int* pitch,RECT_OLD* rect)
    {
        if (!m_surface)
            return 0;
        unsigned char desc[0x7c];
        *reinterpret_cast<unsigned long*>(desc)=0x7cu;
        void** vtable=*reinterpret_cast<void***>(m_surface);
        typedef long (__stdcall *LockMethod)(void*,RECT_OLD*,void*,unsigned long,void*);
        const long hr=reinterpret_cast<LockMethod>(vtable[0x64/4])(m_surface,rect,desc,0,0);
        if (hr) {
            Error(0,"SURFACE::Lock",static_cast<unsigned long>(hr));
            return 0;
        }
        *pitch=*reinterpret_cast<int*>(desc+0x10);
        return *reinterpret_cast<unsigned char**>(desc+0x24);
    }
    void UnLock()
    {
        if (!m_surface)
            return;
        void** vtable=*reinterpret_cast<void***>(m_surface);
        typedef long (__stdcall *UnlockMethod)(void*,void*);
        reinterpret_cast<UnlockMethod>(vtable[0x80/4])(m_surface,0);
    }
    long CopyFromSurface(void* source_surface,RECT_OLD* source,POINT_OLD* dest);
    int MemorySize();
    int Format();
    void SetPalette(COLOR* palette);
    static void Error(int type,const char* text,unsigned long err)
    {
        // graph_dx7.h retail owner: SURFACE::Lock callers expand this wrapper too.
        if (::Error)
            MYERROR::Error(::Error,"TEXTURE",type,text,err);
    }
    static int MemoryInUse();
};

class TEXTURE : public SURFACE {
public:
    unsigned long property; // +0x18
    TEXTURE(int size_x,int size_y,int format,unsigned long property);
    virtual ~TEXTURE();
    void Draw(unsigned long vertex_shader,void* vertex,unsigned int size_one_vertex);
    void Draw(RECT_OLD* screen,RECT_OLD* tex,const GAMMA* gamma);
    void Draw(float z1,float z2,RECT_OLD* screen,RECT_OLD* tex,const GAMMA* gamma);
    void SetTexture(int stage);
    static void SetDeviceCaps(void* d3d_device);
};

char* GetPixelFormat(DDPIXELFORMAT_OLD* format);
char* GetPixelFormat(int format);

