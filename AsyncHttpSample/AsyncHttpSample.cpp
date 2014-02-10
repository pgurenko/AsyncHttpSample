// AsyncHttpSample.cpp : Defines the entry point for the console application.
// Sample on clean and tidy way of making async WinApi http requests
// API used: WinInet
// Not suitable for Windows Server

#include "stdafx.h"
#include <conio.h>
#include "AsyncHttpRequest.h"
#include "AsyncHttpResponse.h"

class AsyncAsyncHttpRequestHost : AsyncHttpRequest::Host
{
public:
	AsyncAsyncHttpRequestHost(string aURL)
	{
		HttpMakeRequest("GET", aURL);
	}

	~AsyncAsyncHttpRequestHost()
	{
	}

	virtual void OnAsyncHttpRequestComplete(AsyncHttpRequest* apAsyncHttpRequest, DWORD adwResult)
	{
		printf("Response:\n%s", apAsyncHttpRequest->GetResponse()->GetBody().c_str());

		printf("Press any key to continue...");
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	printf("AsyncHttpSample - making WinApi async request nice\n");

	if(argc < 2)
	{
		printf("Usage: AsyncHttpSample <URL>");
		return 0;
	}

	printf("GETting %s", argv[1]);

	AsyncAsyncHttpRequestHost host(Convert_UTF16_UTF8(argv[1]));

	_getch();

	return 0;
}

