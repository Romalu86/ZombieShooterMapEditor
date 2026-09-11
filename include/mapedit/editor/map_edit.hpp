#pragma once
// MAP_EDIT owner and tail layout. Included in ABI order by mapedit/runtime.hpp.
class MAP_EDIT : public MAP {
public:
    MAP_EDIT(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init);
    virtual ~MAP_EDIT();
    virtual int Tact() override;
    virtual int WorkWndMessage(HWND__*, unsigned long, unsigned long, unsigned long) override;
    virtual void DeletePointerToSprite(SPRITE*) override;
    virtual void DrawSecondaryInfo() override;
    virtual void Release() override;
    virtual void Load(STRING name) override;
    virtual SPRITE* CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent) override;

    void DrawMapName();
    void ChangeMouseVid(VID* nvid,unsigned int new_type);
    void ChangeMouseCoorWithSnap();
    void Control(INPUT* input);
    SPRITE* InsertUnit(VID* vid,float x,float y,float z,ANGLE direction);
    void DeleteUnit(float x,float y,int spriteType);
    void FillBox(float beginX,float beginY,float endX,float endY);
    void FillRomb(float beginX,float beginY,float endX,float endY);
    void LoadTerrain(STRING filename);
    void CreateNewRail();
    void SetControlPanel(int flag);
    int DialogControlPanel(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogSelectVid(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogMapProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogConvertSprite(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogUnitProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogTextProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogRegionProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogOptions(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int Left();
    int Right();
    int IsLeftPossible();
    int IsRightPossible();
    int CallDialogBox(const STRING* name,DLGPROC_OLD f);

    union {
        uint32_t optionBits;
        struct {
            unsigned int optDrawMap:1;
            unsigned int optSortVid:1;
            unsigned int optDrawGroup:1;
            unsigned int optDelete:1;
            unsigned int optGround0:1;
            unsigned int optSnap:1;
            unsigned int optChessSnap:1;
            unsigned int optAirBrush:1;
            unsigned int optRandomDir:1;
            unsigned int optTacticMode:1;
            unsigned int optControlPanel:1; // bit 10
            unsigned int optZS1Unknown11:1;
            unsigned int optDrawName:1;      // bit 12, target ctor/dtor Registry "DrawName"
            unsigned int optionUnused:19;
        };
    };                              // +0x4B28
    int optAirBrushSize;            // +0x4B2C
    unsigned int optAirBrushDensity;// +0x4B30
    unsigned int optBigStepZ;       // +0x4B34
    int optGridLineX;               // +0x4B38, target default 0x300
    int optGridLineY;               // +0x4B3C, target default 0x300
    int optShiftSnapX;              // +0x4B40
    int optShiftSnapY;              // +0x4B44
    SPRITE_LIST selectedSprites;    // +0x4B48
    int spriteType;                 // +0x4B58
    uint32_t zs1EditorValue[12];    // +0x4B5C..+0x4B8B; target ctor clears 12 DWORD values
    uint8_t zs1EditorGap64[0x10];   // +0x4B8C..+0x4B9B; semantics still pending
    float insertZ;                  // +0x4B9C; proven by Tact x87 read/write xrefs
    UNDO undo;                      // +0x4BA0..+0x4BF3
    REGION* curRegion;              // +0x4BF4; proven by Control REGION create/delete xrefs
    HWND__* hToolBar;               // +0x4BF8
    HWND__* hControlPanel;          // +0x4BFC
    STRING editFileName;            // +0x4C00
    VID* savedVid;                  // +0x4C04; tactical-mode previous/current mouse VID
    int savedGround0;               // +0x4C08; tactical-mode saved option bit 4
    float savedInsertZ;             // +0x4C0C; tactical-mode saved insertZ
    int convertDrawingVid[5];       // +0x4C10..+0x4C23; ZS1 editor registry convertDrawingVid0..4
    STRING nameDrawingVid[5];       // +0x4C24..+0x4C37; ZS1 editor registry nameDrawingVid0..4
};
struct MAP_EDIT_TAIL_LAYOUT_CHECK {
    uint8_t base[0x4B28];
    uint32_t options;                   // +0x4B28
    int airSize;                        // +0x4B2C
    unsigned int airDensity;            // +0x4B30
    unsigned int bigStepZ;              // +0x4B34
    int gridLineX;                      // +0x4B38
    int gridLineY;                      // +0x4B3C
    int shiftSnapX;                     // +0x4B40
    int shiftSnapY;                     // +0x4B44
    uint8_t selected[0x10];             // +0x4B48
    int spriteType;                     // +0x4B58
    uint32_t values[12];                // +0x4B5C..+0x4B8B
    uint8_t gap64[0x10];                // +0x4B8C..+0x4B9B
    float insertZ;                      // +0x4B9C
    uint8_t undo[0x54];                 // +0x4BA0
    uint32_t region;                    // +0x4BF4
    uint32_t toolbar;                   // +0x4BF8
    uint32_t controlPanel;              // +0x4BFC
    uint32_t filename;                  // +0x4C00
    uint32_t savedVid;                  // +0x4C04
    int savedGround0;                   // +0x4C08
    float savedInsertZ;                 // +0x4C0C
    int convertDrawingVid[5];           // +0x4C10
    uint32_t tailStrings[5];            // +0x4C24
};
