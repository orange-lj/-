#pragma once
//#include"pool.h"
#include"main.h"

typedef struct _PATTERN PATTERN;


/*
Pattern_Create�������ַ�����string�������ӳء�pool�������Pattern����
�����lower��ΪTRUE����ģʽ�ǻ���Դ��string����Сд�汾����ġ�
�����string��������ͨ��������ַ���*������ᴴ����·ģʽ��
����pattern_Match������Ϊwcscmp������
PATTERN������Ȼ��͸��������֤��һ��δʹ�õ�LIST_ELEM��Ա��ͷ����μ�LIST.h������������Բ��뵽�б���
*/
PATTERN* Pattern_Create(POOL* pool, const WCHAR* string, BOOLEAN lower, ULONG level);



int Pattern_MatchX(PATTERN* pat, const WCHAR* string, int string_len);

void Pattern_Free(PATTERN* pat);