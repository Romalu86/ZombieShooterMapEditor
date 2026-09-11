#pragma once
// VID_HARDWARE owner. Included in ABI order by mapedit/runtime.hpp.
class VID_HARDWARE : public VID {
public:
    VID_HARDWARE();
    explicit VID_HARDWARE(VID_HARDWARE* source);
    VID_HARDWARE(int nvid,int size_x,int size_y);
    virtual ~VID_HARDWARE();
    virtual VID* CreateMirror();
    virtual void DrawVidToVid(const SPRITE* sprite);
    virtual void Draw(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetLayer();

    VID_TEXCOOR* m_texCoor; // +0x490
    short m_noSurf;         // +0x494
    TEXTURE** m_textures;   // +0x498 (compiler inserts +0x496..+0x497 padding)
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif


// ZS1-derived software/light VID family required by MAP::CreateVid.
// These declarations preserve the genuine MSVC inheritance/vtable shape;
// large raster bodies are integrated only when their retail ASM is closed.
