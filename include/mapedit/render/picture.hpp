#pragma once
// PICTURE owner family. Included in ABI order by mapedit/runtime.hpp.
class PICTURE_BASE {
public:
    PICTURE_BASE();
    PICTURE_BASE(int size_x,int size_y,int bytes_per_pixel);
    virtual ~PICTURE_BASE();
    virtual void NextFrame();
    virtual void Rewind();
    virtual int Load(const STRING* filename);
    virtual void Close();

    // ZS1 target PICTURE_BASE layout (0x42C bytes including the vptr).
    // These are real recovered owners, replacing the old opaque byte block.
    int noFrame;             // +0x004
    int curFrame;            // +0x008
    int frameTime;           // +0x00C
    int sizeX;               // +0x010
    int sizeY;               // +0x014
    int bytesPerPixel;       // +0x018
    STRING filename;         // +0x01C
    FILE* file;              // +0x020
    COLOR palette[256];      // +0x024
    void* data;              // +0x424
    long dataOffset;         // +0x428

    void Error(TYPE_ERROR type,char* text,unsigned long err);
    void SetSize(int size_x,int size_y,int bytes_per_pixel);
    int SizeX() { return sizeX; }
    int SizeY() { return sizeY; }
    int IsPaletted() { return bytesPerPixel==1; }
    int IsOpened();
    int InViewPort(int x,int y) { return x>=0 && y>=0 && x<sizeX && y<sizeY; }
    int BytesPerPixel();
    int FrameTime();
    int CurFrame();
    int NoFrame() { return noFrame; }
    const COLOR* GetPalette();
    void SetPalette(const COLOR* palette);
    COLOR GetPixel(int x,int y);
    unsigned int GetData(int x,int y);
    void PutData(int x,int y,unsigned int data);
    void PutPixel(int x,int y,COLOR color);
    void SaveTGA(const STRING* filename,int x,int y,int sizeX,int sizeY);
    STRING FileName();
};

class PICTURE {
public:
    enum PICTURE_TYPE { TYPE_UNKNOWN=0, TYPE_TGA=1, TYPE_BMP=2, TYPE_FLC=3, TYPE_VID=4, TYPE_Z=5, TYPE_JPG=6 };
    PICTURE();
    PICTURE(int size_x,int size_y,PICTURE_TYPE create_type);
    virtual ~PICTURE();
    PICTURE_BASE* picture;
    int type;
    int Load(const STRING* filename);
    void Close();
    void Rewind() { picture->Rewind(); }
    void NextFrame() { picture->NextFrame(); }
    int IsZ();
    int IsOpened() { return picture->IsOpened(); }
    int InViewPort(int x,int y) { return picture->InViewPort(x,y); }
    int SizeX() { return picture->SizeX(); }
    int SizeY() { return picture->SizeY(); }
    int NoFrame() { return picture->NoFrame(); }
    int CurFrame();
    int FrameTime();
    int IsPaletted();
    int BytesPerPixel() { return picture->BytesPerPixel(); }
    const COLOR* GetPalette();
    void SetPalette(const COLOR* palette);
    void SetSize(int size_x,int size_y,int bytes_in_pixel);
    COLOR GetPixel(int x,int y);
    unsigned int GetData(int x,int y);
    void PutData(int x,int y,unsigned int data);
    void PutPixel(int x,int y,COLOR color);
    void SaveTGA(const STRING* filename,int x,int y,int sizeX,int sizeY);
    STRING FileName() { return picture->filename; }
};

class PICTURE_MAKEVID {
public:
    PICTURE_MAKEVID();
    PICTURE_MAKEVID(int size_x,int size_y,unsigned long create_layer);
    virtual ~PICTURE_MAKEVID();                                  // vtable +0x00
    virtual void NextFrame();                                    // +0x04
    virtual void Rewind();                                       // +0x08
    virtual int Load(STRING file,STRING alphafile,STRING zfile); // +0x0C
    virtual void Close();                                        // +0x10
    PICTURE texture;                 // +0x004
    PICTURE alpha;                   // +0x010
    PICTURE zBuffer;                 // +0x01C
    COLOR commonPalette[256];        // +0x028
    unsigned char* paletteDecode;    // +0x428
    unsigned int vidType;            // +0x42C
    int CalcCRC32();
    int SizeX() { return texture.SizeX(); }
    int SizeY() { return texture.SizeY(); }
    int NoFrame() { return texture.NoFrame(); }
    int CurFrame();
    int IsPaletted();
    int InViewPort(int x,int y) { return texture.InViewPort(x,y); }
    const COLOR* GetPalette();
    unsigned int GetData(int x,int y);
    void PutData(int x,int y,unsigned int data);
    COLOR GetPixel(int x,int y);
    unsigned int GetAlpha(int x,int y);
    short GetPixelZ(int x,int y);
    short GetPixelZInRect(int x,int y,int width,int height);
    unsigned long GetPixelT(int x,int y);
    void GetRectangle(int* begx,int* begy,int* endx,int* endy);
    int IsPixel(int x,int y);
    int IsPixelInRect(int x,int y,int width,int height);
    int WriteSurfaces(unsigned char* outbuf,unsigned short* buf,int surf_sizex,int surf_sizey);
    void WriteHardware(RESOURCE* res);
    int GetShadow(void* buffer);
    int GetSoftwareRectangle(void* buffer,int x0,int y0,int x1,int y1);
    void CreateOnePalette();
    void WriteSoftware(RESOURCE* res);
    void WritePseudo3d(RESOURCE* res);
    void WriteLight(RESOURCE* res);
    int MakeVid(unsigned int opt,STRING vidname);
    int GetPaletteNumber(COLOR color);
    void SetPaletteDecodeNumber(COLOR color,unsigned char number);
    void PutPixel(int x,int y,COLOR color);
    void PutPixelZ(int x,int y,int z);
    void Error(TYPE_ERROR type,char* text,unsigned long err) {
        STRING file=texture.FileName();
        MYERROR::Error(::Error,"PICTURE '%s'",static_cast<int>(type),text,err,file.CharPtr());
    }
};

// MapEdit CodeView: PICTURE_FONT embeds a second complete PICTURE_MAKEVID at
// +0x430.  Its vtable overrides NextFrame/Rewind/Load and reuses base Close.
class PICTURE_FONT : public PICTURE_MAKEVID {
public:
    PICTURE_FONT();
    virtual ~PICTURE_FONT();
    virtual void NextFrame();
    virtual void Rewind();
    virtual int Load(STRING file,STRING alphafile,STRING zfile);

    PICTURE_MAKEVID font; // +0x430
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

