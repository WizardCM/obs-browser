#include "browser-panel-client.hpp"

#include <util/dstr.h>

#include <QUrl>
#include <QDesktopServices>

#ifdef _WIN32
#include <windows.h>
#endif

/* CefClient */
CefRefPtr<CefLoadHandler> QCefBrowserClient::GetLoadHandler()
{
	return this;
}

CefRefPtr<CefDisplayHandler> QCefBrowserClient::GetDisplayHandler()
{
	return this;
}

CefRefPtr<CefRequestHandler> QCefBrowserClient::GetRequestHandler()
{
	return this;
}

CefRefPtr<CefLifeSpanHandler> QCefBrowserClient::GetLifeSpanHandler()
{
	return this;
}


/* CefDisplayHandler */
void QCefBrowserClient::OnTitleChange(
		CefRefPtr<CefBrowser> browser,
		const CefString &title)
{
	if (widget && widget->cefBrowser->IsSame(browser)) {
		std::string str_title = title;
		QString qt_title = QString::fromUtf8(str_title.c_str());
		QMetaObject::invokeMethod(widget, "titleChanged",
				Q_ARG(QString, qt_title));
	} else { /* handle popup title */
#ifdef _WIN32
		std::wstring str_title = title;
		HWND hwnd = browser->GetHost()->GetWindowHandle();
		SetWindowTextW(hwnd, str_title.c_str());
#endif
	}
}

/* CefRequestHandler */
bool QCefBrowserClient::OnBeforeBrowse(
		CefRefPtr<CefBrowser>,
		CefRefPtr<CefFrame>,
		CefRefPtr<CefRequest> request,
		bool,
		bool)
{
	if (widget) {
		std::string str_url = request->GetURL();
		QString qt_url = QString::fromUtf8(str_url.c_str());
		QMetaObject::invokeMethod(widget, "urlChanged",
				Q_ARG(QString, qt_url));
	}
	return false;
}

/* CefLifeSpanHandler */
bool QCefBrowserClient::OnBeforePopup(
		CefRefPtr<CefBrowser>,
		CefRefPtr<CefFrame>,
		const CefString &target_url,
		const CefString &,
		CefLifeSpanHandler::WindowOpenDisposition,
		bool,
		const CefPopupFeatures &,
		CefWindowInfo &windowInfo,
		CefRefPtr<CefClient> &,
		CefBrowserSettings &,
		bool *)
{
	std::string str_url = target_url;

	std::lock_guard<std::mutex> lock(popup_whitelist_mutex);
	for (size_t i = popup_whitelist.size(); i > 0; i--) {
		PopupWhitelistInfo &info = popup_whitelist[i - 1];

		if (!info.obj) {
			popup_whitelist.erase(popup_whitelist.begin() + (i - 1));
			continue;
		}

		if (astrcmpi(info.url.c_str(), str_url.c_str()) == 0) {
#ifdef _WIN32
			HWND hwnd = (HWND)widget->effectiveWinId();
			windowInfo.parent_window = hwnd;
#endif
			return false;
		}
	}

	/* Open popup URLs in user's actual browser */
	QUrl url = QUrl(str_url.c_str(), QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
	return true;
}

void QCefBrowserClient::OnLoadEnd(
		CefRefPtr<CefBrowser>,
		CefRefPtr<CefFrame> frame,
		int)
{
	if (frame->IsMain() && !script.empty())
		frame->ExecuteJavaScript(script, CefString(), 0);
}
