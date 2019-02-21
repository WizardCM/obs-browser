/******************************************************************************
 Copyright (C) 2014 by John R. Bradley <jrb@turrettech.com>
 Copyright (C) 2018 by Hugh Bailey ("Jim") <jim@obsproject.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "browser-app.hpp"
#include "browser-version.h"
#include <json11/json11.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace json11;

CefRefPtr<CefRenderProcessHandler> BrowserApp::GetRenderProcessHandler()
{
	return this;
}

CefRefPtr<CefBrowserProcessHandler> BrowserApp::GetBrowserProcessHandler()
{
	return this;
}

void BrowserApp::OnRegisterCustomSchemes(
		CefRawPtr<CefSchemeRegistrar> registrar)
{
#if CHROME_VERSION_BUILD >= 3029
	registrar->AddCustomScheme("http", true, false, false, false, true,
			false);
#else
	registrar->AddCustomScheme("http", true, false, false, false, true);
#endif
}

void BrowserApp::OnBeforeChildProcessLaunch(
		CefRefPtr<CefCommandLine> command_line)
{
#ifdef _WIN32
	std::string pid = std::to_string(GetCurrentProcessId());
	command_line->AppendSwitchWithValue("parent_pid", pid);
#else
	(void)command_line;
#endif
}

void BrowserApp::OnBeforeCommandLineProcessing(
		const CefString &,
		CefRefPtr<CefCommandLine> command_line)
{
	if (!shared_texture_available) {
		bool enableGPU = command_line->HasSwitch("enable-gpu");
		CefString type = command_line->GetSwitchValue("type");

		if (!enableGPU && type.empty()) {
			command_line->AppendSwitch("disable-gpu");
			command_line->AppendSwitch("disable-gpu-compositing");
		}
	}

	command_line->AppendSwitch("enable-system-flash");

	command_line->AppendSwitchWithValue("autoplay-policy",
			"no-user-gesture-required");
}

void BrowserApp::OnContextCreated(CefRefPtr<CefBrowser>,
		CefRefPtr<CefFrame>,
		CefRefPtr<CefV8Context> context)
{
	CefRefPtr<CefV8Value> globalObj = context->GetGlobal();

	CefRefPtr<CefV8Value> obsStudioObj = CefV8Value::CreateObject(0, 0);
	globalObj->SetValue("obsstudio",
			obsStudioObj, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> pluginVersion =
		CefV8Value::CreateString(OBS_BROWSER_VERSION_STRING);
	obsStudioObj->SetValue("pluginVersion",
			pluginVersion, V8_PROPERTY_ATTRIBUTE_NONE);

	// TODO: Also return whether we're currently a source or a panel

	CefRefPtr<CefV8Value> func =
		CefV8Value::CreateFunction("getCurrentScene", this);
	obsStudioObj->SetValue("getCurrentScene",
			func, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> getStatus =
		CefV8Value::CreateFunction("getStatus", this);
	obsStudioObj->SetValue("getStatus",
			getStatus, V8_PROPERTY_ATTRIBUTE_NONE);

	// TODO: Check HERE if user has enabled the ability to control OBS
	// TODO: Do not allow any of this if we're in a browser panel

	CefRefPtr<CefV8Value> startStreaming =
			 CefV8Value::CreateFunction("startStreaming", this);
	obsStudioObj->SetValue("startStreaming",
			startStreaming, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> stopStreaming =
		CefV8Value::CreateFunction("stopStreaming", this);
	obsStudioObj->SetValue("stopStreaming",
			stopStreaming, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> startRecording =
		CefV8Value::CreateFunction("startRecording", this);
	obsStudioObj->SetValue("startRecording",
			startRecording, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> stopRecording =
		CefV8Value::CreateFunction("stopRecording", this);
	obsStudioObj->SetValue("stopRecording",
			stopRecording, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> startReplaybuffer =
		CefV8Value::CreateFunction("startReplaybuffer", this);
	obsStudioObj->SetValue("startReplaybuffer",
			startReplaybuffer, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> stopReplaybuffer =
		CefV8Value::CreateFunction("stopReplaybuffer", this);
	obsStudioObj->SetValue("stopReplaybuffer",
			stopReplaybuffer, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> getScenes =
		CefV8Value::CreateFunction("getScenes", this);
	obsStudioObj->SetValue("getScenes",
			getScenes, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> getTransitions =
		CefV8Value::CreateFunction("getTransitions", this);
	obsStudioObj->SetValue("getTransitions",
			getTransitions, V8_PROPERTY_ATTRIBUTE_NONE);

	CefRefPtr<CefV8Value> setCurrentScene =
		CefV8Value::CreateFunction("setCurrentScene", this);
	obsStudioObj->SetValue("setCurrentScene",
			setCurrentScene, V8_PROPERTY_ATTRIBUTE_NONE);

}

void BrowserApp::ExecuteJSFunction(CefRefPtr<CefBrowser> browser,
		const char *functionName,
		CefV8ValueList arguments)
{
	CefRefPtr<CefV8Context> context =
		browser->GetMainFrame()->GetV8Context();

	context->Enter();

	CefRefPtr<CefV8Value> globalObj = context->GetGlobal();
	CefRefPtr<CefV8Value> obsStudioObj = globalObj->GetValue("obsstudio");
	CefRefPtr<CefV8Value> jsFunction = obsStudioObj->GetValue(functionName);

	if (jsFunction && jsFunction->IsFunction())
		jsFunction->ExecuteFunction(NULL, arguments);

	context->Exit();
}

bool BrowserApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
		CefProcessId source_process,
		CefRefPtr<CefProcessMessage> message)
{
	DCHECK(source_process == PID_BROWSER);

	CefRefPtr<CefListValue> args = message->GetArgumentList();

	if (message->GetName() == "Visibility") {
		CefV8ValueList arguments;
		arguments.push_back(CefV8Value::CreateBool(args->GetBool(0)));

		ExecuteJSFunction(browser, "onVisibilityChange", arguments);

	} else if (message->GetName() == "Active") {
		CefV8ValueList arguments;
		arguments.push_back(CefV8Value::CreateBool(args->GetBool(0)));

		ExecuteJSFunction(browser, "onActiveChange", arguments);

	} else if (message->GetName() == "DispatchJSEvent") {
		CefRefPtr<CefV8Context> context =
			browser->GetMainFrame()->GetV8Context();

		context->Enter();

		CefRefPtr<CefV8Value> globalObj = context->GetGlobal();

		std::string err;
		auto payloadJson = Json::parse(args->GetString(1).ToString(), err);

		Json::object wrapperJson;
		if (args->GetSize() > 1)
			wrapperJson["detail"] = payloadJson;
		std::string wrapperJsonString = Json(wrapperJson).dump();
		std::string script;

		script += "new CustomEvent('";
		script += args->GetString(0).ToString();
		script += "', ";
		script += wrapperJsonString;
		script += ");";

		CefRefPtr<CefV8Value> returnValue;
		CefRefPtr<CefV8Exception> exception;

		/* Create the CustomEvent object
		 * We have to use eval to invoke the new operator */
		context->Eval(script, browser->GetMainFrame()->GetURL(),
				0, returnValue, exception);

		CefV8ValueList arguments;
		arguments.push_back(returnValue);

		CefRefPtr<CefV8Value> dispatchEvent =
			globalObj->GetValue("dispatchEvent");
		dispatchEvent->ExecuteFunction(NULL, arguments);

		context->Exit();

	} else if (message->GetName() == "executeCallback") {
		CefRefPtr<CefV8Context> context =
			browser->GetMainFrame()->GetV8Context();
		CefRefPtr<CefV8Value> retval;
		CefRefPtr<CefV8Exception> exception;

		context->Enter();

		CefRefPtr<CefListValue> arguments = message->GetArgumentList();
		int callbackID = arguments->GetInt(0);
		CefString jsonString = arguments->GetString(1);

		std::string script;
		script += "JSON.parse('";
		script += arguments->GetString(1).ToString();
		script += "');";

		CefRefPtr<CefV8Value> callback = callbackMap[callbackID];
		CefV8ValueList args;

		context->Eval(script, browser->GetMainFrame()->GetURL(),
				0, retval, exception);

		args.push_back(retval);

		if(callback)
			callback->ExecuteFunction(NULL, args);

		context->Exit();

		callbackMap.erase(callbackID);

	} else {
		return false;
	}

	return true;
}

bool BrowserApp::Execute(const CefString &name,
		CefRefPtr<CefV8Value>,
		const CefV8ValueList &arguments,
		CefRefPtr<CefV8Value> &retVal,
		CefString &)
{
	if (name == "getCurrentScene") {
		if (arguments.size() == 1 && arguments[0]->IsFunction()) {
			callbackId++;
			callbackMap[callbackId] = arguments[0];
		}

		CefRefPtr<CefProcessMessage> msg =
			CefProcessMessage::Create("getCurrentScene");
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetInt(0, callbackId);

		CefRefPtr<CefBrowser> browser =
			CefV8Context::GetCurrentContext()->GetBrowser();
		browser->SendProcessMessage(PID_BROWSER, msg);

	}
	else if (name == "getStatus") {
		if (arguments.size() == 1 && arguments[0]->IsFunction()) {
			callbackId++;
			callbackMap[callbackId] = arguments[0];
		}

		CefRefPtr<CefProcessMessage> msg =
			CefProcessMessage::Create("getStatus");
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetInt(0, callbackId);

		CefRefPtr<CefBrowser> browser =
			CefV8Context::GetCurrentContext()->GetBrowser();
		browser->SendProcessMessage(PID_BROWSER, msg);

	} else if (name == "startStreaming" || name == "stopStreaming" || name == "startRecording" || name == "stopRecording" || name == "startReplaybuffer" || name == "stopReplaybuffer") {
		// TODO Find out from Jim if we can skip SendProcessMessage and just run OBS functions directly in here (if yes, just copy all the if else statements)
		CefRefPtr<CefProcessMessage> msg =
			CefProcessMessage::Create(name);

		CefRefPtr<CefBrowser> browser =
			CefV8Context::GetCurrentContext()->GetBrowser();
		browser->SendProcessMessage(PID_BROWSER, msg);
	} else if (name == "getScenes" || name == "getTransitions") {
		// TODO This is duplicate code of getStatus
		if (arguments.size() == 1 && arguments[0]->IsFunction()) {
			callbackId++;
			callbackMap[callbackId] = arguments[0];
		}

		CefRefPtr<CefProcessMessage> msg =
			CefProcessMessage::Create(name);
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetInt(0, callbackId);

		CefRefPtr<CefBrowser> browser =
			CefV8Context::GetCurrentContext()->GetBrowser();
		browser->SendProcessMessage(PID_BROWSER, msg);

	} else if (name == "setCurrentScene") {
		if (!(arguments.size() > 0 && arguments[0]->IsString()))
			return false;
			
		CefRefPtr<CefProcessMessage> msg =
			CefProcessMessage::Create("setCurrentScene");
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetString(0, arguments[0]->GetStringValue());

		CefRefPtr<CefBrowser> browser =
			CefV8Context::GetCurrentContext()->GetBrowser();
		browser->SendProcessMessage(PID_BROWSER, msg);
		/* TODO: Figure out how to send a return value (which doesn't crash32)
		CefRefPtr<CefV8Value> val = CefV8Value::CreateBool(true);
		retVal->SetValue(0, val, V8_PROPERTY_ATTRIBUTE_READONLY); */
	} else {
		/* Function does not exist. */
		return false;
	}

	return true;
}
