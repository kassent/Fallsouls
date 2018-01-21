#include "Def.h"

namespace FallSouls
{
	TlsManager::TlsManager(uint32_t threadID)
	{
		uintptr_t moduleTls = *reinterpret_cast<uintptr_t*>(__readgsqword(0x58));
		uint32_t& pTlsID = *reinterpret_cast<uint32_t*>(moduleTls + 0x9C0);
		tlsID = pTlsID;
		pTlsID = threadID;
	}

	TlsManager::~TlsManager()
	{
		uintptr_t moduleTls = *reinterpret_cast<uintptr_t*>(__readgsqword(0x58));
		*reinterpret_cast<uint32_t*>(moduleTls + 0x9C0) = tlsID;
	}

	UIMessage::UIMessage() : name(""), type(0xD)
	{

	}

	UIMessage::~UIMessage() //write dtor for bsfixedstring
	{
		//this->dtor(true);
	}
}

namespace FallSouls
{
	RelocAddr <_PlayUISound>		PlayUISound(0x12BDD80); //48 89 5C 24 ? 57 48 83 EC 50 48 8B D9 E8 ? ? ? ? 48 85 C0

	RelocPtr <BSScaleformManager *> g_scaleformManager(0x5916490);//48 8B 0D ? ? ? ? 48 8B 49 18 E8 ? ? ? ? 48 8B 44 24 ?

	RelocPtr <UIMessageManager*>	g_uiMessageManager(0x5908B48); //48 8B 3D ? ? ? ? E8 ? ? ? ? 41 B8 ? ? ? ? 48 8B CF

	RelocPtr <UI*> g_ui(0x5908918); //48 8B 3D ? ? ? ? E8 ? ? ? ? 48 8B CF 48 8B D0 E8 ? ? ? ? 84 C0 0F 85 ? ? ? ?

	RelocAddr <_RegisterMenuOpenCloseEvent>	RegisterMenuOpenCloseEvent(0x5D1120);//call E8 ? ? ? ? 4C 8B AC 24 ? ? ? ? 4C 8B A4 24 ? ? ? ? 48 8B B4 24 ? ? ? ? 48 8D 5D D0

	RelocPtr <SimpleLock>		globalMenuStackLock(0x65AF4B0);

	RelocPtr <SimpleLock>		globalMenuTableLock(0x65AF4B8);

	RelocAddr <_HasHUDContext> HasHUDContext(0xA4F210); //48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 8B 42 10 4C 89 74 24 ?

	RelocPtr <TimerEventHandler*>	g_timerEventHandler(0x5A12280);//5A12280

	RelocPtr <GameVM *> g_gameVM(0x0590B388);

	RelocPtr <Main*>		g_main(0x5ADC2C8);

	RelocPtr <InputManager*> g_inputMgr(0x05A12260);

	RelocPtr <InputEventTable*> g_inputEventTable(0x05A97AB0);

	RelocPtr <PlayerControls*> g_playerControls(0x5A12268);

	RelocPtr <InputLayerManager *> g_inputLayerMgr(0x5908800);

	RelocPtr <MenuTopicManager *> g_menuTopicMgr(0x5906BB0); //5906BB0

	RelocPtr <JobListManager::ServingThread*>	g_servingThread(0x5AA3DD8);

	RelocPtr <ItemMenuDataManager *> g_itemMenuDataMgr(0x590CA00); //0x590CA00

	RelocPtr <PipboyDataManager *> g_pipboyDataMgr(0x5908B70);

	RelocPtr <PipboyManager *>	g_pipboyManager(0x5A11E20); //48 8B 05 ? ? ? ? 49 8B D9 F3 0F 10 80 ? ? ? ?

	UInt8 InputManager::AllowTextInput(bool allow)
	{
		if (allow)
		{
			if (allowTextInput == 0xFF)
				_WARNING("InputManager::AllowTextInput: counter overflow");
			else
				allowTextInput++;
		}
		else
		{
			if (allowTextInput == 0)
				_WARNING("InputManager::AllowTextInput: counter underflow");
			else
				allowTextInput--;
		}

		return allowTextInput;
	}

	UInt32 InputManager::GetMappedKey(BSFixedString name, UInt32 deviceType, UInt32 contextIdx)
	{
		ASSERT(contextIdx < kContextCount);

		tArray<InputContext::Mapping> * mappings;
		if (deviceType == InputEvent::kDeviceType_Mouse)
			mappings = &context[contextIdx]->mouseMap;
		else if (deviceType == InputEvent::kDeviceType_Gamepad)
			mappings = &context[contextIdx]->gamepadMap;
		else
			mappings = &context[contextIdx]->keyboardMap;

		for (UInt32 i = 0; i < mappings->count; i++)
		{
			InputContext::Mapping m;
			if (!mappings->GetNthItem(i, m))
				break;
			if (m.name == name)
				return m.buttonID;
		}

		// Unbound
		return 0xFF;
	}

	BSFixedString InputManager::GetMappedControl(UInt32 buttonID, UInt32 deviceType, UInt32 contextIdx)
	{
		ASSERT(contextIdx < kContextCount);

		// 0xFF == unbound
		if (buttonID == 0xFF)
			return BSFixedString();

		tArray<InputContext::Mapping> * mappings;
		if (deviceType == InputEvent::kDeviceType_Mouse)
			mappings = &context[contextIdx]->mouseMap;
		else if (deviceType == InputEvent::kDeviceType_Gamepad)
			mappings = &context[contextIdx]->gamepadMap;
		else
			mappings = &context[contextIdx]->keyboardMap;

		for (UInt32 i = 0; i < mappings->count; i++)
		{
			InputContext::Mapping m;
			if (!mappings->GetNthItem(i, m))
				break;
			if (m.buttonID == buttonID)
				return m.name;
		}

		return BSFixedString();
	}


	//void SWFToCodeFunctionHandler::Invoke(Args * args)
	//{
	//	switch (args->optionID)
	//	{
	//	case 0:
	//	{
	//		if (args->numArgs == 1 && args->args[0].IsString())
	//			PlayUISound(args->args[0].GetString());
	//		break;
	//	}
	//	default:
	//		break;
	//	}
	//}

	void SWFToCodeFunctionHandler::RegisterFunctions()
	{
		RegisterFunction("PlaySound", 0);
	}

	DelegateArgs::DelegateArgs(IMenu * menu, ScaleformArgs * args)
	{
		menuName = menu->menuName.c_str();
		optionID = args->optionID;
		for (UInt32 i = 0; i < args->numArgs; ++i)
		{
			parameters.push_back(args->args[i]);
		}
	}
	ScaleformArgs::ScaleformArgs(IMenu * menu, const DelegateArgs & delegateArgs) : result(nullptr), thisObj(nullptr), unk18(nullptr), optionID(delegateArgs.optionID)
	{
		//call this function within thread lock.
		BSFixedString menuName(delegateArgs.menuName.c_str());
		movie = (menu != nullptr) ? menu->movie : nullptr;
		this->numArgs = delegateArgs.parameters.size();
		this->args = (this->numArgs) ? &delegateArgs.parameters[0] : nullptr;
	}


	bool UI::IsMenuOpen(BSFixedString * menuName)
	{
		return CALL_MEMBER_FN(this, IsMenuOpen)(menuName);
	}

	IMenu * UI::GetMenu(BSFixedString * menuName)
	{
		if (!menuName || !menuName->data->Get<char>())
			return NULL;

		MenuTableItem * item = menuTable.Find(menuName);

		if (!item)
			return NULL;

		IMenu * menu = item->menuInstance;
		if (!menu)
			return NULL;

		return menu;
	}

	IMenu * UI::GetMenuByMovie(GFxMovieView * movie)
	{
		if (!movie)
			return NULL;

		IMenu * menu = NULL;
		menuTable.ForEach([movie, &menu](MenuTableItem * item)
		{
			IMenu * itemMenu = item->menuInstance;
			if (itemMenu) {
				GFxMovieView * view = itemMenu->movie;
				if (view) {
					if (movie == view) {
						menu = itemMenu;
						return false;
					}
				}
			}
			return true;
		});

		return menu;
	}

	HUDComponentBase::HUDComponentBase(GFxValue * parent, const char * componentName, HUDContextArray<BSFixedString> * contextList)
	{
		CALL_MEMBER_FN(this, Impl_ctor)(parent, componentName, contextList);
	}

	HUDComponentBase::~HUDComponentBase()
	{
		for (UInt32 i = 0; i < contexts.count; i++)
		{
			contexts.entries[i].Release();
		}

		Heap_Free(contexts.entries);
		contexts.count = 0;
	}

	void HUDComponentBase::UpdateVisibilityContext(void * unk1)
	{
		HasHUDContext(&contexts, unk1);
		bool bVisible = IsVisible();
		double alpha = 0.0;
		if (bVisible) {
			alpha = 100.0;
		}

		BSGFxDisplayObject::BSDisplayInfo dInfo;
		void * unk2 = GetExtDisplayInfo(&dInfo, this);
		SetExtDisplayInfoAlpha(unk2, alpha);
		SetExtDisplayInfo(&dInfo);

		unkEC = bVisible == false;
	}

	void HUDComponentBase::ColorizeComponent()
	{
		SetFilterColor(isWarning);
	}

}