// stdafx.cpp : source file that includes just the standard includes
// AsyncHttpSample.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

wstring Convert_UTF8_UTF16(string aUTF8Value)
{
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(aUTF8Value);
}

string Convert_UTF16_UTF8(wstring aUTF16Value)
{
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(aUTF16Value);
}
