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
    //ȷ������ʹ�õ���Windows Vista����߰汾
    if (Driver_OsVersion < DRIVER_WINDOWS_VISTA)
        return STATUS_NOT_SUPPORTED;
    //ȷ�������Ǵ�SbieSvc���̵��õģ���������
    //�����̣�����core/svc/DriverAssistStart.cpp�е�init���ã�
    //��GUI������̣�����core/svc/GuiServer.cpp�еĵ��ã�

    //��������ԭ���ǣ�������ʹ��Window Station����ʱ�޷��������������ݣ�
    //����������������֮ǰ�����Ǳ����������ý��̴�(������)�����壬��������ֻ�����������Լ��ķ������
    if (proc || (!MyIsCallerMyServiceProcess()))
        return STATUS_ACCESS_DENIED;
    //���ݲ�������call
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
    ////������������Ŀ����Ľṹ��Core/svc/DriverAssistStart.cpp�е�InitClipboard�ڼ������Ϸ������ĸ�Ψһ����Ŀ��
    //if (Clipboard->count < 4)
    //    return;

    //ptr = Clipboard->items;
    //if (*ptr != 0x111111)
    //    return;

    ////��ȷ�����������Ե�һ�����������ʽ0x111111��ʼ֮����ǰɨ���Բ鿴�����������ҵ�0x222222�����ǵڶ�����������ĸ�ʽ
    ////���ǶԵ������͵��ĸ���Ŀִ����ͬ�Ĳ�����������еĳ��ȶ�ƥ�䣬���ǾͿ��Լٶ��ҵ�����Ŀ�ĳ���
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
    ////���ڣ�������Ҫɨ�����������Բ鿴�ĸ�ulong���������Լ����š���Ӧ����0x4000����Ϊ�����Ǵ�SbieSvc���õġ�����ȷ�������ĸ���Ŀ�е�ֵ����0x4000��Ȼ�����ǾͿ��Լٶ������ҵ�����ȷ������
    //ptr = Clipboard->items;
    //for (i = 0; (i < 12) && (*ptr != 0x4000); ++i, ++ptr)
    //    ;
    //if (*ptr != 0x4000) 
    //{
    //    //��������Ч����Ŀ���ȣ����Ҳ��������Լ����������ζ��UAC/UIPI�ѹر�
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