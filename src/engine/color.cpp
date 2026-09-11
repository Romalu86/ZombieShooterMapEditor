#include "mapedit/runtime.hpp"
#include <mmintrin.h>

GAMMA::GAMMA(int red, int green, int blue)
    : subtractive(0), additive(0)
{
    SetRed(red);
    SetGreen(green);
    SetBlue(blue);
}

GAMMA::GAMMA(int alpha,int red,int green,int blue)
    : subtractive(0), additive(0)
{
    SetAlpha(alpha);
    SetRed(red);
    SetGreen(green);
    SetBlue(blue);
}

// single packed unsigned-saturating add over the two GAMMA dwords, then EMMS.
GAMMA::GAMMA(GAMMA c1,GAMMA c2)
{
    __m64 left=*reinterpret_cast<const __m64*>(&c1.subtractive);
    const __m64 right=*reinterpret_cast<const __m64*>(&c2.subtractive);
    left=_mm_adds_pu8(left,right);
    *reinterpret_cast<__m64*>(&subtractive)=left;
    _mm_empty();
}


int GAMMA::IsDefault()
{
    return subtractive==0 && additive==0;
}

COLOR::COLOR(const GAMMA* gamma,const COLOR* col)
{
    GAMMA* mutableGamma=const_cast<GAMMA*>(gamma);
    COLOR* mutableColor=const_cast<COLOR*>(col);
    if (mutableGamma->IsDefault()) {
        operator=(col);
        return;
    }

    int red=static_cast<int>(mutableColor->Red());
    int green=static_cast<int>(mutableColor->Green());
    int blue=static_cast<int>(mutableColor->Blue());

    red=(red*static_cast<int>(((mutableGamma->Diffuse()>>16)&0xFFu)+1u))>>8;
    red+=static_cast<int>((mutableGamma->Specular()>>16)&0xFFu);
    green=(green*static_cast<int>(((mutableGamma->Diffuse()>>8)&0xFFu)+1u))>>8;
    green+=static_cast<int>((mutableGamma->Specular()>>8)&0xFFu);
    blue=(blue*static_cast<int>((mutableGamma->Diffuse()&0xFFu)+1u))>>8;
    blue+=static_cast<int>(mutableGamma->Specular()&0xFFu);

    COLOR result(red,green,blue);
    operator=(&result);
}

const GAMMA GAMMA::operator+(const GAMMA* other)
{
    return GAMMA(this,other);
}

const GAMMA* GAMMA::operator+=(const GAMMA* other)
{
    GAMMA result(this,other);
    return operator=(&result);
}

GAMMA::GAMMA(COLOR diffuseColor,COLOR specularColor)
    : subtractive(~diffuseColor.ARGB32()), additive(specularColor.ARGB32())
{
}




GAMMA::GAMMA(const GAMMA* other)
    : subtractive(other->subtractive), additive(other->additive)
{
}

COLOR::COLOR(int alpha, int red, int green, int blue)
{
    if (alpha < 0) alpha = 0; else if (alpha > 255) alpha = 255;
    if (red < 0) red = 0; else if (red > 255) red = 255;
    if (green < 0) green = 0; else if (green > 255) green = 255;
    if (blue < 0) blue = 0; else if (blue > 255) blue = 255;

    color = (static_cast<unsigned int>(alpha) << 24) |
            (static_cast<unsigned int>(red) << 16) |
            (static_cast<unsigned int>(green) << 8) |
            static_cast<unsigned int>(blue);
}

const GAMMA* GAMMA::operator=(const GAMMA* other)
{
    subtractive = other->subtractive;
    additive = other->additive;
    return this;
}

const GAMMA* GAMMA::SetRed(int red)
{
    if (red < -255) red = -255;
    else if (red > 255) red = 255;
    subtractive &= 0xFF00FFFFu;
    additive &= 0xFF00FFFFu;
    if (red < 0)
        subtractive |= static_cast<unsigned int>(-red) << 16;
    else
        additive |= static_cast<unsigned int>(red) << 16;
    return this;
}

const GAMMA* GAMMA::SetGreen(int green)
{
    if (green < -255) green = -255;
    else if (green > 255) green = 255;
    subtractive &= 0xFFFF00FFu;
    additive &= 0xFFFF00FFu;
    if (green < 0)
        subtractive |= static_cast<unsigned int>(-green) << 8;
    else
        additive |= static_cast<unsigned int>(green) << 8;
    return this;
}

const GAMMA* GAMMA::SetBlue(int blue)
{
    if (blue < -255) blue = -255;
    else if (blue > 255) blue = 255;
    subtractive &= 0xFFFFFF00u;
    additive &= 0xFFFFFF00u;
    if (blue < 0)
        subtractive |= static_cast<unsigned int>(-blue);
    else
        additive |= static_cast<unsigned int>(blue);
    return this;
}



// in graphics/color.hpp so aggregate palette construction folds away as retail.

unsigned int RGB16::rMask = 0xF800u;
unsigned int RGB16::gMask = 0x07E0u;
int RGB16::rShift = 8;
int RGB16::gShift = 3;

// RGB555/RGB565 are distinct retail source types used by picture/VID conversion
// paths.  RGB16 remains the runtime-selected 555/565 surface type.
RGB555::RGB555(const RGB565* r)
{
    const unsigned int c=r->color;
    color=static_cast<uint16_t>((c&0x001Fu) | ((c>>1)&0x7FE0u));
}

RGB555::RGB555(const COLOR* r)
{
    const unsigned int c=r->color;
    color=static_cast<uint16_t>(((c>>9)&0x7C00u) | ((c>>6)&0x03E0u) | ((c>>3)&0x001Fu));
}

COLOR::COLOR(const RGB555* r)
{
    const unsigned int c=r->color;
    color=0xFF000000u | ((c<<9)&0x00FF0000u) | ((c<<6)&0x0000FF00u) | ((c<<3)&0x000000FFu);
}

RGB16::RGB16() {}



unsigned int COLOR::ARGB32() { return color; }





int COLOR::operator==(const COLOR* r)
{
    return color==r->color;
}



COLOR COLOR::AlphaAdd(COLOR r,unsigned int alpha)
{
    ++alpha;
    const unsigned int inv=256u-alpha;
    const unsigned int red=(alpha*((r.color>>16)&0xFFu)+inv*((color>>16)&0xFFu))>>8;
    const unsigned int green=(alpha*((r.color>>8)&0xFFu)+inv*((color>>8)&0xFFu))>>8;
    const unsigned int blue=(alpha*(r.color&0xFFu)+inv*(color&0xFFu))>>8;
    COLOR result(static_cast<int>(red),static_cast<int>(green),static_cast<int>(blue));
    *this=&result;
    return *this;
}



RGB16::RGB16(const RGB16* r) { color=r->color; }

int GAMMA::operator==(const GAMMA* other)
{
    return subtractive==other->subtractive && additive==other->additive;
}


unsigned long GAMMA::Diffuse()
{
    return ~subtractive;
}

unsigned long GAMMA::Specular()
{
    return additive;
}

unsigned int COLOR::RGB24()
{
    return color&0x00FFFFFFu;
}

unsigned int COLOR::RGB565()
{
    return ((color>>3)&0x001Fu) | ((color>>5)&0x07E0u) | ((color>>8)&0xF800u);
}





int COLOR::NearestInPalette(const COLOR* palette,int no_palette)
{
    int nearest=0;
    int best=9999999;
    for (int i=0;i<no_palette;++i) {
        const int diff=Diff(palette+i);
        if (diff<best) {
            best=diff;
            nearest=i;
        }
    }
    return nearest;
}

COLOR COLOR::PrepareForOutputAlpha()
{
    const unsigned int oldAlpha=Alpha();
    if (oldAlpha>0xEFu)
        SetAlpha(255);
    else
        SetAlpha(static_cast<int>((oldAlpha>>4)<<4));

    const unsigned int alpha=Alpha();
    if (alpha) {
        const unsigned int blue=255u*Blue()/alpha;
        const unsigned int green=255u*Green()/alpha;
        const unsigned int red=255u*Red()/alpha;
        COLOR prepared(static_cast<int>(alpha),static_cast<int>(red),static_cast<int>(green),static_cast<int>(blue));
        *this=&prepared;
    }
    return *this;
}



void COLOR::Write(STREAM* stream)
{
    stream->Write(this,4u);
}

void COLOR::Read(STREAM* stream)
{
    stream->Read(this,4);
}



int GAMMA::Alpha()
{
    return (subtractive & 0xFF000000u) ? -static_cast<int>((subtractive >> 24) & 0xFFu)
                                         : static_cast<int>((additive >> 24) & 0xFFu);
}

const GAMMA* GAMMA::SetAlpha(int alpha)
{
    if (alpha < -255) alpha=-255;
    else if (alpha > 255) alpha=255;
    subtractive &= 0x00FFFFFFu;
    additive &= 0x00FFFFFFu;
    if (alpha < 0)
        subtractive |= static_cast<unsigned int>(-alpha) << 24;
    else
        additive |= static_cast<unsigned int>(alpha) << 24;
    return this;
}

GAMMA InterpolateGamma(const GAMMA* gamma1,const GAMMA* gamma2,float interpolation)
{
    GAMMA* first=const_cast<GAMMA*>(gamma1);
    GAMMA* second=const_cast<GAMMA*>(gamma2);
    return GAMMA(
        static_cast<int>(first->Alpha()+(second->Alpha()-first->Alpha())*interpolation),
        static_cast<int>(first->Red()+(second->Red()-first->Red())*interpolation),
        static_cast<int>(first->Green()+(second->Green()-first->Green())*interpolation),
        static_cast<int>(first->Blue()+(second->Blue()-first->Blue())*interpolation));
}

int GAMMA::Red()
{
    return (subtractive & 0x00FF0000u) ? -static_cast<int>((subtractive >> 16) & 0xFFu)
                                         : static_cast<int>((additive >> 16) & 0xFFu);
}
int GAMMA::Green()
{
    return (subtractive & 0x0000FF00u) ? -static_cast<int>((subtractive >> 8) & 0xFFu)
                                         : static_cast<int>((additive >> 8) & 0xFFu);
}
int GAMMA::Blue()
{
    return (subtractive & 0x000000FFu) ? -static_cast<int>(subtractive & 0xFFu)
                                         : static_cast<int>(additive & 0xFFu);
}

unsigned long GAMMA::EncodeToDword()
{
    unsigned long out=(subtractive >> 1) & 0x7F7F7F7Fu;
    const unsigned long a=additive;
    if (a & 0x000000FFu) out |= ((~a >> 1) & 0x0000007Fu) | 0x00000080u;
    if (a & 0x0000FF00u) out |= ((~a >> 1) & 0x00007F00u) | 0x00008000u;
    if (a & 0x00FF0000u) out |= ((~a >> 1) & 0x007F0000u) | 0x00800000u;
    if (a & 0xFF000000u) out |= ((~a >> 1) & 0x7F000000u) | 0x80000000u;
    return out;
}

// Zombie Shooter 1 retail packed signed-byte GAMMA decoder.
GAMMA::GAMMA(GAMMA_CREATE /*type*/,unsigned long gamma)
    : subtractive(0), additive(0)
{
    // ZS1 0x004493B0 is four explicit signed-byte decodes.  Keeping the
    // four byte lanes visible reproduces the retail unrolled VC6 owner; the
    // reconstruction-time masks/signs arrays and loop were not present.
    if (gamma & 0x00000080u)
        additive |= ((~gamma)<<1) & 0x000000FFu;
    else
        subtractive |= (gamma<<1) & 0x000000FFu;

    if (gamma & 0x00008000u)
        additive |= ((~gamma)<<1) & 0x0000FF00u;
    else
        subtractive |= (gamma<<1) & 0x0000FF00u;

    if (gamma & 0x00800000u)
        additive |= ((~gamma)<<1) & 0x00FF0000u;
    else
        subtractive |= (gamma<<1) & 0x00FF0000u;

    if (gamma & 0x80000000u)
        additive |= ((~gamma)<<1) & 0xFF000000u;
    else
        subtractive |= (gamma<<1) & 0xFF000000u;
}

int GAMMA::Read(STREAM* res)
{
    return res->Read(this,8u);
}
