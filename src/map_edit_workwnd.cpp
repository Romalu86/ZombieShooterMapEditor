#include "mapedit/runtime.hpp"

// Original mutable router cursors used by MAP_EDIT::WorkWndMessage.
// Canonical storage in the retail image: 0x004CE358 and 0x004BCBDC.
int g_editorSelectedSpriteCycleIndex=0;
int g_editorMatchingUnitCycleIndex=0;

namespace {
struct TOOLTIP_DISPINFO_OLD {
    HWND__* hwndFrom;       // +0x00
    unsigned int idFrom;    // +0x04
    unsigned int code;      // +0x08
    char* text;             // +0x0C
};

char g_editorTooltipText[256];

// ZS1 editor object clipboard. Retail keeps these owners in .data at
// 0x004A68A0..0x004A6A00; relocation is private to the rebuilt image.
STRING_STREAM g_editorSpriteClipboard;
SPRITE_LIST   g_editorPasteSprites;
int           g_editorSpriteClipboardCount=0;
float         g_editorClipboardAverageX=0.0f;
float         g_editorClipboardAverageY=0.0f;
float         g_editorClipboardAverageZ=0.0f;
float         g_editorClipboardViewX=0.0f;
float         g_editorClipboardViewY=0.0f;
float         g_editorClipboardShiftX=0.0f;
float         g_editorClipboardShiftY=0.0f;
int           g_editorFindStackLayer=0;
int           g_editorFindStackIndex=0;

const unsigned int kToolbarEnableButton=0x401;
const unsigned int kToolbarCheckButton =0x402;


}

// Retail MAP_EDIT::WorkWndMessage/WndProc owner, target 0x00407D40..0x0040B23E.
// The original compiler emits the WM_COMMAND switch as a jump table with
// 118 command ids collapsing to 50 real handlers.  Keep all retail side
// effects in this owner instead of hiding missing logic behind router stubs.
int MAP_EDIT::WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam)
{
    if (msg==0x4Eu) { // WM_NOTIFY
        TOOLTIP_DISPINFO_OLD* tip=reinterpret_cast<TOOLTIP_DISPINFO_OLD*>(lParam);
        if (tip && tip->code==0xFFFFFDF8u) { // TTN_NEEDTEXTA (-520)
            LoadStringA(m_instance,tip->idFrom,g_editorTooltipText,256);
            tip->text=g_editorTooltipText;
        }
        return 0;
    }

    if (msg==0x111u) { // WM_COMMAND
        const unsigned int command=static_cast<unsigned int>(wParam)&0xFFFFu;
        switch (command) {
        case 0xA097: // Copy
        case 0xA09A: { // Cut
            // ZS1 target 0x00408F8B..0x00409172. In tactical mode the
            // clipboard is a serialized set of selected sprites; otherwise
            // retail falls back to the world-coordinate text clipboard.
            if (optTacticMode && selectedSprites.No()>0) {
                m_relation.Release();
                g_editorSpriteClipboardCount=selectedSprites.No();
                g_editorSpriteClipboard.string=STRING("");
                g_editorSpriteClipboard.position=0;
                g_editorClipboardAverageX=0.0f;
                g_editorClipboardAverageY=0.0f;
                g_editorClipboardAverageZ=0.0f;

                for (int i=0;i<selectedSprites.No();++i) {
                    SPRITE* spr=*selectedSprites[i];
                    if (spr)
                        spr->Save(&g_editorSpriteClipboard);
                }
                for (int i=0;i<selectedSprites.No();++i) {
                    SPRITE* spr=*selectedSprites[i];
                    if (!spr)
                        continue;
                    spr->Action(0x50,reinterpret_cast<int>(&g_editorSpriteClipboard),0,0);
                    g_editorClipboardAverageX+=spr->X();
                    g_editorClipboardAverageY+=spr->Y();
                    g_editorClipboardAverageZ+=spr->Z();
                    if ((spr->Vid()->m_unknown0C & 0x10u)!=0) {
                        g_editorClipboardViewX=Graph->ViewXMin();
                        g_editorClipboardViewY=Graph->ViewYMin();
                        g_editorClipboardShiftX=m_shiftX;
                        g_editorClipboardShiftY=m_shiftY;
                    }
                }
                const float inv=1.0f/static_cast<float>(selectedSprites.No());
                g_editorClipboardAverageX*=inv;
                g_editorClipboardAverageY*=inv;
                g_editorClipboardAverageZ*=inv;

                if (command==0xA09A) {
                    undo.Begin();
                    for (int i=0;i<selectedSprites.No();++i)
                        undo.AddRemove(*selectedSprites[i]);
                    undo.End();
                    selectedSprites.Release();
                }
                break;
            }
            Printf("%i,%i,%i",(int)Mouse->X(),(int)Mouse->Y(),(int)Mouse->Z()).WriteToClipboard(m_hWnd);
            break;
        }
        case 0xA095: { // Paste
            // ZS1 target 0x004091D4..0x00409433. Recreate version-13
            // sprite records, decode their Action(0x51) relations, move the
            // whole selection under the current software cursor, and select it.
            if (optTacticMode && g_editorSpriteClipboardCount>0) {
                g_editorPasteSprites.Release();
                g_editorSpriteClipboard.position=0;
                for (int i=0;i<g_editorSpriteClipboardCount;++i) {
                    SPRITE* spr=LoadSprite(&g_editorSpriteClipboard,13);
                    if (spr && spr!=reinterpret_cast<SPRITE*>(-1))
                        g_editorPasteSprites.Insert(spr);
                }

                if (g_editorSpriteClipboardCount>1 ||
                    (g_editorPasteSprites.No()>0 && ((*g_editorPasteSprites[0])->Vid()->m_unknown0C&0x10u)!=0))
                    g_editorClipboardAverageZ=Mouse->Z();

                selectedSprites.Release();
                for (int i=0;i<g_editorPasteSprites.No();++i) {
                    SPRITE* spr=*g_editorPasteSprites[i];
                    if (!spr)
                        continue;
                    spr->Action(0x51,reinterpret_cast<int>(&g_editorSpriteClipboard),13,999);
                    spr->ChangeCoor(Mouse->X()-g_editorClipboardAverageX+spr->X(),
                                    Mouse->Y()-g_editorClipboardAverageY+spr->Y(),
                                    Mouse->Z()-g_editorClipboardAverageZ+spr->Z());

                    if ((spr->Vid()->m_unknown0C&0x10u)!=0) {
                        // Exact ZS1 isometric correction from 0x40935F..0x4093C2.
                        const float correctedX=spr->X()-g_editorClipboardShiftX
                            -g_editorClipboardViewX*0.5f+Graph->ViewXMin()*0.5f;
                        const float correctedY=spr->Y()-g_editorClipboardShiftY
                            -g_editorClipboardViewY*0.5f+Graph->ViewYMin()*0.5f+spr->Z();
                        spr->ChangeCoor(correctedX,correctedY,spr->Z());
                    }
                    selectedSprites.Insert(spr);
                }
                g_editorPasteSprites.Release();
                break;
            }

            STRING str;
            float x=0.0f,y=0.0f;
            str.ReadFromClipboard(m_hWnd);
            sscanf(str.CharPtr(),"%f,%f",&x,&y);
            SetShiftCoor(x,y,2);
            break;
        }
        case 0xA0A1: { // cycle selected sprite
            if (optTacticMode && selectedSprites.No()>0) {
                ++g_editorSelectedSpriteCycleIndex;
                if (g_editorSelectedSpriteCycleIndex>=selectedSprites.No())
                    g_editorSelectedSpriteCycleIndex=0;
                SPRITE* spr=*selectedSprites[g_editorSelectedSpriteCycleIndex];
                SetShiftCoor(spr->X(),spr->Y(),2);
            }
            break;
        }
        case 0xA0A2: { // hide selected VID / mouse VID
            if (optTacticMode && selectedSprites.No()>0) {
                for (int i=0;i<selectedSprites.No();++i)
                    (*selectedSprites[i])->Vid()->SetPropHide(1);
            } else if (Mouse && Mouse->Vid()) {
                VID* vid=Mouse->Vid();
                vid->SetPropHide(!vid->PropHide());
            }
            break;
        }
        case 0xA098: { // New map -> retail logo.map defaults + Map Property
            optionBits|=0x800u;
            editFileName=STRING("maps\\logo.map");
            STRING dialogName("MAP_PROPERTY");
            CallDialogBox(&dialogName,AppMapProperty);
            break;
        }
        case 0xA09B: { // Find unused vid
            STRING dialogName("UNUSED_VID");
            CallDialogBox(&dialogName,AppUnusedVid);
            break;
        }
        case 0xA0A6: // Align X
        case 0xA0A7: // Align Y
        case 0xA0A8: { // Align Z
            if (optTacticMode && selectedSprites.No()>1) {
                SPRITE* first=*selectedSprites[0];
                for (int i=1;i<selectedSprites.No();++i) {
                    SPRITE* spr=*selectedSprites[i];
                    if (!spr)
                        continue;
                    if (command==0xA0A6)
                        spr->ChangeCoor(first->X(),spr->Y(),spr->Z());
                    else if (command==0xA0A7)
                        spr->ChangeCoor(spr->X(),first->Y(),spr->Z());
                    else
                        spr->ChangeCoor(spr->X(),spr->Y(),first->Z());
                }
            }
            break;
        }
        case 0xA0AB: { // Find at stack...
            ACT* pattern=EditorStackSearchAction();
            if (g_editorFindStackIndex<0) {
                pattern->var1=-999999;
                pattern->var2=-999999;
                pattern->var3=-999999;
            }
            if (!DialogBoxParamA(m_instance,"EDIT_STACK_LINE",m_hWnd,AppEditStackLine,0))
                break;

            if (g_editorFindStackLayer>=MAPEDIT_MAP_LAYER_COUNT)
                g_editorFindStackLayer=0;

            SPRITE* found=0;
            int layer=g_editorFindStackLayer;
            int index=g_editorFindStackIndex;
            while (layer<MAPEDIT_MAP_LAYER_COUNT && !found) {
                --index;
                while (index>=0) {
                    SPRITE** slot=m_layers[layer][index];
                    SPRITE* spr=slot ? *slot : 0;
                    if (!spr) {
                        --index;
                        continue;
                    }
                    if (spr->HaveAction(pattern)) {
                        found=spr;
                        break;
                    }
                    --index;
                }
                if (found)
                    break;
                ++layer;
                if (layer<MAPEDIT_MAP_LAYER_COUNT)
                    index=m_layers[layer].No();
            }
            if (found) {
                g_editorFindStackLayer=layer;
                g_editorFindStackIndex=index;
                if (!optTacticMode)
                    SendMessageA(m_hWnd,0x111u,0x9C60u,0);
                selectedSprites.Release();
                selectedSprites.Insert(found);
                SetShiftCoor(found->X(),found->Y(),2);
            } else {
                g_editorFindStackLayer=MAPEDIT_MAP_LAYER_COUNT;
                g_editorFindStackIndex=0;
            }
            break;
        }
        case 0xA09E: { // select all sprites of current editor class
            if (optTacticMode) {
                POLYGON polygon;
                polygon.CreateBox(0.0f,0.0f,m_w,m_h);
                FindSpritesInsidePolygon(spriteType<<20,&polygon,&selectedSprites);
            }
            break;
        }
        case 0xA09F: { // select current-class sprites inside view
            if (optTacticMode) {
                POLYGON polygon;
                polygon.CreateBox(Graph->ViewXMin()+m_shiftX,Graph->ViewYMin()+m_shiftY,
                                  Graph->ViewXMax()+m_shiftX,Graph->ViewYMax()+m_shiftY);
                FindSpritesInsidePolygon(spriteType<<20,&polygon,&selectedSprites);
            }
            break;
        }
        case 0xA0A0: { // select all sprites having selected VID
            if (optTacticMode && selectedSprites.No()>0) {
                POLYGON polygon;
                polygon.CreateBox(0.0f,0.0f,m_w,m_h);
                const int type=MAP::EncodeVidQuery((*selectedSprites[0])->Vid()->m_idx);
                FindSpritesInsidePolygon(type,&polygon,&selectedSprites);
            }
            break;
        }
        case 0xA09D: { // select selected-VID sprites inside view
            if (optTacticMode && selectedSprites.No()>0) {
                POLYGON polygon;
                polygon.CreateBox(Graph->ViewXMin()+m_shiftX,Graph->ViewYMin()+m_shiftY,
                                  Graph->ViewXMax()+m_shiftX,Graph->ViewYMax()+m_shiftY);
                const int type=MAP::EncodeVidQuery((*selectedSprites[0])->Vid()->m_idx);
                FindSpritesInsidePolygon(type,&polygon,&selectedSprites);
            }
            break;
        }
        case 0x9C64:
            { STRING dialogName("CONVERTSPRITE"); CallDialogBox(&dialogName,AppConvertSprite); }
            break;
        case 0xB02A:
            { STRING dialogName("OPTIONS"); CallDialogBox(&dialogName,AppOptions); }
            break;
        case 0xB01F:
            { STRING dialogName("MAP_PROPERTY"); CallDialogBox(&dialogName,AppMapProperty); }
            break;
        case 0x9C93:
            { STRING dialogName("SELECT_VID"); CallDialogBox(&dialogName,AppSelectVid); }
            break;
        case 0x9C89:
            if (selectedSprites.No()>0 && (*selectedSprites[0])->IsSpriteClass(0x13u))
                { STRING dialogName("TEXT_PROPERTY"); CallDialogBox(&dialogName,AppTextProperty); }
            else if (spriteType&0x40)
                { STRING dialogName("REGION_PROPERTY"); CallDialogBox(&dialogName,AppRegionProperty); }
            else
                { STRING dialogName("UNIT_PROPERTY"); CallDialogBox(&dialogName,AppUnitProperty); }
            break;
        case 0x9C8E:
            undo.Undo();
            break;
        case 0xB024:
            undo.Redo();
            break;
        case 0x9C42:
            // ZS1 target 0x00407E7D..0x00407F64.  The bit-11 editor
            // state routes ordinary Save directly through the Save-As owner.
            // Otherwise retail resets undo and dispatches .men through MENU::Save;
            // all other names go through the virtual MAP::Save route.
            if (!(optionBits&0x800u)) {
                undo.Reset();
                if (strstr(editFileName.CharPtr(),".men"))
                    m_menu.Save(&editFileName);
                else
                    Save(editFileName);
                break;
            }
            // fall through to retail Save-As routing
        case 0x9C8B: { // save/export
            // Retail .data block at 0x004BC088: OPENFILENAMEA requires one
            // double-NUL terminated label/pattern chain, not a plain label.
            static char kSaveFilter[] =
                "Map file\0*.map\0"
                "Menu file\0*.men\0"
                "AllMap(tga) file\0*.tga\0"
                "GridZ(tga) file\0*.bmp\0\0";
            STRING newName=SaveDialog(kSaveFilter);
            if (newName=="")
                break;
            STRING extPart=newName.AfterLast(".");
            STRING ext=extPart.ToLower();
            if (ext=="tga") {
                PICTURE pict((int)SizeX(),(int)SizeY(),PICTURE::TYPE_TGA);
                m_input.ChangeCoor(600.0f,600.0f);
                for (int y=0;y<(int)SizeY();y+=256) {
                    for (int x=0;x<(int)SizeX();x+=256) {
                        m_shiftX=(float)x-Graph->ViewXMin();
                        m_shiftY=(float)y-Graph->ViewYMin();
                        Graph->ClearScreen(COLOR(0,0,0));
                        Graph->PreTact();
                        Graph->Tact(1);
                        Graph->SavePict(&pict,x,y,(int)Graph->ViewXMin(),(int)Graph->ViewYMin(),256,256);
                        Graph->PostTact(1);
                    }
                }
                pict.SaveTGA(&newName,0,0,-1,-1);
                pict.Close();
            } else if (ext=="bmp") {
                PICTURE pict((int)SizeX()/8,(int)SizeY()/8,PICTURE::TYPE_TGA);
                for (int y=0;y<(int)SizeY();y+=8) {
                    for (int x=0;x<(int)SizeX();x+=8) {
                        const int z=(int)GetGroundZ((float)x,(float)y);
                        pict.PutPixel(x/8,y/8,COLOR(0,z/256,z));
                    }
                }
                newName=newName.ToLower();
                newName.Replace(".bmp",".tga");
                pict.SaveTGA(&newName,0,0,-1,-1);
                pict.Close();
            } else if (ext=="men") {
                // Retail Save-As treats MEN as a real editor document: both
                // filenames become the chosen path and undo history is reset
                // before MENU::Save.
                m_mapName=newName;
                editFileName=m_mapName;
                undo.Reset();
                m_menu.Save(&newName);
                optionBits&=~0x800u;
            } else {
                m_mapName=newName;
                editFileName=m_mapName;
                undo.Reset();
                Save(m_mapName);
                optionBits&=~0x800u;
            }
            DrawMapName();
            break;
        }
        case 0x9C41:
        case 0xB01C: { // open/import
            // Retail .data block at 0x004BC0F8.
            static char kOpenFilter[] =
                "Map files\0*.map\0"
                "Menu files\0*.men\0"
                "Terrain files\0*.vid\0"
                "GridZ files\0*.tga;*.z\0"
                "All Files\0*.*\0\0";
            STRING name=OpenDialog(kOpenFilter);
            if (name=="")
                break;
            STRING extPart=name.AfterLast(".");
            STRING ext=extPart.ToLower();
            if (ext=="tga" || ext=="z") {
                PICTURE pict;
                if (pict.Load(&name)!=0) {
                    ::Error->Window("grid error:can't load %s",name.CharPtr());
                    pict.Close();
                    break;
                }
                ResetGroundZ();
                const int mapSizeX=(int)SizeX();
                const int mapSizeY=(int)SizeY();
                for (int y=0;y<mapSizeY;y+=8) {
                    for (int x=0;x<mapSizeX;x+=8) {
                        const float xz=(float)pict.SizeX()*(float)x/(float)mapSizeX;
                        const float yz=(float)pict.SizeY()*(float)y/(float)mapSizeY;
                        if (pict.IsZ()) {
                            SetGroundZ((float)x,(float)y,(float)((int)pict.GetData((int)xz,(int)yz))/8.0f);
                        } else {
                            COLOR color=pict.GetPixel((int)xz,(int)yz);
                            SetGroundZ((float)x,(float)y,(float)color.Green()*256.0f+(float)color.Blue());
                        }
                    }
                }
                pict.Close();
            } else if (ext=="vid") {
                LoadTerrain(name);
            } else if (ext=="men") {
                // ZS1 retail WorkWndMessage 0x00409CC7..0x00409D08:
                // leave map-editor draw mode, release the current map state, remember
                // the opened menu filename, then load MENU and refresh the title.
                optionBits &= ~0x800u;
                Release();
                editFileName=name;
                m_menu.Load(&name);
                DrawMapName();
            } else {
                // ZS1 target 0x00409D0D..0x00409DD2.  Opening a MAP clears
                // editor bit 11, re-enables Save/Save-As in both toolbar and
                // menu, then enters the virtual MAP_EDIT::Load owner.
                optionBits&=~0x800u;
                SendMessageA(hToolBar,kToolbarEnableButton,0x9C42u,1);
                EnableMenuItem(GetMenu(m_hWnd),0x9C42u,0u);
                EnableMenuItem(GetMenu(m_hWnd),0x9C8Bu,0u);
                Load(name);
            }
            break;
        }
        case 0x9C4E:
            undo.Begin();
            FillBox(0.0f,0.0f,SizeX(),SizeY());
            undo.End();
            break;
        case 0xB027: { // remove exact duplicate sprites
            undo.Begin();
            for (int layer=0;layer<MAPEDIT_MAP_LAYER_COUNT;++layer) {
                int i=0;
                SPRITE* spr=FirstSprite(layer,&i);
                while (spr) {
                    int j=i;
                    SPRITE* spr2=NextSprite(layer,&j);
                    while (spr2) {
                        if (spr->Vid()==spr2->Vid() &&
                            spr->Direction().value==spr2->Direction().value &&
                            spr->X()==spr2->X() && spr->Y()==spr2->Y() && spr->Z()==spr2->Z()) {
                            undo.AddRemove(spr);
                            spr=spr2;
                        }
                        spr2=NextSprite(layer,&j);
                    }
                    spr=NextSprite(layer,&i);
                }
            }
            undo.End();
            break;
        }
        case 0x9C86: { // remove all instances of current mouse VID
            undo.Begin();
            int i=0;
            SPRITE* spr=FirstSprite(Mouse->Vid()->m_layer,&i);
            while (spr) {
                if (spr->Vid()==Mouse->Vid())
                    undo.AddRemove(spr);
                spr=NextSprite(Mouse->Vid()->m_layer,&i);
            }
            undo.End();
            break;
        }
        case 0xA08F:
            ResetGroundZ();
            break;
        case 0x9C85:
            m_groups.DeleteAll();
            break;
        case 0xB01E:
            DeleteExtraVid();
            break;
        case 0x9C8D:
            m_menu.DeleteAll();
            break;
        case 0xB01D:
            optGround0=!optGround0;
            break;
        case 0x9C59:
            optShiftSnapX=0;
            optShiftSnapY=0;
            optSnap=!optSnap;
            SendMessageA(hToolBar,kToolbarCheckButton,0x9C59,(long)optSnap);
            break;
        case 0x9C62:
            optChessSnap=!optChessSnap;
            break;
        case 0xB029:
            optAirBrush=!optAirBrush;
            break;
        case 0xB02B:
            optRandomDir=!optRandomDir;
            break;
        case 0x9C75:
            optDelete=!optDelete;
            break;
        case 0x9C7E:
            ChangeMouseVid(Vid(Right()),(unsigned int)spriteType);
            break;
        case 0x9C7D:
            ChangeMouseVid(Vid(Left()),(unsigned int)spriteType);
            break;
        case 0xA093:
            SetControlPanel(!optControlPanel);
            break;
        case 0xA091:
            SetShiftCoor(SizeX()/2.0f,SizeY()/2.0f,0);
            break;
        case 0xA092:
            m_input.ChangeCoor(Graph->SizeX()/2.0f,Graph->SizeY()/2.0f);
            break;
        case 0x9C6F:
            ChangeMouseVid(Mouse->Vid(),1u);
            break;
        case 0x9C70:
            ChangeMouseVid(Mouse->Vid(),2u);
            break;
        case 0x9C71:
            ChangeMouseVid(Mouse->Vid(),4u);
            break;
        case 0x9C72:
            ChangeMouseVid(Mouse->Vid(),8u);
            break;
        case 0x9C73:
            ChangeMouseVid(Mouse->Vid(),16u);
            break;
        case 0xB022:
            ChangeMouseVid(Vid(Right()),0x40u);
            break;
        case 0xB023:
            ChangeMouseVid(Mouse->Vid(),0x20u);
            break;
        case 0x9C60: { // ZS1 tactical mode, target 0x004080BB..0x004082E2
            optTacticMode=!optTacticMode;
            const int normalEnable=!optTacticMode;
            SendMessageA(hToolBar,kToolbarCheckButton,0x9C60,optTacticMode);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C59,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C62,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C75,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0xB01D,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0xB029,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0xB02B,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C89,optTacticMode);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C83,optTacticMode);

            if (optTacticMode) {
                // Retail saves the ordinary editor cursor state, forces Ground0
                // on and zeroes insertZ.  It then switches to NVID 1 (or EmptyVid
                // if it is unavailable) through ChangeMouseVid; HardwareOn/Off is
                // not part of the ZS1 owner.
                savedVid=Mouse->Vid();
                savedGround0=optGround0 ? 1 : 0;
                savedInsertZ=insertZ;
                optGround0=1;
                insertZ=0.0f;

                VID* tacticVid=(m_noVid>1 && VidSlot(1)) ? VidSlot(1) : EmptyVid;
                ChangeMouseVid(tacticVid,savedVid->m_unknown0C);
            } else {
                // Exact ZS1 exit semantics: restore Ground0/insertZ.  With one
                // selected object, make that object's VID and direction current;
                // otherwise restore the cursor VID saved on tactical entry.
                optGround0=savedGround0 ? 1u : 0u;
                insertZ=savedInsertZ;

                if (selectedSprites.No()==1) {
                    SPRITE* selected=*selectedSprites[0];
                    VID* selectedVid=selected->Vid();
                    ChangeMouseVid(selectedVid,selectedVid->m_unknown0C);
                    Mouse->ChangeDirection(selected->Direction());
                } else {
                    ChangeMouseVid(savedVid,savedVid->m_unknown0C);
                }
            }
            break;
        }
        case 0x9C83: { // create/delete current selection group
            GROUP* first=m_groups.First();
            if (selectedSprites.IsEqual(first)) {
                delete first;
                break;
            }
            if (selectedSprites.No()==0)
                break;
            SPRITE* spr=*selectedSprites[0];
            m_groups.DeletePointerToSprite(spr);
            GROUP* group=m_groups.CreateNewGroup(spr);
            for (int i=1;i<selectedSprites.No();++i) {
                spr=*selectedSprites[i];
                m_groups.DeletePointerToSprite(spr);
                group->Insert(spr);
            }
            break;
        }
        case 0x9C92: { // find next unit using current mouse VID
            SPRITE* spr=Hash->NextUnit(&g_editorMatchingUnitCycleIndex);
            if (!spr)
                spr=Hash->FirstUnit(&g_editorMatchingUnitCycleIndex);
            while (spr && spr->Vid()!=Mouse->Vid()) {
                spr=Hash->NextUnit(&g_editorMatchingUnitCycleIndex);
                if (!spr)
                    spr=Hash->FirstUnit(&g_editorMatchingUnitCycleIndex);
            }
            if (spr)
                SetShiftCoor(spr->X(),spr->Y(),2);
            break;
        }
        case 0xB026: { // rebuild ground height from rendered Z buffer
            const int noX=(int)SizeX()/8;
            const int noY=(int)SizeY()/8;
            const unsigned int cells=(unsigned int)(noX*noY);
            int* data=static_cast<int*>(::operator new(cells*sizeof(int)));
            if (!data) {
                ::Error->Window("Enough memory");
                break;
            }
            int* count=static_cast<int*>(::operator new(cells*sizeof(int)));
            if (!count) {
                // Retail owner returns here without freeing data.
                ::Error->Window("Enough memory");
                break;
            }
            memset(data,0,cells*sizeof(int));
            memset(count,0,cells*sizeof(int));
            m_input.ChangeCoor(600.0f,600.0f);
            for (int tileY=0;tileY<(int)SizeY();tileY+=256) {
                for (int tileX=0;tileX<(int)SizeX();tileX+=256) {
                    m_shiftX=(float)tileX-Graph->ViewXMin();
                    m_shiftY=(float)tileY-Graph->ViewYMin();
                    Graph->ClearScreen(COLOR(0,0,0));
                    Graph->PreTact();
                    Graph->Tact(1);
                    int pitch=0;
                    unsigned short* zbuffer=Graph->LockZ(&pitch);
                    for (int localY=0;localY<256;++localY) {
                        for (int localX=0;localX<256;++localX) {
                            const int mapX=tileX+localX;
                            const int mapY=tileY+localY;
                            if (!ValidateXY((float)mapX,(float)mapY))
                                continue;
                            const int screenX=localX+(int)Graph->ViewXMin();
                            const int screenY=localY+(int)Graph->ViewYMin();
                            const int z=(int)zbuffer[screenX+screenY*pitch]/8-128;
                            const int yy=mapY+z;
                            if (!ValidateXY((float)mapX,(float)yy))
                                continue;
                            const int cellX=mapX/8;
                            const int cellY=yy/8;
                            const int cell=cellY*noX+cellX;
                            if (z>data[cell])
                                data[cell]=z;
                            count[cell]=1;
                        }
                    }
                    Graph->UnLockZ();
                    Graph->PostTact(1);
                }
            }
            for (int y=0;y<noY;++y) {
                for (int x=0;x<noX;++x) {
                    const int cell=y*noX+x;
                    if (count[cell]) {
                        data[cell]/=count[cell];
                        if (data[cell]>m_groundz[cell])
                            m_groundz[cell]=(short)data[cell];
                    }
                }
            }
            ::operator delete(data);
            ::operator delete(count);
            break;
        }
        case 0xB028: { // bake visible ground into hardware_ground.vid
            PICTURE_MAKEVID pict((int)SizeX(),(int)SizeY(),5u);
            GAMMA gamma=Graph->GetGamma();
            GAMMA neutral;
            Graph->SetGamma(&neutral);
            m_input.ChangeCoor(600.0f,600.0f);
            for (int y=0;y<(int)SizeY();y+=256) {
                for (int x=0;x<(int)SizeX();x+=256) {
                    m_shiftX=(float)x-Graph->ViewXMin();
                    m_shiftY=(float)y-Graph->ViewYMin();
                    Graph->ClearScreen(COLOR(0,0,0));
                    Graph->PreTact();
                    Graph->Tact(1);
                    Graph->SavePictAndZ(&pict,x,y,(int)Graph->ViewXMin(),(int)Graph->ViewYMin(),256,256);
                    Graph->PostTact(1);
                }
            }
            Graph->SetGamma(&gamma);
            const int err=pict.MakeVid(1u,STRING("hardware_ground.vid"));
            pict.Close();
            if (err)
                break;

            int i=0;
            for (;i<m_noVid;++i) {
                VID* vid=VidSlot(i);
                if (vid && vid->IsExtraType() && i==0x400) {
                    STRING* terrainName=&vid->m_resourceName;
                    FRemove(terrainName);
                    STRING baked("hardware_ground.vid");
                    FRename(&baked,terrainName);
                    break;
                }
            }
            if (i>=m_noVid) {
                STRING filename=editFileName;
                STRING dir=FCurrentDirectory();
                filename.Replace(".map",".vid");
                STRING empty("");
                filename.Replace(&dir,&empty);
                filename.RemoveBeginChars("\\");
                FRemove(&filename);
                STRING baked("hardware_ground.vid");
                FRename(&baked,&filename);
                LoadTerrain(filename);
            }

            // ZS1 retail 0x0040AF5A walks all 21 layer lists backwards.
            // Deleting while walking forward shifts LIST entries and does not
            // preserve the retail destructive rebuild semantics.
            for (int layer=MAPEDIT_MAP_LAYER_COUNT-1;layer>=0;--layer) {
                for (int index=m_layers[layer].No()-1;index>=0;--index) {
                    SPRITE** slot=m_layers[layer][index];
                    SPRITE* spr=slot ? *slot : 0;
                    if (spr && !spr->Vid()->PropHide() && spr->Vid()->m_idx!=0x400)
                        spr->ScalarDeletingDestructor(1u);
                }
            }
            undo.Reset();
            Save(editFileName);
            Load(editFileName);
            break;
        }
        case 0x9C44:
            SendMessageA(hwnd,0x10u,0u,0); // WM_CLOSE
            break;
        default:
            // Retail jump table maps the remaining command IDs to its common no-op return.
            break;
        }
        return 0;
    }

    if (msg==0x84u) { // WM_NCHITTEST: show editor control panel only over its viewport strip
        RECT_OLD rect;
        GetWindowRect(m_hWnd,&rect);
        const float screenX=(float)((unsigned int)lParam&0xFFFFu)-(float)rect.left;
        const float screenY=(float)(((unsigned int)lParam>>16)&0xFFFFu)-(float)rect.top;
        if (screenX>Graph->ViewXMax() && screenY>Graph->ViewYMin()) {
            m_input.screenMouseX=screenX;
            m_input.screenMouseY=screenY;
            EnableWindow(hControlPanel,1);
        } else if (EnableWindow(hControlPanel,0)==0) {
            SetFocus(m_hWnd);
        }
    }

    return MAP::WorkWndMessage(hwnd,msg,wParam,lParam);
}
