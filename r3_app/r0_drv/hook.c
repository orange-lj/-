#include "hook.h"
#include"syscall.h"
#include"../common/defines.h"
#define NUM_TARGET_PAGES    4
#define MAX_DEPTH           10
#define _F_ADDR_SIZE(f)     (((f) & 0xFFFF) >> 8)
#define F_ADDR_SIZE()       (_F_ADDR_SIZE(inst->flags))
#define F_64_BIT            (0xF0 << 16)
#define F_MAKE_COMBO(dataSize, addrSize, rex)   \
    (                                           \
        (((dataSize) & 0xFF)) |                 \
        (((addrSize) & 0xFF) << 8) |            \
        (((rex) & 0xFF) << 16)                  \
    )

//结构体
#pragma pack(push)
#pragma pack(1)


typedef struct _SERVICE_DESCRIPTOR {

    LONG* Addrs;
    ULONG* DontCare1;   // always zero?
    ULONG Limit;
    LONG DontCare2;     // always zero?
    UCHAR* DontCare3;

} SERVICE_DESCRIPTOR;


#pragma pack(pop)



//变量
/*图例：*/
/*0-操作码无效或未处理，或在此表外处理*/
/*1-操作码、modrm、可选显示*/
/*2-操作码，立即8*/
/*3-操作码，立即16/32*/
/*4-操作码，modrm，立即8*/
/*5-操作码，modrm，立即16/32*/
/*6-操作码，仅此而已*/
/*7-操作码，地址32/64(由F_ADDR_SIZE选择)*/
static const UCHAR Hook_Opcodes_1[256] = {

    /*   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* 0 */  1, 1, 1, 1, 2, 3, 6, 6, 1, 1, 1, 1, 2, 3, 6, 0,
    /* 1 */  1, 1, 1, 1, 2, 3, 6, 6, 1, 1, 1, 1, 2, 3, 6, 6,
    /* 2 */  1, 1, 1, 1, 2, 3, 0, 6, 1, 1, 1, 1, 2, 3, 0, 6,
    /* 3 */  1, 1, 1, 1, 2, 3, 0, 6, 1, 1, 1, 1, 2, 3, 0, 6,
    /* 4 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /* 5 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /* 6 */  6, 6, 1, 1, 1, 0, 0, 0, 3, 0, 2, 0, 6, 6, 6, 6,
    /* 7 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 8 */  4, 5, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1,
    /* 9 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6,
    /* A */  7, 7, 7, 7, 6, 6, 6, 6, 2, 3, 6, 6, 6, 6, 6, 6,
    /* B */  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    /* C */  4, 4, 3, 6, 0, 0, 4, 5, 0, 6, 3, 6, 6, 2, 6, 6,
    /* D */  1, 1, 1, 1, 2, 2, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0,
    /* E */  2, 2, 2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 6, 6, 6, 6,
    /* F */  0, 0, 0, 0, 6, 6, 0, 0, 6, 6, 6, 6, 6, 6, 0, 0
};

static LONG Hook_FindSyscall2(
    void* addr, int depth, ULONG_PTR* targets, BOOLEAN check_target,
    LONG eax, LONG* skip);
static UCHAR* Hook_Analyze_ModRM(
    UCHAR* modrm, HOOK_INST* inst);
static UCHAR* Hook_Analyze_Prefix(
    UCHAR* addr, BOOLEAN is64, ULONG* flags);
static UCHAR* Hook_Analyze_Inst(UCHAR* addr, HOOK_INST* inst);
static UCHAR* Hook_Analyze_CtlXfer(UCHAR* addr, HOOK_INST* inst);
static LONG Hook_CheckSkippedSyscall(LONG eax, LONG* skip);
static ULONG Hook_Find_ZwRoutine(ULONG service_index, void** out_routine);


BOOLEAN Hook_GetService(void* DllProc, LONG* SkipIndexes, ULONG ParamCount, void** NtService, void** ZwService)
{
    LONG svc_num;
    void* svc_addr;

    if (NtService)
        *NtService = NULL;
    if (ZwService)
        *ZwService = NULL;

    if ((ULONG_PTR)DllProc < 0x10000) 
    {
        svc_num = (ULONG)(ULONG_PTR)DllProc;
    }
    else
    {
        svc_num = Hook_GetServiceIndex(DllProc, SkipIndexes);
        if (svc_num == -1)
            return FALSE;

    }
    svc_addr = Hook_GetNtServiceInternal(svc_num, ParamCount);
    if (!svc_addr)
        return FALSE;

    if (NtService)
        *NtService = svc_addr;
    if (ZwService) 
    {
        svc_addr = Hook_GetZwServiceInternal(svc_num);
        if (!svc_addr)
            return FALSE;
        *ZwService = svc_addr;
    }
    return TRUE;
}

LONG Hook_GetServiceIndex(void* DllProc, LONG* SkipIndexes)
{
    static LONG SkipIndexes_default[1] = { 0 };
    ULONG_PTR* targets;
    LONG eax;

    if (!SkipIndexes)
        SkipIndexes = SkipIndexes_default;

    targets = (ULONG_PTR*)ExAllocatePoolWithTag(
        PagedPool, PAGE_SIZE * NUM_TARGET_PAGES, tzuk);
    if (!targets) 
    {
        //Log_Msg0(MSG_1104);
        return -1;
    }
    targets[0] = 0;
    eax = Hook_FindSyscall2(DllProc, 1, targets, TRUE, -1, SkipIndexes);
    ExFreePoolWithTag(targets, tzuk);

    if ((eax != -1) && (eax < 0x2000) && ((eax & 0xFFF) < 0x600))
        return eax;

    return -1;

}

void* Hook_GetNtServiceInternal(ULONG ServiceIndex, ULONG ParamCount)
{
    SERVICE_DESCRIPTOR* Descr;
    ULONG TableCount;
    LONG_PTR offset;
    void* value;
    WCHAR err[2];

    SERVICE_DESCRIPTOR* ShadowTable = Syscall_GetServiceTable();
    if (!ShadowTable)
        return NULL;
    value = NULL;
    err[0] = err[1] = 0;
    if (ServiceIndex >= 0x2000 || (ServiceIndex & 0xFFF) >= 0x600) {
        //err[0] = L'1';      // invalid service number
        //goto finish;
    }
    if (ServiceIndex & 0x1000) {
        Descr = &ShadowTable[1];
        ServiceIndex &= ~0x1000;
    }
    else
        Descr = &ShadowTable[0];
    //64位服务表只计算四个以上参数的数量，因为前四个参数是在寄存器中传递的
    TableCount = (ULONG)(Descr->Addrs[ServiceIndex] & 0x0F);
    if ((ParamCount <= 4 && TableCount != 0) ||
        (ParamCount > 4 && TableCount != ParamCount - 4)) {

        //err[0] = L'2';      // parameter count mismatch
        //goto finish;
    }
    offset = (LONG_PTR)Descr->Addrs[ServiceIndex];
    offset >>= 4;

    value = (UCHAR*)Descr->Addrs + offset;
finish:
    if (err[0])
        //Log_Msg1(MSG_HOOK_NT_SERVICE, err);

    return value;

}

void* Hook_GetZwServiceInternal(ULONG ServiceIndex)
{
    ULONG subcode;
    void* routine;

    routine = NULL;
    subcode = Hook_Find_ZwRoutine(ServiceIndex, &routine);
    if (subcode != 0) {
        //WCHAR err[8];
        //RtlStringCbPrintfW(err, sizeof(err), L"%d", subcode);
        //Log_Msg1(MSG_HOOK_ZW_SERVICE, err);
        //routine = NULL;
    }
    return routine;
}

BOOLEAN Hook_Analyze(void* address, BOOLEAN probe_address, BOOLEAN is64, HOOK_INST* inst)
{
    UCHAR* addr;
    WCHAR text[64];

    memzero(inst, sizeof(HOOK_INST));
    __try 
    {
        if (probe_address)
            ProbeForRead(address, 16, sizeof(UCHAR));
        addr = Hook_Analyze_Prefix(address, is64, &inst->flags);
        inst->op1 = addr[0];
        inst->op2 = addr[1];
        addr = Hook_Analyze_Inst(addr, inst);
        if (!addr) 
        {
        
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        addr = NULL;
    }
    if (!addr) 
    {
        return FALSE;
    }
    inst->len = (ULONG)(addr - (UCHAR*)address);
    return TRUE;
}

LONG Hook_FindSyscall2(void* addr, int depth, ULONG_PTR* targets, BOOLEAN check_target, LONG eax, LONG* skip)
{
    HOOK_INST inst;
    ULONG edx;
    ULONG i;
    BOOLEAN is64;
    ULONG count;

    is64 = TRUE;
    if (depth > MAX_DEPTH)
        return -1;
    //对于每个起始地址，我们将其记录在跳转目标列表中，以便下次处理该地址时可以忽略它。
    //请注意，当我们在处理“Call edX”或“Call[edX]”之后进入递归调用时，CHECK_TARGET为FALSE，如下所示。
    //在本例中，我们不检查跳转目标，因为我们希望每次都重新分析通向SYSENTER的序列
    if (check_target) 
    {
        for (i = 0;i < (ULONG)targets[0];++i) 
        {
            if (targets[i] == (ULONG_PTR)addr) 
            {
                return -1;
            }
        }
        ++i;
        if (i > (PAGE_SIZE * NUM_TARGET_PAGES / sizeof(ULONG_PTR) - 8))
            return -1;
        targets[i] = (ULONG_PTR)addr;
        targets[0] = (ULONG)i;
    }
    check_target = TRUE;
    //分析从传递的地址开始的指令
    for (count = 0; count < 1024; ++count) 
    {
        BOOLEAN ok = Hook_Analyze(addr, TRUE, is64, &inst);
        if ((!ok) || inst.kind == INST_RET || inst.kind == INST_JUMP_MEM)
            return -1;
        if (inst.kind == INST_SYSCALL) 
        {
            
            eax = Hook_CheckSkippedSyscall(eax, skip);
            if (eax != -1)
                return eax;
        }
        if (inst.kind == INST_CTLXFER) 
        {
            eax = Hook_FindSyscall2(
                (void*)inst.parm, depth + 1,
                targets, check_target,
                eax, skip);

            check_target = TRUE;

            eax = Hook_CheckSkippedSyscall(eax, skip);
            if (eax != -1)
                return eax;

            if (inst.op1 == 0xE9 ||     // if it was a jump, then stop
                inst.op1 == 0xEB)
                return -1;

        }
        edx = 0;                        // reset edx on every cycle
        if (inst.op1 == 0xB8)           // mov eax, imm32
            eax = (LONG)inst.parm;
        else if (inst.op1 == 0xBA)      // mov edx, imm32
            edx = (ULONG)inst.parm;
        addr = ((UCHAR*)addr) + inst.len;
    }
}

UCHAR* Hook_Analyze_ModRM(UCHAR* modrm, HOOK_INST* inst)
{
    UCHAR disp;
    //如果操作码暗示modrm字段，我们需要跳过一个或两个字节的modrm字段，并可能跳过额外的一个或四个字节的位移
    inst->modrm = modrm;
    //表示不移位
    disp = 0xFF;
    //mod==11b：单字节modrm，无位移
    if ((modrm[0] & 0xC0) == 0xC0)
        //跳过modrm的一个字节
        ++modrm;   
    // (mod != 11b) && (rm == 100b) && (! 16-bit):  two-byte modrm.
    // displacement is governed by mod and base fields,
    // where base is low 3 bits of second modrm byte (aka SIB).
    // interpretation of mod+base is same as that of
    // mod+rm for purposes of deciding size of displacement
    else if ((F_ADDR_SIZE() != 2) && (modrm[0] & 7) == 4) {
        disp = (modrm[0] & 0xC0) | (modrm[1] & 7);
        modrm += 2;             // skip two bytes of modrm

    // (mod != 11b) && (rm != 100b):  one-byte modrm.
    // displacement is governed by mod+rm fields, see
    // interpretation below
    }
    else {
        disp = (modrm[0] & 0xC0) | (modrm[0] & 7);
        ++modrm;                // skip one byte of modrm
    }

    // displacement is one byte if mod == 01b,
    // four bytes if mod == 10b (option 1), or
    // four bytes if mod == 00b and rm/base == 101b (option 2)
    if (disp != 0xFF) {
        if ((disp & 0xC0) == 0x40)
            ++modrm;            // skip 8-bit displacement
        else if ((disp & 0xC0) == 0x80 ||
            (disp & 7) == 5)
            modrm += 4;         // skip 32-bit displacement
    }

    return modrm;
}

UCHAR* Hook_Analyze_Prefix(UCHAR* addr, BOOLEAN is64, ULONG* flags)
{
    ULONG rex, osize, asize;
    UCHAR b;

    rex = 0;
    osize = 4;
    asize = is64 ? 8 : 4;

    while (1) {

        b = *addr;
        ++addr;

        if (b == 0x2E ||                // CS: override prefix 2E
            b == 0x3E ||                // DS: override prefix 3E
            b == 0x26 ||                // ES: override prefix 26
            b == 0x64 ||                // FS: override prefix 64
            b == 0x65 ||                // GS: override prefix 65
            b == 0x36 ||                // SS: override prefix 36
            b == 0xF0 ||                // LOCK prefix
            b == 0xF3 ||                // REP or REPZ prefix
            b == 0xF2)                  // REPNZ prefix
            continue;

        if (b == 0x66) {                // Operand-size override prefix 66
            osize = 2;
            continue;
        }

        if (b == 0x67) {                // Address-size override prefix 67
            asize = is64 ? 4 : 2;
            continue;
        }

        if (is64 && (b & 0xF0) == 0x40) {   // REX prefix 4x
            rex = b & 0x0F;
            continue;
        }

        --addr;                         // Not a prefix byte
        break;
    }

    if (is64 && (rex & 8))              // REX.W prefix
        osize = 8;

    // 在64位模式下，64位的Addr或数据通常从指令中仅编码的32位进行符号扩展。仅在两种情况下，指令实际编码完整的64位信息
    //
    // - moving to/from accum register:                 opcodes A0..A3
    //   - this is the default, unless overridden
    //     by prefix 67
    //
    // - moving immediate value to any register:        opcodes B8..BF
    //   - prefix 48, REX.W, must be specified for this

    if (asize == 8 && (b < 0xA0 || b > 0xA3))
        asize = 4;
    if (osize == 8 && (b < 0xB8 || b > 0xBF))
        osize = 4;

    // 将前缀信息编码到标志中

    *flags = F_MAKE_COMBO(osize, asize, rex);
    if (is64)
        *flags |= F_64_BIT;

    return addr;

}

UCHAR* Hook_Analyze_Inst(UCHAR* addr, HOOK_INST* inst)
{
    UCHAR* addr2;
    UCHAR op, op2;

    addr2 = Hook_Analyze_CtlXfer(addr, inst);
    if (addr2 != addr)
        return addr2;

    op = addr[0];
    op2 = addr[1];
    switch (Hook_Opcodes_1[op]) 
    {
    case 1:
        return Hook_Analyze_ModRM(addr + 1, inst);
    }
}

UCHAR* Hook_Analyze_CtlXfer(UCHAR* addr, HOOK_INST* inst)
{
    UCHAR op1, op2;
    LONG rel32;
    BOOLEAN have_rel32;

    op1 = addr[0];
    op2 = addr[1];
    rel32 = 0;
    have_rel32 = FALSE;
    //系统控制权转移
    if ((op1 == 0x0F && op2 == 0x05) ||                 // syscall
        (op1 == 0x0F && op2 == 0x34) ||                 // sysenter
        (op1 == 0xCD && op2 == 0x2E)) 
    {                 // int 2e

        //inst->kind = INST_SYSCALL;
        //addr += 2;
    }

    if (op1 == 0xCC) 
    {
        //inst->kind = INST_RET;
        //++addr;
    }

    // return control transfer

    if (op1 == 0xC3 || op1 == 0xCB ||                   // ret, retf
        op1 == 0xC2 || op1 == 0xCA) 
    {                   // ret, retf nnn

        //inst->kind = INST_RET;
        //if (op1 & 1)
        //    ++addr;
        //else
        //    addr += 3;
    }

    // short jumps with an 8-bit displacement

    if ((op1 >= 0x70 && op1 <= 0x7F) ||                 // jcc disp8
        (op1 >= 0xE0 && op1 <= 0xE3) ||                 // loop/jcxz disp8
        (op1 == 0xEB)) 
    {                                // jmp disp8

        //have_rel32 = TRUE;
        //rel32 = (LONG) * (CHAR*)(addr + 1);
        //addr += 2;
    }

    // control transfer to a near 32-bit displacement

    if (op1 == 0x0F && (op2 & 0xF0) == 0x80) 
    {          // jcc disp32

        //have_rel32 = TRUE;
        //rel32 = *(LONG*)(addr + 2);
        //addr += 6;
    }

    if (op1 == 0xE8 || op1 == 0xE9) 
    {                   // jmp/call disp32
        //have_rel32 = TRUE;
        //inst->rel32 = (LONG*)(addr + 1);
        //rel32 = *inst->rel32;
        //addr += 5;
    }

    // double-byte instructions

    if (op1 == 0xFF) 
    {

        // control transfer using a register, no displacement

        //if ((op2 >= 0xD0 && op2 <= 0xD7) ||             // call reg
        //    (op2 >= 0xE0 && op2 <= 0xE7)) {             // jmp reg
        //
        //    inst->kind = INST_CTLXFER_REG;
        //    inst->parm = op2 & 0x0F;
        //    addr += 2;
        //
        //}
        //else if (op2 == 0x12) {
        //
        //    // control transfer to [edx]
        //
        //    inst->kind = INST_CTLXFER_REG;
        //    inst->parm = 0x80 | (op2 & 0x0F);
        //    addr += 2;
        //
        //}
        //else {
        //
        //    // control transfer using a modrm field
        //
        //    if ((op2 & 0x38) == 0x10 ||                     // call
        //        (op2 & 0x38) == 0x18 ||                     // call
        //        (op2 & 0x38) == 0x20 ||                     // jmp
        //        (op2 & 0x38) == 0x28) {                     // jmp
        //
        //        inst->kind = (op2 >= 0x20) ? INST_JUMP_MEM : INST_CALL_MEM;
        //        addr = Hook_Analyze_ModRM(addr + 1, inst);
        //    }
        //}

    }

    // far jump through call gate to absolute address

    if (op1 == 0xEA) 
    {

        //USHORT seg = *(USHORT*)(addr + 5);
        //if (seg == 0x001B || seg == 0x0023) {   // application CS == 0x001B
        //    inst->kind = INST_CTLXFER;          // on 64-bit, also 0023 ?
        //    inst->parm = *(ULONG*)(addr + 1);
        //    addr += 7;
        //}
    }

    // fix relative target address

    if (have_rel32) 
    {
        //inst->kind = INST_CTLXFER;
        //inst->parm = (ULONG_PTR)addr + rel32;
    }

    return addr;

}

LONG Hook_CheckSkippedSyscall(LONG eax, LONG* skip)
{
    LONG i;
    for (i = 0; i < skip[0]; ++i)
        if (eax == skip[i + 1])
            return -1;
    return eax;
}

ULONG Hook_Find_ZwRoutine(ULONG ServiceNum, void** out_routine)
{
    UCHAR* addr;
    BOOLEAN found = FALSE;
    int i;

    //在64位Windows上，ZwXxx函数的顺序似乎是任意的

    UCHAR* first_routine = (UCHAR*)ZwWaitForSingleObject;
    UCHAR* last_routine = (UCHAR*)ZwUnloadKey + 0x140;

    // 扫描NTOSKRNL中的ZwXxx函数，以找到调用系统服务的索引

    i = 0;
    addr = first_routine;
    while (addr != last_routine) {

        UCHAR* save_addr = addr;

        // DbgPrint("Address %X Byte %X\n", addr, *addr);

        if (*(USHORT*)addr == 0xC033 && addr[2] == 0xC3) { // $Workaround$ - 3rd party fix
            // HAL7600 activation tool overwrites ZwLockProductActivationKeys
            // with 33 C0 C3 (xor eax,eax ; ret), but leaves the original
            // "xchg ax,ax" at the end of the original code, so we try to
            // locate this and advance to the next redirector
            for (i = 3; i < 36; ++i) {
                if (*(USHORT*)(addr + i) == 0x9066)
                    break;
            }
            if (i < 36) {
                addr += i + 2;
                continue;
            }
        }

        // 在XP 64位中，ZwXxx系统服务重定向器如下所示 :-
        // 0x48 8B C4 FA 48 83 EC xx 50 9C 6A xx 48 8D xx xx xx xx xx 50 ...
        //  ... B8 xx xx xx xx E9 xx xx xx xx 66 90

        // ZwXxx系统服务重定向器在Windows 10 64位中看起来像这样
        // 48 8B C4 FA 48 83 EC xx 50 9C 6A xx 48 8D xx xx
        // xx xx xx 50 B8 xx xx xx xx E9 xx xx xx xx C3 90

        if (addr[0] != 0x48 || addr[1] != 0x8B)
            break;
        addr += 4;
        if (addr[0] != 0x48 || addr[1] != 0x83)
            break;
        addr += 8;
        if (addr[0] != 0x48 || addr[1] != 0x8D)
            break;
        addr += 8;

        if (addr[0] != 0xB8)
            break;
        if (*(ULONG*)(addr + 1) == ServiceNum) {
            addr = save_addr;
            found = TRUE;
            break;
        }
        addr += 5;

        if (addr[0] != 0xE9)
            break;
        addr += 5;

        if ((addr[0] != 0x66 && addr[0] != 0xC3) || addr[1] != 0x90)
            break;
        addr += 2;
    }

    if ((!found) && (addr != last_routine)) {
        return 0x24;
    }

    if (found) {
        *out_routine = addr;
        return 0;
    }
    else
        return 0x29;
}
