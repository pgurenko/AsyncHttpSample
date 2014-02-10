#include "StdAfx.h"
#include <memory>
#include <sstream>
#include <string>
#include <algorithm>
#include "AsyncHttpRequest.h"
#include "AsyncHttpResponse.h"

const DWORD AsyncHttpResponse::m_dwBufSize = 1024;

AsyncHttpResponse::AsyncHttpResponse(AsyncHttpRequest* apAsyncHttpRequest)
	: m_pAsyncHttpRequest(apAsyncHttpRequest),
	  m_spBuf(new char[m_dwBufSize])
{
}

AsyncHttpResponse::~AsyncHttpResponse()
{
}

string AsyncHttpResponse::GetLastResponseInfo(DWORD adwGetLastErrorResult)
{
	unique_ptr<char> spBuf(new char[m_dwBufSize]);

	DWORD dwMsgLength = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
		GetModuleHandle(TEXT("wininet.dll")),
		adwGetLastErrorResult,
		0,
		spBuf.get(),
		m_dwBufSize,
		NULL);

	string result(spBuf.get(), dwMsgLength);

	if(adwGetLastErrorResult == ERROR_INTERNET_EXTENDED_ERROR)
	{
		DWORD dwErrorCode;
		DWORD dwErrorMsgLength = 0;

		InternetGetLastResponseInfoA(&dwErrorCode, NULL, &dwErrorMsgLength);

		unique_ptr<char> spMsg(new char[dwErrorMsgLength]);
		if (!InternetGetLastResponseInfoA(&dwErrorCode, spMsg.get(), &dwErrorMsgLength))
		{
			printf("InternetGetLastResponseInfoA failed. GetLastError()=%p", GetLastError());
			return "";
		}

		result.assign(spMsg.get(), dwErrorMsgLength);
	}
	return result;
}

int AsyncHttpResponse::ProcessResponse(const INTERNET_ASYNC_RESULT* aAsyncResult)
{
	if(!aAsyncResult->dwResult)
	{
		printf("Request %p failed. dwError=%d Message=\"%s\"", this, aAsyncResult->dwError, GetLastResponseInfo(aAsyncResult->dwError).c_str());
		m_pAsyncHttpRequest->m_pHost->OnAsyncHttpRequestComplete(m_pAsyncHttpRequest, aAsyncResult->dwError);
		return -1;
	}

	DWORD dwBytesRead = 0;
	do
	{
		dwBytesRead = 0;

		if (!InternetReadFile(m_pAsyncHttpRequest->m_hRequest, m_spBuf.get(), m_dwBufSize, &dwBytesRead))
		{
			if (GetLastError() != ERROR_IO_PENDING)
			{
				printf("InternetReadFileEx failed. Request=%p GetLastError()=%d", this, GetLastError());
			}
			m_pAsyncHttpRequest->m_pHost->OnAsyncHttpRequestComplete(m_pAsyncHttpRequest, GetLastError());
			return -1;
		}

		m_Body.append(m_spBuf.get(), dwBytesRead);
	}
	while(dwBytesRead > 0);

	m_pAsyncHttpRequest->m_pHost->OnAsyncHttpRequestComplete(m_pAsyncHttpRequest, 0);

	return 0;
}

string AsyncHttpResponse::GetHeader(string aHeaderName)
{
	DWORD dwBufSize = 65535;

	wstring HeaderName = Convert_UTF8_UTF16(aHeaderName);

	unique_ptr<TCHAR> spBuf(new TCHAR[dwBufSize]);
	wcscpy_s(spBuf.get(), dwBufSize, HeaderName.c_str());

	BOOL bResult = HttpQueryInfo(m_pAsyncHttpRequest->m_hRequest, HTTP_QUERY_CUSTOM, spBuf.get(), &dwBufSize, NULL);
	if(!bResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		spBuf.reset(new TCHAR[dwBufSize / sizeof(TCHAR)]);
		wcscpy_s(spBuf.get(), dwBufSize, HeaderName.c_str());

		bResult = HttpQueryInfo(m_pAsyncHttpRequest->m_hRequest, HTTP_QUERY_CUSTOM, spBuf.get(), &dwBufSize, NULL);
	}

	if(!bResult)
	{
		printf("HttpQueryInfo failed with GetLastError()=%p", GetLastError());
		return "";
	}

	wstring result(spBuf.get(), dwBufSize / sizeof(TCHAR));
	return Convert_UTF16_UTF8(result);
}

string AsyncHttpResponse::GetContentType()
{
	DWORD dwBufSize = 65535;

	unique_ptr<TCHAR> spBuf(new TCHAR[dwBufSize]);
	if(!HttpQueryInfo(m_pAsyncHttpRequest->m_hRequest, HTTP_QUERY_CONTENT_TYPE, spBuf.get(), &dwBufSize, NULL))
	{
		printf("HttpQueryInfo failed with GetLastError()=%p", GetLastError());
		return "";
	}

	wstring result(spBuf.get(), dwBufSize / sizeof(TCHAR));
	return Convert_UTF16_UTF8(result);
}

DWORD AsyncHttpResponse::GetContentLength()
{
	DWORD dwBufSize = 65535;

	unique_ptr<TCHAR> spBuf(new TCHAR[dwBufSize]);
	if(!HttpQueryInfo(m_pAsyncHttpRequest->m_hRequest, HTTP_QUERY_CONTENT_LENGTH, spBuf.get(), &dwBufSize, NULL))
	{
		printf("HttpQueryInfo failed with GetLastError()=%p", GetLastError());
		return 0;
	}
	return _tcstoul(spBuf.get(), NULL, 10);
}

string& AsyncHttpResponse::GetBody()
{
	return m_Body;
}
