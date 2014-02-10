// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <Windows.h>

#include <string>
#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <algorithm>

using namespace std;

wstring Convert_UTF8_UTF16(string aUTF8Value);
string Convert_UTF16_UTF8(wstring aUTF16Value);
