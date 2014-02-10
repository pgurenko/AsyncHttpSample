#include "StdAfx.h"
#include <memory>
#include <algorithm>

#include "AsyncHttpRequest.h"
#include "AsyncHttpResponse.h"

void* AsyncHttpRequest::m_hInetInstance = NULL;

int AsyncHttpRequest::Host::HttpMakeRequest(const char* aszVerb, string aURL, string aReferrer, DWORD adwContext, HttpHeaders aHeaders, string aBody, DWORD dwTimeOut)
{
	printf("HttpMakeRequest %s %s Ref=%s dwTimeOut=%d", aszVerb, aURL.c_str(), aReferrer == "" ? "<empty>" : aReferrer.c_str(), dwTimeOut);

	shared_ptr<AsyncHttpRequest> spAsyncHttpRequest(new AsyncHttpRequest(this, adwContext));
	spAsyncHttpRequest->SetHeaders(aHeaders);
	spAsyncHttpRequest->SetBody(aBody);

	if(spAsyncHttpRequest->MakeRequest(aszVerb, aURL, aReferrer, dwTimeOut))
	{
		OnAsyncHttpRequestComplete(spAsyncHttpRequest.get(), -1);
		return -1;
	}

	m_PendingRequests.push_back(spAsyncHttpRequest);

	return 0;
}

void AsyncHttpRequest::Host::OnAsyncHttpRequestComplete(AsyncHttpRequest* apAsyncHttpRequest, DWORD adwResult)
{
	for(vector<shared_ptr<AsyncHttpRequest>>::iterator iter = m_PendingRequests.begin();
		iter != m_PendingRequests.end();
		iter++)
	{
		if(iter->get() == apAsyncHttpRequest)
		{
			m_PendingRequests.erase(iter);
			break;
		}
	}
}

AsyncHttpRequest::AsyncHttpRequest(Host* apHost, DWORD adwContext)
	: m_pHost(apHost),
	  m_dwContext(adwContext)
{
	m_hSession	= NULL;
	m_hRequest	= NULL;
}

AsyncHttpRequest::~AsyncHttpRequest()
{
	if (m_hSession)
	{
		InternetCloseHandle(m_hSession);
		m_hSession = NULL;
	}

	if (m_hRequest)
	{
		InternetCloseHandle(m_hRequest);
		m_hRequest = NULL;
	}
}

void WINAPI AsyncHttpRequest::InternetStatusCallback(HINTERNET hInternet,
												DWORD dwContext,
												DWORD dwInternetStatus,
												LPVOID lpStatusInfo,
												DWORD dwStatusInfoLen)
{
	AsyncHttpResponse* pResponse = (AsyncHttpResponse*)dwContext;
	const INTERNET_ASYNC_RESULT* pAsyncResult = (const INTERNET_ASYNC_RESULT*)lpStatusInfo;

	switch (dwInternetStatus)
	{
		case INTERNET_STATUS_REQUEST_COMPLETE:
			printf("REQUEST_COMPLETE: %p", dwContext);
			pResponse->ProcessResponse(pAsyncResult);
			break;
		default:
			break;
	}
}

int AsyncHttpRequest::InitWinInet()
{
	DWORD dwResult = InternetAttemptConnect(0);
	if(dwResult != ERROR_SUCCESS)
	{
		printf("InternetAttemptConnect failed. Result=%p", dwResult);
		return -1;
	}

	if(InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0) == FALSE)
	{
		printf("InternetCheckConnection failed. GetLastError()=%p", GetLastError());
		return -1;
	}

	m_hInetInstance = InternetOpen(_T("UCWALib / 1.0"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
	if (!m_hInetInstance)
	{
		printf("InternetOpen failed. GetLastError()=%p", GetLastError());
		return -1;
	}

	if (InternetSetStatusCallback(m_hInetInstance, &InternetStatusCallback) == INTERNET_INVALID_STATUS_CALLBACK)
	{
		printf("InternetSetStatusCallback returned INTERNET_INVALID_STATUS_CALLBACK");
		return -1;
	}
	return 0;
}

void AsyncHttpRequest::SetHeaders(HttpHeaders& aHeaders)
{
	m_RequestHeaders = aHeaders;
}

void AsyncHttpRequest::SetBody(string& aBody)
{
	m_Body = aBody;
}

int AsyncHttpRequest::MakeRequest(const char* aszVerb, string& aURL, string& aReferrer, DWORD dwTimeOut)
{
	if(!m_hInetInstance)
	{
		int iResult = InitWinInet();
		if(iResult)
			return -1;
	}

	m_Verb = Convert_UTF8_UTF16(string(aszVerb));
	m_Url = Convert_UTF8_UTF16(aURL);
	m_Referrer = Convert_UTF8_UTF16(aReferrer);

	::ZeroMemory(&m_URLComponents, sizeof(m_URLComponents));
	m_URLComponents.dwStructSize = sizeof(m_URLComponents);

	m_URLComponents.dwHostNameLength = 1;
	m_URLComponents.dwUrlPathLength = 1;
	m_URLComponents.dwExtraInfoLength = 1;

	if (!InternetCrackUrl(m_Url.c_str(), 0, 0, &m_URLComponents))
	{
		printf("InternetCrackUrl failed. GetLastError()=%p", GetLastError());
		return -1;
	}

	m_HostName.assign(m_URLComponents.lpszHostName, m_URLComponents.dwHostNameLength);
	m_UrlPath.assign(m_URLComponents.lpszUrlPath, m_URLComponents.dwUrlPathLength);
	m_ExtraInfo.assign(m_URLComponents.lpszExtraInfo, m_URLComponents.dwExtraInfoLength);

	wstring UrlPathWithExtra = m_UrlPath + m_ExtraInfo;

	m_hSession = InternetConnect(m_hInetInstance, 
		m_HostName.c_str(),
		m_URLComponents.nPort,
		NULL,
		NULL,
		INTERNET_SERVICE_HTTP,
		INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,
		(DWORD)this);
	
	if(!m_hSession)
	{
		printf("InternetConnect failed. GetLastError()=%p", GetLastError());
		return -1;
	}

	m_spAsyncHttpResponse.reset(new AsyncHttpResponse(this));

	DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT;
	
	if(m_URLComponents.nScheme == INTERNET_SCHEME_HTTPS)
	{
		dwFlags |= INTERNET_FLAG_SECURE;
	}

	m_hRequest = HttpOpenRequest(m_hSession, 
		m_Verb.c_str(),
		UrlPathWithExtra.c_str(),
		NULL,
		m_Referrer.c_str(),
		NULL,
		dwFlags,
		(DWORD)m_spAsyncHttpResponse.get());

	if(!m_hRequest)
	{
		printf("HttpOpenRequest failed. GetLastError()=%p", GetLastError());
		return -1;
	}

	if(!InternetSetOption(m_hRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeOut, sizeof(DWORD)))
	{
		printf("InternetSetOption failed. GetLastError()=%p", GetLastError());
		return -1;
	}

	stringstream ss;
	ss << m_Body.size();
	m_RequestHeaders["Content-Length"] = ss.str();

	wstring strHeaders;
	for(HttpHeaders::iterator iter = m_RequestHeaders.begin();
		iter != m_RequestHeaders.end();
		iter++)
	{
		strHeaders += Convert_UTF8_UTF16(iter->first) + L": " + 
			Convert_UTF8_UTF16(iter->second) + L"\r\n";
	}

	if (!HttpSendRequest(m_hRequest, strHeaders.c_str(), strHeaders.size(), (LPVOID)m_Body.c_str(), m_Body.size()))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			printf("HttpOpenRequest failed. GetLastError()=%p", GetLastError());
			return -1;
		}
	}
	return 0;
}

DWORD AsyncHttpRequest::GetContext()
{
	return m_dwContext;
}

AsyncHttpResponse* AsyncHttpRequest::GetResponse()
{
	return m_spAsyncHttpResponse.get();
}
