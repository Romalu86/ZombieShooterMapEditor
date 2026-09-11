#pragma once
// GRAPH_INIT/GAMMA/COLOR owners. Included in ABI order by mapedit/runtime.hpp.
struct GRAPH_INIT {
    char gameName[256];
    uint32_t screenModeX[32]; uint32_t screenModeY[32]; uint32_t screenModeColorDepth[8];
    uint32_t options; uint32_t defaultDevice;
    int defaultScreenX,defaultScreenY,defaultColorDepth,defaultFullScreen;
    int HaveMode(unsigned int size_x,unsigned int size_y,unsigned int color_depth);
};

class COLOR;
class GAMMA {
public:
    enum GAMMA_CREATE { DECODE };
    uint32_t subtractive;
    uint32_t additive;

    GAMMA() : subtractive(0), additive(0) {}
    GAMMA(int red,int green,int blue);
    GAMMA(int alpha,int red,int green,int blue);
    GAMMA(GAMMA c1,GAMMA c2);
    GAMMA(COLOR diffuseColor,COLOR specularColor);
    GAMMA(const GAMMA* other);
    GAMMA(GAMMA_CREATE type,unsigned long gamma);
    const GAMMA* SetAlpha(int alpha);
    const GAMMA* SetRed(int red);
    const GAMMA* SetGreen(int green);
    const GAMMA* SetBlue(int blue);
    int Alpha();
    int Red();
    int Green();
    int Blue();
    unsigned long EncodeToDword();
    const GAMMA* operator=(const GAMMA* other);
    const GAMMA operator+(const GAMMA* other);
    const GAMMA* operator+=(const GAMMA* other);
    int operator==(const GAMMA* other);
    int IsDefault();
    unsigned long Diffuse();
    unsigned long Specular();
    int Write(STREAM* res);
    int Read(STREAM* res);
};

GAMMA InterpolateGamma(const GAMMA* gamma1,const GAMMA* gamma2,float interpolation);

class RGB16;
struct RGB565 { uint16_t color; };
struct RGB555 {
    uint16_t color;
    RGB555(const COLOR* r);
    RGB555(const RGB565* r);
};
class COLOR {
public:
    uint32_t color;
    // basetype.h source shape: intentionally empty and header-visible.
    // PICTURE_BASE owns COLOR palette[256]; retail ctor emits no palette-init loop.
    COLOR() {}
    COLOR(int red,int green,int blue) {
        if (red<0) red=0; else if (red>255) red=255;
        if (green<0) green=0; else if (green>255) green=255;
        if (blue<0) blue=0; else if (blue>255) blue=255;
        color=(static_cast<unsigned int>(red)<<16)|0xFF000000u|
              (static_cast<unsigned int>(green)<<8)|static_cast<unsigned int>(blue);
    }
    COLOR(int alpha,int red,int green,int blue);
    COLOR(const COLOR& r) : color(r.color) {}
    COLOR(const RGB16* r);
    COLOR(const RGB555* r);
    COLOR(const GAMMA* gamma,const COLOR* col);
    unsigned int Red() { return (color>>16)&0xFFu; }
    unsigned int Green() { return (color>>8)&0xFFu; }
    unsigned int Blue() { return color&0xFFu; }
    unsigned int Alpha() { return color>>24; }
    unsigned int ARGB32();
    unsigned int RGB24();
    unsigned int RGB565();
    int NearestInPalette(const COLOR* palette,int no_palette);
    int Diff(const COLOR* col2) {
        const int dr=static_cast<int>(Red()>>3)-static_cast<int>(const_cast<COLOR*>(col2)->Red()>>3);
        int result=Sqr(dr)*64*59*59;
        const int dg=static_cast<int>(Green()>>2)-static_cast<int>(const_cast<COLOR*>(col2)->Green()>>2);
        result+=Sqr(dg)*16*30*30;
        const int db=static_cast<int>(Blue()>>3)-static_cast<int>(const_cast<COLOR*>(col2)->Blue()>>3);
        result+=Sqr(db)*16*11*11;
        const int da=static_cast<int>(Alpha()/15u)-static_cast<int>(const_cast<COLOR*>(col2)->Alpha()/15u);
        result+=Sqr(da)*225*30;
        return result;
    }
    int Sqr(int diff) { return diff*diff; }
    COLOR AlphaAdd(COLOR r,unsigned int alpha);
    COLOR PrepareForOutputAlpha();
    COLOR SetAlpha(int alpha) {
        if (alpha<0) alpha=0;
        else if (alpha>255) alpha=255;
        color=(color&0x00FFFFFFu)|(static_cast<unsigned int>(alpha)<<24);
        return *this;
    }
    void Write(STREAM* stream);
    void Read(STREAM* stream);
    const COLOR* operator=(const COLOR* r) { color=r->color; return this; }
    int operator==(const COLOR* r);
};

class RGB16 {
public:
    uint16_t color;
    // ZS1 retail stores/loads both masks as DWORD globals (GRAPH::Init
    // 0x0041ECFD/0x0041ED07 and RGB16 pack loops), not WORD globals.
    static unsigned int rMask;
    static unsigned int gMask;
    static int rShift;
    static int gShift;
    RGB16();
    RGB16(const COLOR* r) {
        const uint32_t c=r->color;
        color=static_cast<uint16_t>(((c>>(16-rShift))&rMask)|
                                    ((c>>(8-gShift))&gMask)|
                                    ((c>>3)&0x1Fu));
    }
    RGB16(const RGB16* r);
    // ZS1 GRAPH::Init 0x0041ED47..0x0041F0EA proves this basetype owner
    // is header-visible: all eight snow-ramp constructions are folded into
    // GRAPH::Init rather than calling an external RGB16(int,int,int) body.
    RGB16(int red,int green,int blue) {
        if (red<0) red=0; else if (red>255) red=255;
        if (green<0) green=0; else if (green>255) green=255;
        if (blue<0) blue=0; else if (blue>255) blue=255;
        // ZS1 retail pack shape shifts the already-clamped channels directly.
        // Building an intermediate RGB24 DWORD changes VC6's owner substantially.
        color=static_cast<uint16_t>(((static_cast<unsigned int>(red)<<rShift)&rMask) |
                                    ((static_cast<unsigned int>(green)<<gShift)&gMask) |
                                    (static_cast<unsigned int>(blue)>>3));
    }
    // ZS1 GRAPH::Init 0x0041ECF1..0x0041ED47 writes these four globals
    // directly in the caller; keep the historical basetype helper inline.
    static void SetFormat(int rgb565) {
        if (rgb565) {
            rMask=0xF800u; gMask=0x07E0u; rShift=8; gShift=3;
        } else {
            rMask=0x7C00u; gMask=0x03E0u; rShift=7; gShift=2;
        }
    }
};
inline COLOR::COLOR(const RGB16* r)
{
    const unsigned int c=r->color;
    const unsigned int red=(c<<(16-RGB16::rShift))&0x00FF0000u;
    const unsigned int green=(c<<(8-RGB16::gShift))&0x0000FF00u;
    const unsigned int blue=(c<<3)&0x000000FFu;
    color=0xFF000000u|red|green|blue;
}


