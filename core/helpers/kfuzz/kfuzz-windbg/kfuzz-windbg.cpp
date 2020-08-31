/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier
*/

#include "stdafx.h"
#include "ExtensionProvider.h"

Debugger::DataModel::Libraries::ExtensionProvider *g_pProvider = nullptr;
IDataModelManager *g_pManager = nullptr;
IDebugHost *g_pHost = nullptr;

namespace Debugger::DataModel::ClientEx
{
	IDataModelManager *GetManager() { return g_pManager; }
	IDebugHost *GetHost() { return g_pHost; }
}

// DebugExtensionInitialize:
//
// Called to initialize the debugger extension.  For a data model extension, this acquires
// the necessary data model interfaces from the debugger and instantiates singleton instances
// of any of the extension classes which provide the functionality of the debugger extension.
//
extern "C"
HRESULT CALLBACK DebugExtensionInitialize(PULONG /*pVersion*/, PULONG /*pFlags*/)
{
	HRESULT hr = S_OK;

	try
	{
		Microsoft::WRL::ComPtr<IDebugClient> spClient;
		Microsoft::WRL::ComPtr<IHostDataModelAccess> spAccess;

		//
		// Create a client interface to the debugger and ask for the data model interfaces.  The client
		// library requires an implementation of Debugger::DataModel::ClientEx::(GetManager and ::GetHost)
		// which return these interfaces when called.
		//
		hr = DebugCreate(__uuidof(IDebugClient), (void **)&spClient);
		if (SUCCEEDED(hr))
		{
			hr = spClient.As(&spAccess);
		}

		if (SUCCEEDED(hr))
		{
			hr = spAccess->GetDataModel(&g_pManager, &g_pHost);
		}

		if (SUCCEEDED(hr))
		{
			//
			// Create the provider class which itself is a singleton and holds singleton instances of
			// all extension classes.
			//
			g_pProvider = new Debugger::DataModel::Libraries::ExtensionProvider();
		}
	}
	catch (...)
	{
		return E_FAIL;
	}

	if (FAILED(hr))
	{
		if (g_pManager != nullptr)
		{
			g_pManager->Release();
			g_pManager = nullptr;
		}

		if (g_pHost != nullptr)
		{
			g_pHost->Release();
			g_pHost = nullptr;
		}
	}

	return hr;
}

// DebugExtensionCanUnload:
//
// Called after DebugExtensionUninitialize to determine whether the debugger extension can
// be unloaded.  A return of S_OK indicates that it can.  A failure (or return of S_FALSE) indicates
// that it cannot.
//
// Extension libraries are responsible for ensuring that there are no live interfaces back into the
// extension before unloading!
//
extern "C"
HRESULT CALLBACK DebugExtensionCanUnload(void)
{
	auto objCount = Microsoft::WRL::Module<InProc>::GetModule().GetObjectCount();
	return (objCount == 0) ? S_OK : S_FALSE;
}

// DebugExtensionUninitialize:
//
// Called before unloading (and before DebugExtensionCanUnload) to prepare the debugger extension for
// unloading.  Any manipulations done during DebugExtensionInitialize should be undone and any interfaces
// released.
//
// Deleting the singleton instances of extension classes should unlink them from the data model.  There still
// may be references into the extension alive from scripts, other debugger extensions, debugger variables,
// etc...  The extension cannot return S_OK from DebugExtensionCanUnload until there are no such live
// references.
//
// If DebugExtensionCanUnload returns a "do not unload" indication, it is possible that DebugExtensionInitialize
// will be called without an interveining unload.
//
extern "C"
void CALLBACK DebugExtensionUninitialize()
{
	if (g_pProvider != nullptr)
	{
		delete g_pProvider;
		g_pProvider = nullptr;
	}

	if (g_pManager != nullptr)
	{
		g_pManager->Release();
		g_pManager = nullptr;
	}

	if (g_pHost != nullptr)
	{
		g_pHost->Release();
		g_pHost = nullptr;
	}
}

// DebugExtensionUnload:
//
// A final callback immediately before the DLL is unloaded.  This will only happen after a successful
// DebugExtensionCanUnload.
//
extern "C"
void CALLBACK DebugExtensionUnload()
{
}
