#pragma once
//#include"pool.h"
#include"main.h"

typedef struct _PATTERN PATTERN;


/*
Pattern_Create：根据字符串“string”创建从池“pool”分配的Pattern对象。
如果“lower”为TRUE，则模式是基于源“string”的小写版本编译的。
如果“string”不包含通配符星形字符（*），则会创建短路模式，
其中pattern_Match（）变为wcscmp（）。
PATTERN对象虽然不透明，但保证以一个未使用的LIST_ELEM成员开头（请参见LIST.h），因此它可以插入到列表中
*/
PATTERN* Pattern_Create(POOL* pool, const WCHAR* string, BOOLEAN lower, ULONG level);



int Pattern_MatchX(PATTERN* pat, const WCHAR* string, int string_len);

void Pattern_Free(PATTERN* pat);