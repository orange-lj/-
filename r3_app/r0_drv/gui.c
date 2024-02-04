#include "gui.h"
#include"api_defs.h"
#include"util.h"
#include"api.h"
#include"process.h"

static NTSTATUS Gui_Api_Init(PROCESS* proc, ULONG64* parms);
static NTSTATUS Gui_Api_Clipboard(PROCESS* proc, ULONG64* parms);
static void Gui_InitClipboard();
static void Gui_FixClipboard(ULONG integrity);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, Gui_Init)
#endif // ALLOC_PRAGMA



typedef struct _GUI_CLIPBOARD {
    ULONG* items;
    ULONG count;
} GUI_CLIPBOARD;

static ULONG Gui_ClipboardItemLength = 0;
static ULONG Gui_ClipboardIntegrityIndex = 0;




BOOLEAN Gui_Init(void)
{
    Api_SetFunction(API_INIT_GUI, Gui_Api_Init);
    Api_SetFunction(API_GUI_CLIPBOARD, Gui_Api_Clipboard);

    return TRUE;
}


NTSTATUS Gui_Api_Clipboard(PROCESS* proc, ULONG64* parms) 
{
    //确保我们使用的是Windows Vista或更高版本
    if (Driver_OsVersion < DRIVER_WINDOWS_VISTA)
        return STATUS_NOT_SUPPORTED;
    //确保我们是从SbieSvc进程调用的，它可以是
    //主进程（对于core/svc/DriverAssistStart.cpp中的init调用）
    //或GUI代理进程（对于core/svc/GuiServer.cpp中的调用）

    //这样做的原因是，我们在使用Window Station对象时无法锁定剪贴板数据，
    //因此在这里调用我们之前，我们必须依靠调用进程打开(和锁定)剪贴板，并且我们只能依靠我们自己的服务进程
    if (proc || (!MyIsCallerMyServiceProcess()))
        return STATUS_ACCESS_DENIED;
    //根据参数处理call
    if ((ULONG_PTR)parms[1] == -1) {

        if (!Gui_ClipboardItemLength)
            Gui_InitClipboard();

    }
    else {

        if (Gui_ClipboardItemLength && Gui_ClipboardIntegrityIndex)
            Gui_FixClipboard((ULONG)parms[1]);
        else
            return STATUS_UNKNOWN_REVISION;
    }

    return STATUS_SUCCESS;

}

void Gui_InitClipboard(void) 
{
    //ULONG* ptr;
    //ULONG x2, x3, x4, i;

    //GUI_CLIPBOARD* Clipboard = Gui_GetClipboard();
    //if (!Clipboard)
    //    return;
    ////分析剪贴板项目区域的结构。Core/svc/DriverAssistStart.cpp中的InitClipboard在剪贴板上放置了四个唯一的项目。
    //if (Clipboard->count < 4)
    //    return;

    //ptr = Clipboard->items;
    //if (*ptr != 0x111111)
    //    return;

    ////在确保数据区域以第一个剪贴板项格式0x111111开始之后，向前扫描以查看我们在哪里找到0x222222，这是第二个剪贴板项的格式
    ////我们对第三个和第四个条目执行相同的操作，如果所有的长度都匹配，我们就可以假定找到了条目的长度
    //for (x2 = 0; (x2 < 12) && (*ptr != 0x222222); ++x2, ++ptr)
    //    ;
    //if (*ptr != 0x222222)
    //    return;

    //for (x3 = 0; (x3 < 12) && (*ptr != 0x333333); ++x3, ++ptr)
    //    ;
    //if (*ptr != 0x333333)
    //    return;

    //for (x4 = 0; (x4 < 12) && (*ptr != 0x444444); ++x4, ++ptr)
    //    ;
    //if (*ptr != 0x444444)
    //    return;

    //if (x2 != x3 || x3 != x4)
    //    return;
    ////现在，我们需要扫描数据区域，以查看哪个ulong包含完整性级别编号。它应该是0x4000，因为我们是从SbieSvc调用的。我们确保所有四个条目中的值都是0x4000，然后我们就可以假定我们找到了正确的乌龙
    //ptr = Clipboard->items;
    //for (i = 0; (i < 12) && (*ptr != 0x4000); ++i, ++ptr)
    //    ;
    //if (*ptr != 0x4000) 
    //{
    //    //我们有有效的项目长度，但找不到完整性级别，这可能意味着UAC/UIPI已关闭
    //    Gui_ClipboardItemLength = x2;
    //    Gui_ClipboardIntegrityIndex = -1;

    //    return;
    //}
    //ptr += x2;
    //if (*ptr != 0x4000)                     // 0x222222
    //    return;
    //ptr += x2;
    //if (*ptr != 0x4000)                     // 0x333333
    //    return;
    //ptr += x2;
    //if (*ptr != 0x4000)                     // 0x444444
    //    return;
    ////
    //// finish
    ////

    //Gui_ClipboardItemLength = x2;
    //Gui_ClipboardIntegrityIndex = i;
}


void Gui_FixClipboard(ULONG integrity)
{
    //if (Gui_ClipboardIntegrityIndex != -1) {    // do nothing if UIPI is off

    //    ULONG i;
    //    ULONG* ptr;

    //    GUI_CLIPBOARD* Clipboard = Gui_GetClipboard();
    //    if (!Clipboard)
    //        return;

    //    ptr = Clipboard->items;
    //    for (i = 0; i < Clipboard->count; ++i) {
    //        const ULONG il = ptr[Gui_ClipboardIntegrityIndex] & ~1;
    //        if ((il == 0x0000) || (il == 0x1000) || (il == 0x2000)
    //            || (il == 0x3000) || (il == 0x4000)) {
    //            ptr[Gui_ClipboardIntegrityIndex] = integrity;
    //        }
    //        ptr += Gui_ClipboardItemLength;
    //    }
    //}
}

NTSTATUS Gui_Api_Init(PROCESS* proc, ULONG64* parms) 
{
    NTSTATUS status = STATUS_SUCCESS;
    if (NT_SUCCESS(status) && (!Process_ReadyToSandbox)) {

        Process_ReadyToSandbox = TRUE;
    }

    return status;
}