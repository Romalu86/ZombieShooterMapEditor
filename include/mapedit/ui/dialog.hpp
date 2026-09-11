#pragma once
// DIALOG owner family. Included in ABI order by mapedit/runtime.hpp.
//
// ZS1 retail keeps the small window/control wrappers in the historical header.
// The native lane deliberately uses /Ob1: this makes the same tiny owners fold
// into dialog/GRAPH callers without forceinline/noinline or synthetic padding.
class DIALOG_ITEM {
public:
    HWND__* hWnd; int id;
    DIALOG_ITEM(HWND__* wnd,int rc_id) : hWnd(wnd),id(rc_id) {}

    // to inline the header body).
    int Message(unsigned int msg,unsigned int w,long l)
    {
        return static_cast<int>(SendDlgItemMessageA(hWnd,id,msg,w,l));
    }
    int IsNotify(unsigned int msg,unsigned int wParam,unsigned short notifyMessage)
    {
        if (msg!=0x111u) return 0;
        if ((wParam&0xFFFFu)!=static_cast<unsigned int>(id)) return 0;
        return ((wParam>>16)&0xFFFFu)==static_cast<unsigned int>(notifyMessage);
    }
    void SetText(const STRING* str)
    {
        SetDlgItemTextA(hWnd,id,const_cast<STRING*>(str)->CharPtr());
    }
    STRING GetText();
    void SetUnsigned(unsigned int val) { SetDlgItemInt(hWnd,id,val,0); }
    unsigned int GetUnsigned() { return GetDlgItemInt(hWnd,id,0,0); }
    void SetInt(int val) { SetDlgItemInt(hWnd,id,static_cast<unsigned int>(val),1); }
    int GetInt() { return static_cast<int>(GetDlgItemInt(hWnd,id,0,1)); }
    void Enable(int flag) { EnableWindow(GetDlgItem(hWnd,id),flag); }
    void Disable() { EnableWindow(GetDlgItem(hWnd,id),0); }
};
class DIALOG_COMBO_BOX : public DIALOG_ITEM {
public:
    DIALOG_COMBO_BOX(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    void Reset() { Message(0x14B,0,0); }
    int NoString() { return Message(0x146,0,0); }
    int AddString(const STRING* str)
    {
        return Message(0x143,0,reinterpret_cast<long>(const_cast<STRING*>(str)->CharPtr()));
    }
    int AddStringWithData(const STRING* str,int data)
    {
        const int index=AddString(str);
        if (index!=-1) Message(0x151,static_cast<unsigned int>(index),data);
        return index;
    }
    void SetCurrent(int index) { Message(0x14E,static_cast<unsigned int>(index),0); }
    void SetCurrent(const STRING* str)
    {
        Message(0x14D,0,reinterpret_cast<long>(const_cast<STRING*>(str)->CharPtr()));
    }
    int GetCurrentStringIndex() { return Message(0x147,0,0); }
    int GetData(int index) { return Message(0x150,static_cast<unsigned int>(index),0); }
    int GetCurrentStringData() { return GetData(GetCurrentStringIndex()); }
};
class DIALOG_BUTTON : public DIALOG_ITEM {
public:
    DIALOG_BUTTON(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    unsigned int GetCheck() { return Message(0xF0,0,0)==1; }
    void SetCheck(int check) { Message(0xF1,check!=0,0); }
    int IsClicked(unsigned int msg,unsigned int wParam) { return IsNotify(msg,wParam,0); }
};
class DIALOG_LIST_BOX : public DIALOG_ITEM {
public:
    DIALOG_LIST_BOX(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    int NoString() { return Message(0x18B,0,0); }
    void Reset() { Message(0x184,0,0); }
    void SetColumnWidth(int width) { Message(0x195,static_cast<unsigned int>(width),0); }
    int AddString(const STRING* str)
    {
        return Message(0x180,0,reinterpret_cast<long>(const_cast<STRING*>(str)->CharPtr()));
    }
    int AddStringWithData(const STRING* str,int data)
    {
        const int index=AddString(str);
        if (index!=-1) Message(0x19A,static_cast<unsigned int>(index),data);
        return index;
    }
    void SetCurrent(int index) { Message(0x186,static_cast<unsigned int>(index),0); }
    int GetCurrentStringIndex() { return Message(0x188,0,0); }
    int GetData(int index) { return Message(0x199,static_cast<unsigned int>(index),0); }
    int GetCurrentStringData() { return GetData(GetCurrentStringIndex()); }
    int GetStringDataIndex(int data);
    int IsSelChange(unsigned int msg,unsigned int wParam) { return IsNotify(msg,wParam,1); }
    int IsDoubleClicked(unsigned int msg,unsigned int wParam) { return IsNotify(msg,wParam,2); }
    void SetSelected(int index) { Message(0x185u,1u,index); }
    int IsSelected(int index) { return Message(0x187u,static_cast<unsigned int>(index),0)>0; }
    void SetUnSelected(int index) { Message(0x185u,0u,index); }
};
class DIALOG_TABS : public DIALOG_ITEM {
public:
    DIALOG_TABS(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    int NoItem() { return Message(0x1304,0,0); }
    int SetMinWidth(int width) { return Message(0x1331,0,width); }
    int AddItemWithData(const STRING* str,int data);
    void SetCurrentWithData(int data);
    void SetCurrent(int index) { Message(0x130C,static_cast<unsigned int>(index),0); }
    int GetData(int index);
    int GetCurrentItemData() { return GetData(GetCurrentItemIndex()); }
    int GetCurrentItemIndex() { return Message(0x130B,0,0); }
    int IsSelChange(unsigned int msg,unsigned int wParam,long lParam);
};
class DIALOG_TEXT : public DIALOG_ITEM {
public:
    DIALOG_TEXT(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    const STRING* operator=(const STRING* str) { SetText(str); return str; }
    operator STRING() { return GetText(); }
};
class DIALOG_UNSIGNED : public DIALOG_ITEM {
public:
    DIALOG_UNSIGNED(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    unsigned int operator=(unsigned int pos) { SetUnsigned(pos); return pos; }
    operator unsigned int() { return GetUnsigned(); }
};
class DIALOG_INT : public DIALOG_ITEM {
public:
    DIALOG_INT(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    int operator=(int pos) { SetInt(pos); return pos; }
    operator int() { return GetInt(); }
};
class DIALOG_SLIDER : public DIALOG_ITEM {
public:
    DIALOG_SLIDER(HWND__* wnd,int rc_id) : DIALOG_ITEM(wnd,rc_id) {}
    void SetRange(int left,int right)
    {
        Message(0x406u,1u,static_cast<long>((left&0xFFFF)|((right&0xFFFF)<<16)));
    }
    int operator=(int pos) { Message(0x405u,1u,pos); return pos; }
    operator int() { return Message(0x400u,0u,0); }
};
class DIALOG_RADIO : public DIALOG_ITEM {
    int lastId;
public:
    DIALOG_RADIO(HWND__* wnd,int firstId,int lastResourceId)
        : DIALOG_ITEM(wnd,firstId),lastId(lastResourceId) {}
    void Check(int index) { CheckRadioButton(hWnd,id,lastId,id+index); }
    void SetItem(int index,const STRING* str)
    {
        SetDlgItemTextA(hWnd,id+index,const_cast<STRING*>(str)->CharPtr());
    }
    int GetIndexClicked(unsigned int msg,unsigned int wParam)
    {
        if (msg!=0x111u || ((wParam>>16)&0xFFFFu)!=0u) return -1;
        const int commandId=static_cast<int>(wParam&0xFFFFu);
        if (commandId<id || commandId>lastId) return -1;
        return commandId-id;
    }
    int GetIndexDblClicked(unsigned int msg,unsigned int wParam)
    {
        if (msg!=0x111u || ((wParam>>16)&0xFFFFu)!=5u) return -1;
        const int commandId=static_cast<int>(wParam&0xFFFFu);
        if (commandId<id || commandId>lastId) return -1;
        return commandId-id;
    }
};
