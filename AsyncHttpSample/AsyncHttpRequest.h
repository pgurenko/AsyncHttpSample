#pragma once

#include <string>
#include <vector>
#include <map>
#include <wininet.h>

using namespace std;

class AsyncHttpResponse;

#define HttpHeaders map<string, string>

class AsyncHttpRequest
{
public:
	
	class Host
	{
	protected:
		vector<shared_ptr<AsyncHttpRequest>> m_PendingRequests;

	protected:
		int HttpMakeRequest(const char* aszVerb, string aURL, string aReferrer = string(), DWORD adwContext = 0, HttpHeaders aHeaders = map<string, string>(), string aBody = string(), DWORD dwTimeOut = 5000);

	public:
		virtual void OnAsyncHttpRequestComplete(AsyncHttpRequest* apAsyncHttpRequest, DWORD adwResult);
	};

protected:
	
	static HINTERNET m_hInetInstance;

	static void WINAPI InternetStatusCallback(HINTERNET hInternet,
					DWORD dwContext,
					DWORD dwInternetStatus,
					LPVOID lpStatusInfo,
					DWORD dwStatusInfoLen);

	static int InitWinInet();

protected:

	Host* m_pHost;
	DWORD m_dwContext;
	HINTERNET m_hSession;
	HINTERNET m_hRequest;

	URL_COMPONENTSW m_URLComponents;

	wstring m_Verb;
	wstring m_Url;
	wstring m_Referrer;
	wstring m_HostName;
	wstring m_UrlPath;
	wstring m_ExtraInfo;

	string m_Body;

	unique_ptr<AsyncHttpResponse> m_spAsyncHttpResponse;
	HttpHeaders m_RequestHeaders;

public:

	AsyncHttpRequest(Host* apHost, DWORD adwContext = 0);
	~AsyncHttpRequest();

	int MakeRequest(const char* aszVerb, string& aURL, string& aReferrer, DWORD dwTimeOut = 5000);

	void SetHeaders(HttpHeaders& aHeaders);
	void SetBody(string& aBody);

	DWORD GetContext();
	AsyncHttpResponse* GetResponse();

	friend class AsyncHttpResponse;
};
