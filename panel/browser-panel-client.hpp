#pragma once

#include "cef-headers.hpp"
#include "browser-panel-internal.hpp"

#include <string>

class QCefBrowserClient : public CefClient,
			  public CefDisplayHandler,
#if CHROME_VERSION_BUILD >= 6533
			  public CefCommandHandler,
			  public CefPermissionHandler,
#endif
			  public CefRequestHandler,
			  public CefLifeSpanHandler,
			  public CefContextMenuHandler,
			  public CefLoadHandler,
			  public CefKeyboardHandler,
			  public CefFocusHandler,
			  public CefJSDialogHandler {

public:
	inline QCefBrowserClient(QCefWidgetInternal *widget_, const std::string &script_, bool allowAllPopups_)
		: widget(widget_),
		  script(script_),
		  allowAllPopups(allowAllPopups_)
	{
	}

	/* CefClient */
	CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; };
	CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; };
#if CHROME_VERSION_BUILD >= 6533
	CefRefPtr<CefCommandHandler> GetCommandHandler() override { return this; };
	CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return this; };
#endif
	CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; };
	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; };
	CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; };
	CefRefPtr<CefFocusHandler> GetFocusHandler() override { return this; };
	CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; };
	CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() override { return this; };

	/* CefCommandHandler */
#if CHROME_VERSION_BUILD >= 6533
	bool OnChromeCommand(CefRefPtr<CefBrowser> browser, int command_id,
			     cef_window_open_disposition_t disposition) override;

	/* CefPermissionHandler */
	bool OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser, uint64_t prompt_id,
				    const CefString &requesting_origin, uint32_t requested_permissions,
				    CefRefPtr<CefPermissionPromptCallback> callback) override;

#endif

	/* CefDisplayHandler */
	void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) override;

	/* CefRequestHandler */
	bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
			    bool user_gesture, bool is_redirect) override;

	void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode,
			 const CefString &errorText, const CefString &failedUrl) override;

	bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url,
			      CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture) override;

	/* CefLifeSpanHandler */
#ifndef __APPLE__
	void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
#endif
	bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
#if CHROME_VERSION_BUILD >= 6834
			   int popup_id,
#endif
			   const CefString &target_url, const CefString &target_frame_name,
			   CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture,
			   const CefPopupFeatures &popupFeatures, CefWindowInfo &windowInfo,
			   CefRefPtr<CefClient> &client, CefBrowserSettings &settings,
			   CefRefPtr<CefDictionaryValue> &extra_info, bool *no_javascript_access) override;

	void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

	/* CefFocusHandler */
	bool OnSetFocus(CefRefPtr<CefBrowser> browser, CefFocusHandler::FocusSource source) override;

	/* CefContextMenuHandler */
	void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
				 CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;

#if defined(_WIN32)
	bool RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
			    CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model,
			    CefRefPtr<CefRunContextMenuCallback> callback) override;
#endif

	bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
				  CefRefPtr<CefContextMenuParams> params, int command_id,
				  CefContextMenuHandler::EventFlags event_flags) override;

	/* CefLoadHandler */
	void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
			 TransitionType transition_type) override;

	void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;

	/* CefKeyboardHandler */
	bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event,
			   bool *is_keyboard_shortcut) override;

	/* CefJSDialogHandler */
	bool OnJSDialog(CefRefPtr<CefBrowser> browser, const CefString &origin_url,
			CefJSDialogHandler::JSDialogType dialog_type, const CefString &message_text,
			const CefString &default_prompt_text, CefRefPtr<CefJSDialogCallback> callback,
			bool &suppress_message) override;

	QCefWidgetInternal *widget = nullptr;
	std::string script;
	bool allowAllPopups;

	IMPLEMENT_REFCOUNTING(QCefBrowserClient);
};
