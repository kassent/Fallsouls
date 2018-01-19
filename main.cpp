#include "F4SE_common/F4SE_version.h"
#include "F4SE_common/BranchTrampoline.h"

#include "F4SE/PluginAPI.h"

#include <shlobj.h>
#include <memory>
#include <string>

#include "Hooks.h"

#define PLUGIN_VERSION	MAKE_EXE_VERSION(1, 0, 2)
#define PLUGIN_NAME		"FallSouls"

IDebugLog						gLog;
PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
F4SEScaleformInterface			* g_scaleform = nullptr;


extern "C"
{
	bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
	{
		std::unique_ptr<char[]> sPath(new char[MAX_PATH]);
		sprintf_s(sPath.get(), MAX_PATH, "%s%s.log", "\\My Games\\Fallout4\\F4SE\\", PLUGIN_NAME);
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, sPath.get());

		_MESSAGE("%s: %08X", PLUGIN_NAME, PLUGIN_VERSION);

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = PLUGIN_NAME;
		info->version = PLUGIN_VERSION;

		g_pluginHandle = f4se->GetPluginHandle();

		if (f4se->runtimeVersion < RUNTIME_VERSION_1_10_50)
		{
			MessageBox(nullptr, "UNSUPPORTED GAME VERSION. REQUIRED VERSION IS: V1.10.50", PLUGIN_NAME, MB_OK);
			return false;
		}

		if (f4se->isEditor)
		{
			_FATALERROR("loaded in editor, marking as incompatible");
			return false;
		}

		g_scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
		if (!g_scaleform)
		{
			_FATALERROR("couldn't get scaleform interface");
			return false;
		}

		return true;
	}

	bool F4SEPlugin_Load(const F4SEInterface * f4se)
	{
		if (!g_branchTrampoline.Create(1024 * 64))
		{
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return false;
		}

		if (!g_localTrampoline.Create(1024 * 64, nullptr))
		{
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
			return false;
		}

		if (g_scaleform)
			g_scaleform->Register("FallSouls", FallSouls::ScaleformCallback);

		FallSouls::LoadSettings();

		FallSouls::InstallHooks();

		return true;
	}
};