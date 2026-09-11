#include "mapedit/runtime.hpp"

// MapEditZS1.exe constructor owner. The active body keeps the already recovered
// toolbar/editor setup and now includes the ZS1-only drawing-VID conversion registry route.
MAP_EDIT::MAP_EDIT(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init)
    : MAP(instance,prev,command_line,sw,init), selectedSprites(), undo(), editFileName()
{
    if (!IsInitSuccess()) return;

    spriteType=2;
    for (int i=0;i<12;++i) zs1EditorValue[i]=0;
    insertZ=0.0f;
    curRegion=0;
    hToolBar=0; hControlPanel=0;
    optDrawMap=0; optDrawGroup=0; optTacticMode=0; optZS1Unknown11=0;
    optGround0=1; optSnap=0; optChessSnap=0;
    optRandomDir=0; optDelete=0; optSortVid=0;
    optAirBrush=0; optAirBrushSize=10; optAirBrushDensity=20;
    optShiftSnapX=0; optShiftSnapY=0;
    optDrawMap=Registry->GetInt(STRING("DrawMap"),(int)optDrawMap);
    optBigStepZ=(unsigned int)Registry->GetInt(STRING("BigStepZ"),90);
    optGridLineX=Registry->GetInt(STRING("GridLineX"),0x300);
    optGridLineY=Registry->GetInt(STRING("GridLineY"),0x300);
    optDrawName=Registry->GetInt(STRING("DrawName"),0) ? 1u : 0u;
    optControlPanel=0;
    savedVid=0;

    // ZS1-only MapEdit drawing VID conversion slots (target ctor 0x0040515C..0x00405524).
    // Each slot is stored in registry as convertDrawingVidN/nameDrawingVidN. When a
    // valid NVID and external VID resource are configured, retail copies the five
    // HEAD shorts into that already-loaded VID and then calls its virtual Load().
    for (int i=0;i<5;++i) {
        const STRING suffix=Printf("%i",i);
        const STRING convertKey=STRING("convertDrawingVid")+suffix;
        const STRING nameKey=STRING("nameDrawingVid")+suffix;
        convertDrawingVid[i]=Registry->GetInt(convertKey,0);
        nameDrawingVid[i]=Registry->GetString(nameKey,STRING(""));

        const int nvid=convertDrawingVid[i];
        if (nvid<=0 || nvid>=m_noVid || !m_vids[nvid] || nameDrawingVid[i]=="")
            continue;

        RESOURCE drawingVid;
        if (drawingVid.OpenForRead(&nameDrawingVid[i],0x20444956u)!=0) { // 'VID '
            MYERROR::Log(::Error,"LOAD::Can't open file %s",nameDrawingVid[i].CharPtr());
            continue;
        }

        drawingVid.GoBegin(0x44414548u); // 'HEAD'
        drawingVid.Read(&m_vids[nvid]->m_extraTypeFlags,2u);
        drawingVid.Read(&m_vids[nvid]->m_phaseRandomInterval,2u);
        drawingVid.Read(&m_vids[nvid]->m_dotFrameCount,2u);
        drawingVid.Read(&m_vids[nvid]->m_regionTileStepX,2u);
        drawingVid.Read(&m_vids[nvid]->m_regionTileStepY,2u);
        m_vids[nvid]->Load(&drawingVid);
    }

    // 34 initializers are present in original source/codegen; CreateToolbarEx receives 33.
    // Preserve that asymmetry exactly instead of "fixing" it.
    TBBUTTON_OLD buttons[34]={
        {0,45087,4,0,{0,0},0,-1}, {1,40001,4,0,{0,0},0,-1}, {2,40002,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {3,40078,4,0,{0,0},0,-1}, {4,45092,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {5,40025,4,2,{0,0},0,-1}, {6,40034,4,2,{0,0},0,-1},
        {7,40053,4,2,{0,0},0,-1}, {8,45085,5,2,{0,0},0,-1}, {9,45097,4,2,{0,0},0,-1},
        {10,45099,4,2,{0,0},0,-1}, {0,0,4,1,{0,0},0,-1}, {11,40047,4,6,{0,0},0,-1},
        {12,40048,5,6,{0,0},0,-1}, {13,40049,4,6,{0,0},0,-1}, {14,40050,4,6,{0,0},0,-1},
        {15,45091,4,6,{0,0},0,-1}, {16,40051,4,6,{0,0},0,-1}, {17,45090,4,6,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {18,40032,4,2,{0,0},0,-1}, {0,0,4,1,{0,0},0,-1},
        {19,40067,4,2,{0,0},0,-1}, {0,0,4,1,{0,0},0,-1}, {20,40073,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {21,40061,4,0,{0,0},0,-1}, {22,40062,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {23,45098,4,0,{0,0},0,-1}, {24,41107,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}
    };

    hToolBar=CreateToolbarEx(m_hWnd,0x50800100u,0x73,25,instance,0x73,buttons,33,0,0,0,0,sizeof(TBBUTTON_OLD));
    RECT_OLD rect;
    GetWindowRect(hToolBar,&rect);
    Graph->SetViewPort(Graph->ViewXMin(),Graph->ViewYMin()+float(rect.bottom-rect.top),Graph->ViewXMax(),Graph->ViewYMax());

    SetControlPanel(Registry->GetInt(STRING("ControlPanel"),0));

    // ZS1 0x00405E3B..0x00405E45: the editor switches the MAP-owned mouse
    // from hardware-cursor mode to the software VID cursor immediately after
    // control-panel restoration and before the HideVids registry block.
    Mouse->HardwareOff();

    const int no_hide=Registry->GetInt(STRING("NoHideVids"),0);
    if (no_hide) {
        unsigned int hide[MAPEDIT_MAX_VID];
        Registry->GetData(STRING("HideVids"),hide,(unsigned long)(no_hide*4));
        for (int i=0;i<no_hide;i++)
            if (m_vids[hide[i]]) m_vids[hide[i]]->SetPropHide(1);
    }
    Load(STRING(m_startupLoad));

    // ZS1 0x00405FFD..0x00406024: after the initial map load retail restores
    // the editor cursor/VID selection through ChangeMouseVid.  VID #1 is used
    // when present; otherwise the global EmptyVid fallback is passed together
    // with the current editor sprite type.  This also repopulates the remembered
    // VID slot used when leaving tactical mode.
    VID* initialMouseVid=(m_noVid>1 && m_vids[1]) ? m_vids[1] : EmptyVid;
    ChangeMouseVid(initialMouseVid,static_cast<unsigned int>(spriteType));
}

// Compiler-generated scalar deleting destructor: calls the 0x004060C0 body,
// then conditionally invokes operator delete when flags&1.
// MapEditZS1.exe destructor owner; persists editor options and ZS1 drawing-VID conversion slots.
MAP_EDIT::~MAP_EDIT()
{
    // ZS1 0x004060D3..0x004060E3: cursor ownership is restored before the
    // initialization-success gate.  The target checks the global Mouse pointer
    // first, so keep this safe even for a partially initialized editor instance.
    if (Mouse)
        Mouse->HardwareOn();

    if (!IsInitSuccess()) return;

    Registry->SetInt(STRING("DrawMap"),(int)optDrawMap);
    Registry->SetInt(STRING("BigStepZ"),(int)optBigStepZ);
    Registry->SetInt(STRING("ControlPanel"),(int)optControlPanel);
    Registry->SetInt(STRING("GridLineX"),optGridLineX);
    Registry->SetInt(STRING("GridLineY"),optGridLineY);
    Registry->SetInt(STRING("DrawName"),(int)optDrawName);

    unsigned int no_hide=0;
    unsigned int hide[MAPEDIT_MAX_VID];
    for (int i=0;i<m_noVid;i++) {
        if (m_vids[i] && m_vids[i]->PropHide()) hide[no_hide++]=(unsigned int)i;
    }
    Registry->SetInt(STRING("NoHideVids"),(int)no_hide);
    Registry->SetData(STRING("HideVids"),hide,no_hide*4u);

    // Target dtor 0x00406432..0x00406513 persists all five ZS1 conversion slots.
    for (int i=0;i<5;++i) {
        const STRING suffix=Printf("%i",i);
        Registry->SetInt(STRING("convertDrawingVid")+suffix,convertDrawingVid[i]);
        Registry->SetString(STRING("nameDrawingVid")+suffix,nameDrawingVid[i]);
    }

    Release();
}
