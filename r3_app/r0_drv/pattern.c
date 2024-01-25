#include "pattern.h"
#include"../common/defines.h"
#include"../common/pool.h"
static const WCHAR* Pattern_Hex = L"__hex";
static const WCHAR* Pattern_wcsnstr(
    const WCHAR* hstr, const WCHAR* nstr, int nlen);
static int Pattern_Match2(
    PATTERN* pat,
    const WCHAR* string, int string_len,
    int str_index, int con_index);

struct _PATTERN {

    // ���÷�����ʹ��δʹ�õ�list_elem
    LIST_ELEM list_elem;

    // length of the entire PATTERN object
    ULONG length;

    // pattern info
    union {
        ULONG v;
        USHORT num_cons;        // number of constant parts
        struct {
            int unused : 16;
            int star_missing : 1;
            int star_at_head : 1;
            int star_at_tail : 1;
            int have_a_qmark : 1;
        } f;
    } info;

    // ָ��Դģʽ�ַ�����ָ�룬��Ϊ��pattern�����һ���ַ���
    WCHAR* source;

    //��ʾ����ƥ�伶���ֵ
    ULONG level;

    //ָ�������ֵ�ָ�����顣Ԫ�ص�ʵ��������info.num_cons��ʾ��
    //�ַ�������Ϊ��PATTERN�����һ���ַ����
    struct {
        BOOLEAN hex;
        USHORT len;
        WCHAR* ptr;
    } cons[0];

};


PATTERN* Pattern_Create(POOL* pool, const WCHAR* string, BOOLEAN lower, ULONG level)
{
    ULONG num_cons;
    const WCHAR* iptr;
    const WCHAR* iptr2;
    ULONG len_ptr;
    ULONG len_pat;
    PATTERN* pat;
    WCHAR* optr;
    BOOLEAN any_hex_cons;

    //���������ַ����г������ֵ��������Լ���Щ����ʹ�õ��ֽ���
    num_cons = 0;
    len_pat = sizeof(PATTERN);
    if (string)
        len_pat += wcslen(string) * sizeof(WCHAR);
    len_pat += sizeof(WCHAR);

    iptr = string;
    while (iptr) {

        while (*iptr == L'*')
            ++iptr;
        iptr2 = wcschr(iptr, L'*');

        if (iptr2) {
            len_ptr = (ULONG)(iptr2 - iptr);
            ++iptr2;
        }
        else
            len_ptr = wcslen(iptr);

        if (len_ptr) {
            len_pat += (len_ptr + 1) * sizeof(WCHAR)
                // plus one entry in cons array:
                + sizeof(((PATTERN*)NULL)->cons[0]);
            ++num_cons;
        }

        iptr = iptr2;
    }
    //����������³��ȵ�PATTERN����
        //-ͼ���ṹ�Ĵ�С
        //��Դ�ַ����ĳ��ȣ����ֽ�Ϊ��λ������NULL�ַ�
        //-�������ֵĳ��ȣ����ֽ�Ϊ��λ��ÿ�����ֶ�����һ��NULL�ַ�
        //-�������ֵ�����*sizeof��WCHAR*��������ָ�����飬��������������㡣
    pat = (PATTERN*)Pool_Alloc(pool, len_pat);
    if (!pat)
        return NULL;

    memzero(&pat->list_elem, sizeof(LIST_ELEM));
    pat->length = len_pat;
    //���������ָ��Ƶ�ģʽ�С����Ǹ���PATTERN�д�cons������濪ʼ�Ĳ����ַ�����
    //�����������Ԫ��ָ��ÿ�����Ƶ��ַ���
    any_hex_cons = FALSE;

    optr = (WCHAR*)&pat->cons[num_cons];
    num_cons = 0;
    iptr = string;
    while (iptr) 
    {
        while (*iptr == L'*')
            ++iptr;
        iptr2 = wcschr(iptr, L'*');

        if (iptr2) 
        {
            len_ptr = (ULONG)(iptr2 - iptr);
            ++iptr2;
        }
        else
            len_ptr = wcslen(iptr);

        if (len_ptr) 
        {
            //���������ֵ��ַ�����������֮ǰ
            pat->cons[num_cons].len = (USHORT)len_ptr;
            pat->cons[num_cons].ptr = optr;

            wmemcpy(optr, iptr, len_ptr);
            optr[len_ptr] = L'\0';
            if (lower)
                _wcslwr(optr);

            if (Pattern_wcsnstr(optr, Pattern_Hex, 5)) {
                any_hex_cons = TRUE;
                pat->cons[num_cons].hex = TRUE;
            }
            else
                pat->cons[num_cons].hex = FALSE;

            ++num_cons;
            optr += len_ptr + 1;
        }
        iptr = iptr2;
    }
    //��Դ�ַ�������ģʽ�У��������г������֣�Ȼ���ʼ����Ϣ��
    if (string)
        wcscpy(optr, string);
    else
        *optr = L'\0';
    pat->source = optr;

    pat->level = level;

    pat->info.v = 0;
    pat->info.num_cons = (USHORT)num_cons;

    if (string && string[0] == L'*')
        pat->info.f.star_at_head = TRUE;
    if (string && string[wcslen(string) - 1] == L'*')
        pat->info.f.star_at_tail = TRUE;

    if (num_cons <= 1 &&
        (!pat->info.f.star_at_head) &&
        (!pat->info.f.star_at_tail) &&
        (!any_hex_cons))
    {
        pat->info.f.star_missing = TRUE;

        if (wcschr(string, L'?'))
            pat->info.f.have_a_qmark = TRUE;
    }

    //
    // we're done
    //

    return pat;
}

int Pattern_MatchX(PATTERN* pat, const WCHAR* string, int string_len)
{
    //��·������ַ���ΪNULL���������ģʽΪNULL��
    //����FALSE�����ģʽû��ͨ����ǣ���ʹ�ü򵥵�
    //�ַ����Ƚ�
    if (!string)
        return 0;

    if (pat->info.f.star_missing) {

        if (pat->info.num_cons == 0)
            return 0;
        if (string_len != pat->cons[0].len)
            return 0;

        if (pat->info.f.have_a_qmark) {

            const WCHAR* x = Pattern_wcsnstr(
                string, pat->cons[0].ptr, pat->cons[0].len);
            if (x != string)
                return 0;

        }
        else {

            ULONG x = wmemcmp(string, pat->cons[0].ptr, pat->cons[0].len);
            if (x != 0)
                return 0;
        }

        return string_len;
    }
    //��������Ǻ����ַ�����Ч
    return Pattern_Match2(pat, string, string_len, 0, 0);
}

void Pattern_Free(PATTERN* pat)
{
    Pool_Free(pat, pat->length);
}

const WCHAR* Pattern_wcsnstr(const WCHAR* hstr, const WCHAR* nstr, int nlen)
{
    int i;
    while (*hstr) {
        if (*hstr == *nstr || *nstr == L'?') {
            for (i = 0; i < nlen; ++i) {
                if ((hstr[i] != nstr[i]) &&
                    (hstr[i] == L'\0' || nstr[i] != L'?'))
                    break;
            }
            if (i == nlen)
                return hstr;
        }
        ++hstr;
    }
    return NULL;
}

int Pattern_Match2(PATTERN* pat, const WCHAR* string, int string_len, int str_index, int con_index)
{
    return 0;
}
