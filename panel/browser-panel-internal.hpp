#pragma once

#include <QTimer>
#include <QPointer>
#include "browser-panel.hpp"
#include "cef-headers.hpp"

#include <vector>
#include <mutex>

struct PopupWhitelistInfo {
	std::string url;
	QPointer<QObject> obj;

	inline PopupWhitelistInfo(const std::string &url_, QObject *obj_)
		: url(url_), obj(obj_)
	{
	}
};

extern std::mutex popup_whitelist_mutex;
extern std::vector<PopupWhitelistInfo> popup_whitelist;
extern std::vector<PopupWhitelistInfo> forced_popups;

/* ------------------------------------------------------------------------- */

#if CHROME_VERSION_BUILD < 3770
class QCefRequestContextHandler : public CefRequestContextHandler {
	CefRefPtr<CefCookieManager> cm;

public:
	inline QCefRequestContextHandler(CefRefPtr<CefCookieManager> cm_)
		: cm(cm_)
	{
	}

	virtual CefRefPtr<CefCookieManager> GetCookieManager() override;

	IMPLEMENT_REFCOUNTING(QCefRequestContextHandler);
};
#endif

/* ------------------------------------------------------------------------- */

class QCefWidgetInternal : public QCefWidget {
	Q_OBJECT

public:
	QCefWidgetInternal(QWidget *parent, const std::string &url,
			   CefRefPtr<CefRequestContext> rqc);
	~QCefWidgetInternal();

	CefRefPtr<CefBrowser> cefBrowser;
	std::string url;
	std::string script;
	CefRefPtr<CefRequestContext> rqc;
	QTimer timer;
	bool allowAllPopups_ = false;

	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void showEvent(QShowEvent *event) override;
	virtual QPaintEngine *paintEngine() const override;

	virtual void setURL(const std::string &url) override;
	virtual void setStartupScript(const std::string &script) override;
	virtual void allowAllPopups(bool allow) override;
	virtual void closeBrowser() override;

	void Resize();

protected:
	virtual void focusInEvent(QFocusEvent *event) override
	{
		QWidget::focusInEvent(event);
		blog(LOG_WARNING, "QCefWidgetInternal focused.");
		if (cefBrowser)
			cefBrowser->GetHost()->SetFocus(true);
	}

	virtual void focusOutEvent(QFocusEvent *event) override
	{
		QWidget::focusOutEvent(event);
		blog(LOG_WARNING, "QCefWidgetInternal unfocused.");
		// TODO This doesn't trigger OnTakeFocus or anything
		// https://magpcss.org/ceforum/viewtopic.php?f=6&t=16458&p=40440#p40443
		if (cefBrowser)
			cefBrowser->GetHost()->SetFocus(false);
	}

public slots:
	void Init();
};
