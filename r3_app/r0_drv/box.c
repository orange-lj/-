#include "box.h"
#include"../common/defines.h"
BOOLEAN Box_IsValidName(const WCHAR* name)
{
    int i;

    for (i = 0; i < (BOXNAME_COUNT - 2); ++i) {
        if (!name[i])
            break;
        if (name[i] >= L'0' && name[i] <= L'9')
            continue;
        if (name[i] >= L'A' && name[i] <= L'Z')
            continue;
        if (name[i] >= L'a' && name[i] <= L'z')
            continue;
        if (name[i] == L'_')
            continue;
        return FALSE;
    }
    if (i == 0 || name[i])
        return FALSE;
    return TRUE;
}
