#include "mapedit/runtime.hpp"

// Direct ZS1 owner: tactical mode is possible iff GROUPS::First() is non-null;
// ordinary region mode (spriteType==0x40) disables Left.
int MAP_EDIT::IsLeftPossible()
{
    if (optTacticMode)
        return m_groups.First()!=0;
    if (spriteType==0x40)
        return 0;
    return 1;
}

// Direct ZS1 owner. In tactical mode this mirrors IsLeftPossible; outside
// tactical mode retail returns 1 unconditionally, including region mode.
int MAP_EDIT::IsRightPossible()
{
    if (optTacticMode)
        return m_groups.First()!=0;
    if (spriteType==0x40)
        return 1;
    return 1;
}

int MAP_EDIT::CallDialogBox(const STRING* name,DLGPROC_OLD f)
{
    Graph->BeginPause();
    const int result=DialogBoxParamA(m_instance,const_cast<STRING*>(name)->CharPtr(),m_hWnd,f,0);
    Graph->EndPause();
    return result;
}
