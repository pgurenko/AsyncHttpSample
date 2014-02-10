#pragma once

#include <wininet.h>
#include <string>

using namespace std;

class AsyncHttpRequest;

class AsyncHttpResponse
{
private:
	
	static const DWORD m_dwBufSize;
	unique_ptr<char> m_spBuf;
	string m_Body;

	AsyncHttpRequest* m_pAsyncHttpRequest;

private:
	string GetLastResponseInfo(DWORD adwGetLastErrorResult);
public:
	AsyncHttpResponse(AsyncHttpRequest* apAsyncHttpRequest);
	~AsyncHttpResponse();

	int ProcessResponse(const INTERNET_ASYNC_RESULT* aAsyncResult);
	string GetHeader(string aHeaderName);
	string GetContentType();
	DWORD GetContentLength();
	string& GetBody();
};
