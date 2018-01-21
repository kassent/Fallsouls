#include "common/ICriticalSection.h"

#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/SafeWrite.h"

#include "f4se/ScaleformCallbacks.h"
#include "f4se/xbyak/xbyak.h"

#include "Def.h"
#include "Hooks.h"
#include "HookUtil.h"

#include <queue>
#include <algorithm>

//#define DEBUG 1

namespace FallSouls
{
	enum
	{
		kType_Console,
		kType_Terminal,
		kType_ExamineConfirm,
		kType_LevelUp,
		kType_Pause,
		kType_Barter,
		kType_MessageBox,
		kType_PipboyHolotape,
		kType_Book,
		kType_Container,
		kType_SPECIAL,
		kType_Examine,
		kType_Lockpicking,
		kType_SleepWaitMenu,
		kType_Pipboy,
		kType_TerminalHolotape,
		kType_Caravan,
		kType_NumMenus = 0x11
	};

	struct MenuSetting
	{
		std::string		menuName;
		bool			isEnabled;
	};

	struct CoreController
	{
		static UInt32								globalControlCounter; // thread-safe...
		static std::vector<MenuSetting>				globalMenuSettings;
	};

	UInt32						CoreController::globalControlCounter = 0;
	std::vector<MenuSetting>	CoreController::globalMenuSettings = 
	{
		{ "Console",					true },
		{ "TerminalMenu",				true },
		{ "ExamineConfirmMenu",			true },
		{ "LevelUpMenu",				true },
		{ "PauseMenu",					true },
		{ "BarterMenu",					true },
		{ "MessageBoxMenu",				true },
		{ "PipboyHolotapeMenu",			true },
		{ "BookMenu",					true },
		{ "ContainerMenu",				true },
		{ "SPECIALMenu",				true },
		{ "ExamineMenu",				true },
		{ "LockpickingMenu",			true },
		{ "SleepWaitMenu",				true },
		{ "PipboyMenu",					true },
		{ "TerminalHolotapeMenu",		true },
		{ "Workshop_CaravanMenu",		true }
	};

	void LoadSettings()
	{
		constexpr char * configFile = ".\\Data\\MCM\\Settings\\FallSouls.ini";
		constexpr char * settingsSection = "Settings";
		for (auto & menuSetting : CoreController::globalMenuSettings)
		{
			std::string settingName = "b" + menuSetting.menuName;
			menuSetting.isEnabled = GetPrivateProfileIntA(settingsSection, settingName.c_str(), 1, configFile) != 0;
		}
	}

	class FallSouls_OnModSettingChanged : public ::GFxFunctionHandler
	{
	public:
		virtual void Invoke(Args * args) override
		{
			LoadSettings();
			static BSFixedString console("Console");
			static BSFixedString levelUpMenu("LevelUpMenu");
			auto & pHashSet = (*g_ui)->menuTable;
			globalMenuTableLock->LockThis();
			auto * pItem = pHashSet.Find(&console);
			if (pItem && pItem->menuInstance && !(*g_ui)->IsMenuOpen(&console))
			{
				if (CoreController::globalMenuSettings[kType_Console].isEnabled)
				{
					pItem->menuInstance->flags &= ~IMenu::kFlag_PauseGame;
					pItem->menuInstance->flags |= IMenu::kFlag_FallSoulsMenu;
				}
				else
				{
					pItem->menuInstance->flags |= IMenu::kFlag_PauseGame;
					pItem->menuInstance->flags &= ~IMenu::kFlag_FallSoulsMenu;
				}
			}
			pItem = pHashSet.Find(&levelUpMenu);
			if (pItem && pItem->menuInstance && !(*g_ui)->IsMenuOpen(&levelUpMenu))
			{
				if (CoreController::globalMenuSettings[kType_LevelUp].isEnabled)
				{
					pItem->menuInstance->flags &= ~IMenu::kFlag_PauseGame;
					pItem->menuInstance->flags |= IMenu::kFlag_FallSoulsMenu;
				}
				else
				{
					pItem->menuInstance->flags |= IMenu::kFlag_PauseGame;
					pItem->menuInstance->flags &= ~IMenu::kFlag_FallSoulsMenu;
				}
			}
			globalMenuTableLock->ReleaseThis();
		}
	};

	bool ScaleformCallback(GFxMovieView * view, GFxValue * value)
	{
		RegisterFunction<FallSouls_OnModSettingChanged>(value, view->movieRoot, "onModSettingChanged");
		return true;
	}
}


namespace FallSouls
{
	template <UInt32 INDEX>
	class Callback
	{
	public:
		typedef IMenu * (*CallbackType)(void);

		Callback() : m_callback(nullptr) {}

		bool ReplaceFunctor()
		{
			bool result = false;
			if ((*g_ui) != nullptr)
			{
				auto & pHashSet = (*g_ui)->menuTable;
				const char * menuName = nullptr;
				if (INDEX < CoreController::globalMenuSettings.size())
					menuName = CoreController::globalMenuSettings[INDEX].menuName.c_str();
				globalMenuTableLock->LockThis();
				auto * pItem = pHashSet.Find(&BSFixedString(menuName));
				if (pItem != nullptr)
				{
					m_callback = pItem->menuConstructor;
					pItem->menuConstructor = CreateGameMenuInstance;
					if (INDEX == kType_Console)
					{
						static BSFixedString console("Console");
						if (pItem->menuInstance && !(*g_ui)->IsMenuOpen(&console))
						{
							if (CoreController::globalMenuSettings[kType_Console].isEnabled)
							{
								pItem->menuInstance->flags &= ~IMenu::kFlag_PauseGame;
								pItem->menuInstance->flags |= IMenu::kFlag_FallSoulsMenu;
							}
							else
							{
								pItem->menuInstance->flags |= IMenu::kFlag_PauseGame;
								pItem->menuInstance->flags &= ~IMenu::kFlag_FallSoulsMenu;
							}
						}
					}
					_MESSAGE("Callback<%s>::Register()", menuName);
					result = true;
				}
				globalMenuTableLock->ReleaseThis();
			}
			return result;
		}

		static Callback * GetSingleton()
		{
			static Callback singleton;
			return &singleton;
		}

		static void	Register()
		{
			GetSingleton()->ReplaceFunctor();
		}

		static IMenu * CreateGameMenuInstance()
		{
			auto callback = GetSingleton()->m_callback;
			IMenu* pResult = nullptr;
			if (callback != nullptr && (pResult = callback(), pResult)\
				&& CoreController::globalMenuSettings[INDEX].isEnabled)
			{
				pResult->flags &= ~IMenu::kFlag_BlurBackground;
				pResult->flags &= ~IMenu::kFlag_ShaderWorld;
				pResult->flags &= ~IMenu::kFlag_PauseGame;
				//pResult->flags &= ~IMenu::kFlag_Unk200000;
				pResult->flags |= IMenu::kFlag_FallSoulsMenu;
			}
			return pResult;
		}

		CallbackType				m_callback;
	};
}

namespace FallSouls
{
	RelocAddr <bool *>	isGameLoading(0x5908933);

	using _SetThreadEvent = void(*)(UInt32 eventID);
	RelocAddr<_SetThreadEvent>	SetThreadEvent(0x1B62BD0);

	using _ProcessUserInterface = void(*)();
	RelocAddr<_ProcessUserInterface>	ProcessUserInterface(0xCBCD40); // need to hook 2 locations.

	using _IsGameRunning = bool(*)(UInt32);
	RelocAddr<_IsGameRunning>	IsGameRunning(0xCBC8D0); //48 8B 05 ? ? ? ? 48 85 C0 74 12 85 C9

	using _RegisterThreadTask = void(*)(void *, void *, void *, const char *);
	RelocAddr<_RegisterThreadTask>	RegisterThreadTask(0x1B4FE30);

	using _IncreaseCullingProcessRef = void(*)(void * pObj);
	RelocAddr<_IncreaseCullingProcessRef>	IncreaseCullingProcessRef(0x1B14180);

	using  _MessageQueueProcessTask = bool(*)(void * messageQueue, float timeout, UInt32 unk1);
	RelocAddr <_MessageQueueProcessTask> MessageQueueProcessTask(0x00D57A00);

	ICriticalSection		s_taskQueueLock;
	std::queue<ITaskDelegate*>	s_tasks;


	class PlayerControlsEx : public PlayerControls
	{
	public:
		using FnProcessUserEvent = void(__thiscall PlayerControlsEx::*)(InputEvent * inputEvent);
		static FnProcessUserEvent	ProcessUserEvent_Original;
		static bool			s_disableUserEvent;
		static SimpleLock	s_controlThreadLock;

		bool CanProcessUerEvent_Hook()
		{
			return (s_disableUserEvent) ? false : this->CanProcessUerEvent();
		}

		void ProcessUserEvent_Hook(InputEvent * inputEvent)
		{
			auto * input = (s_disableUserEvent) ? nullptr : inputEvent;
			return (this->*ProcessUserEvent_Original)(input);
		}

		static void DisableUserEvent(bool disabled)
		{
			SimpleLocker locker(&s_controlThreadLock);
			s_disableUserEvent = disabled;
		}

		static void ProcessPlayerCameraInput_Hook(InputEventTable * pTable, void * receiver)
		{
			using _ProcessPlayerCameraInput = void(*)(InputEventTable *, void *);
			if (s_disableUserEvent)
			{
				if ((*g_playerCamera) && (*g_inputEventTable))
				{
					auto * pLock = &(*g_inputEventTable)->inputIDLock;
					pLock->LockThis();
					(*g_playerCamera)->unk40 = (*g_inputEventTable)->inputID;
					pLock->ReleaseThis();
				}
				return;
			}
			static RelocAddr<_ProcessPlayerCameraInput> ProcessPlayerCameraInput(0x1B2CD60);
			ProcessPlayerCameraInput(pTable, receiver);
		}

		static void InitHooks()
		{
			ProcessUserEvent_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D721D0), &ProcessUserEvent_Hook);
			//g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0x0F43E51), GetFnAddr(&CanProcessUerEvent_Hook));
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0x0F43EF6), (uintptr_t)ProcessPlayerCameraInput_Hook);
		}
	};
	bool									PlayerControlsEx::s_disableUserEvent = false;
	SimpleLock								PlayerControlsEx::s_controlThreadLock;
	PlayerControlsEx::FnProcessUserEvent	PlayerControlsEx::ProcessUserEvent_Original = nullptr;


	class PlayerCharacterEx : public PlayerCharacter
	{
	public:
		using FnCanPlayEquipmentAnimation = bool(__thiscall  PlayerCharacterEx::*)();
		static FnCanPlayEquipmentAnimation CanPlayEquipmentAnimation_Original;

		bool CanPlayEquipmentAnimation_Hook()
		{
			static BSFixedString pipboyMenu("PipboyMenu");
			if ((*g_ui) && (*g_ui)->IsMenuOpen(&pipboyMenu))
				return false;
			return (this->*CanPlayEquipmentAnimation_Original)();
		}

		static void InitHooks()
		{
			CanPlayEquipmentAnimation_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D6C8C8) + 0xD5 * 0x8, &CanPlayEquipmentAnimation_Hook);
		}
	};
	PlayerCharacterEx::FnCanPlayEquipmentAnimation	PlayerCharacterEx::CanPlayEquipmentAnimation_Original = nullptr;



	class GameplayControlHandler : public BSTEventSink<MenuOpenCloseEvent>
	{
	public:
		virtual ~GameplayControlHandler() { };
		virtual	EventResult	ReceiveEvent(MenuOpenCloseEvent * evn, void * dispatcher) override
		{
			auto functor = [evn](const MenuSetting & setting)->bool {return (setting.menuName == evn->menuName.c_str()) ? true : false; };
			if (std::find_if(CoreController::globalMenuSettings.begin(), CoreController::globalMenuSettings.end(), functor) != CoreController::globalMenuSettings.end())
			{
				CoreController::globalControlCounter = (*g_ui)->GetFlagCount(IMenu::kFlag_FallSoulsMenu);
				(CoreController::globalControlCounter) ? PlayerControlsEx::DisableUserEvent(true) : PlayerControlsEx::DisableUserEvent(false);
			}
			if (!evn->isOpen)
			{
				static BSFixedString console("Console");
				static BSFixedString levelUpMenu("LevelUpMenu");
				if (evn->menuName == console)
				{
					globalMenuTableLock->LockThis();
					auto & pHashSet = (*g_ui)->menuTable;
					auto * pItem = pHashSet.Find(&console);
					if (pItem && pItem->menuInstance)
					{
						if (CoreController::globalMenuSettings[kType_Console].isEnabled)
						{
							pItem->menuInstance->flags &= ~IMenu::kFlag_PauseGame;
							pItem->menuInstance->flags |= IMenu::kFlag_FallSoulsMenu;
						}
						else
						{
							pItem->menuInstance->flags |= IMenu::kFlag_PauseGame;
							pItem->menuInstance->flags &= ~IMenu::kFlag_FallSoulsMenu;
						}
					}
					globalMenuTableLock->ReleaseThis();
				}
				else if (evn->menuName == levelUpMenu)
				{
					globalMenuTableLock->LockThis();
					auto & pHashSet = (*g_ui)->menuTable;
					auto * pItem = pHashSet.Find(&levelUpMenu);
					if (pItem && pItem->menuInstance)
					{
						if (CoreController::globalMenuSettings[kType_LevelUp].isEnabled)
						{
							pItem->menuInstance->flags &= ~IMenu::kFlag_PauseGame;
							pItem->menuInstance->flags |= IMenu::kFlag_FallSoulsMenu;
						}
						else
						{
							pItem->menuInstance->flags |= IMenu::kFlag_PauseGame;
							pItem->menuInstance->flags &= ~IMenu::kFlag_FallSoulsMenu;
						}
					}
					globalMenuTableLock->ReleaseThis();
				}
			}
			return kEvent_Continue;
		};

		static void RegisterHandler()
		{
			if ((*g_ui) != nullptr)
			{
				static auto * pHandler = new GameplayControlHandler();
				RegisterMenuOpenCloseEvent((*g_ui)->menuOpenCloseEventSource, pHandler);
			}
		}

		static void RegisterCoreControls()
		{
			static bool isInit = false;
			if (!isInit)
			{
				isInit = true;
				Callback<kType_Console>::Register();
				Callback<kType_Terminal>::Register();
				Callback<kType_ExamineConfirm>::Register();
				Callback<kType_LevelUp>::Register();
				Callback<kType_Pause>::Register();
				Callback<kType_Barter>::Register();
				Callback<kType_MessageBox>::Register();
				Callback<kType_PipboyHolotape>::Register();
				Callback<kType_Book>::Register();
				Callback<kType_Container>::Register();
				Callback<kType_SPECIAL>::Register();
				Callback<kType_Examine>::Register();
				Callback<kType_Lockpicking>::Register();
				Callback<kType_SleepWaitMenu>::Register();
				Callback<kType_Pipboy>::Register();
				Callback<kType_TerminalHolotape>::Register();
				Callback<kType_Caravan>::Register();
				GameplayControlHandler::RegisterHandler();
			}
		}
	};


	class VertibirdMenu : public GameMenuBase
	{
	public:
		using FnCanProcess = bool(__thiscall VertibirdMenu::*)(InputEvent *);
		static FnCanProcess	CanProcess_Original;

		bool CanProcess_Hook(InputEvent * inputEvent)
		{
			if (CoreController::globalControlCounter || (*g_ui)->numPauseGame)
				return false;
			return (this->*CanProcess_Original)(inputEvent);
		}

		static void InitHooks()
		{
			CanProcess_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D51278) + 1 * 0x8, &CanProcess_Hook);
		}
	};
	VertibirdMenu::FnCanProcess		VertibirdMenu::CanProcess_Original = nullptr;


	class BookMenu : public GameMenuBase
	{
	public:
		using FnProcessMessage = UInt32(__thiscall BookMenu::*)(UIMessage * msg);
		static FnProcessMessage	ProcessMessage_Original;
		//<SIZE=0x138><VTBL=2DAE2A8><CTOR=01259A90>
		UInt32	ProcessMessage_Hook(UIMessage * msg)
		{
			return (this->*ProcessMessage_Original)(msg);
		}

		static void InitHooks()
		{
			SafeWrite16(RelocAddr<uintptr_t>(0x01257620), 0x9090); //75 08 48 8B CB E8 ? ? ? ? 83 CE FF
			UInt8 codes[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			SafeWriteBuf(RelocAddr<uintptr_t>(0x12577B6), codes, sizeof(codes));//unnormal speed...
			//012577B6
			//ProcessMessage_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DAE2A8) + 3 * 0x8, &ProcessMessage_Hook);
		}
	};
	BookMenu::FnProcessMessage BookMenu::ProcessMessage_Original = nullptr;


	class ContainerMenu : public GameMenuBase
	{
	public:
		using FnIsEnabled = bool(__thiscall ContainerMenu::*)(InputEvent *);
		using FnOnButtonEvent = void(__thiscall ContainerMenu::*)(ButtonEvent *);
		using FnProcessMessage = UInt32(__thiscall ContainerMenu::*)(UIMessage * msg);

		static FnIsEnabled	IsEnabled_Original;
		static FnOnButtonEvent	OnButtonEvent_Original;
		static FnProcessMessage	ProcessMessage_Original;
		//<SIZE=0x440><VTBL=2DAF3F8><CTOR=12631B0>
		bool IsEnabled_Hook(InputEvent * inputEvent) 
		{ 
			bool result = (this->*IsEnabled_Original)(inputEvent);
			if (result && inputEvent)
			{
				auto * pControlID = inputEvent->GetControlID();
				_MESSAGE("==>deviceType=%d eventType=%d controlID=%s", inputEvent->deviceType, inputEvent->eventType, (pControlID != nullptr) ? pControlID->c_str() : "");
			}
			return result;
		}

		UInt32	ProcessMessage_Hook(UIMessage * msg)
		{
			if (msg->type == kMessage_Open)
			{
				static BSFixedString dialogueMenu("DialogueMenu");
				(*g_uiMessageManager)->SendUIMessage(dialogueMenu, kMessage_Close);
			}
			return (this->*ProcessMessage_Original)(msg);
		}

		static void InitHooks()
		{
			//IsEnabled_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DAF508) + 1 * 0x8, &IsEnabled_Hook);
			//ProcessMessage_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DAF3F8) + 3 * 0x8, &ProcessMessage_Hook);
			SafeWrite8(RelocAddr<uintptr_t>(0x12636EB), 0xEB); //75 08 48 8B CB E8 ? ? ? ? 83 CE FF 
		}
	};
	ContainerMenu::FnIsEnabled	ContainerMenu::IsEnabled_Original = nullptr;
	ContainerMenu::FnOnButtonEvent ContainerMenu::OnButtonEvent_Original = nullptr;
	ContainerMenu::FnProcessMessage ContainerMenu::ProcessMessage_Original = nullptr;


	class BarterMenu : public GameMenuBase
	{
	public:
		using FnProcessMessage = UInt32(__thiscall BarterMenu::*)(UIMessage * msg);
		static FnProcessMessage	ProcessMessage_Original;

		UInt32	ProcessMessage_Hook(UIMessage * msg)
		{
			if (msg->type == kMessage_Open)
			{
				static BSFixedString dialogueMenu("DialogueMenu");
				(*g_uiMessageManager)->SendUIMessage(dialogueMenu, kMessage_Close);
			}
			return (this->*ProcessMessage_Original)(msg);
		}

		static void InitHooks()
		{
			//ProcessMessage_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D43778) + 3 * 0x8, &ProcessMessage_Hook);
		}

	};
	BarterMenu::FnProcessMessage		BarterMenu::ProcessMessage_Original = nullptr;


	class ExamineMenu : public GameMenuBase
	{
	public:

		static void InitHooks()
		{
			SafeWrite8(RelocAddr<uintptr_t>(0x0B1A72B), 0xEB); //this is also used in other menus.B1A630 all kinds of mod menu,cooking menu.
		}
	};


	class SleepWaitMenu : public GameMenuBase
	{
	public:
		using FnProcessMessage = UInt32(__thiscall SleepWaitMenu::*)(UIMessage *);
		using FnInvoke = void(__thiscall SleepWaitMenu::*)(Args *);
		using FnCanProcess = bool(__thiscall SleepWaitMenu::*)(BSFixedString &);

		static FnProcessMessage ProcessMessage_Original;
		static FnInvoke			Invoke_Original;
		static FnCanProcess		CanProcess_Original;

		enum
		{
			kMessage_ConfirmWait = 0x10,
			kMessage_RequestWait
		};
		//<SIZE=0x108><VTBL=2DB5338><CTOR=12B0600>
		class ConfirmWaitingDelegate : public ITaskDelegate
		{
		public:
			ConfirmWaitingDelegate(){};
			virtual void Run() override
			{
				static BSFixedString sleepWaitMenu("SleepWaitMenu");
				(*g_uiMessageManager)->SendUIMessage(sleepWaitMenu, kMessage_ConfirmWait);
			}
			static void Register()
			{
				s_taskQueueLock.Enter();
				auto * pTask = new ConfirmWaitingDelegate();
				s_tasks.push(pTask);
				s_taskQueueLock.Leave();
			}
		};
		void Invoke_Hook(Args * scaleformArgs)
		{
			if (!scaleformArgs->optionID && !(this->flags & kFlag_PauseGame))
			{
				static BSFixedString sleepWaitMenu("SleepWaitMenu");
				(*g_uiMessageManager)->SendUIMessage(sleepWaitMenu, kMessage_RequestWait);
				return;
			}
			return (this->*Invoke_Original)(scaleformArgs);
		}

		UInt32	ProcessMessage_Hook(UIMessage * msg)
		{
			switch (msg->type)
			{
			case kMessage_RequestWait:
			{
				if (!(*g_ui)->numPauseGame && !(this->flags & kFlag_IncMenuModeCounter))
				{
					(*g_ui)->numPauseGame += 1;
					this->flags |= kFlag_IncMenuModeCounter;
					ConfirmWaitingDelegate::Register();
				}
				return 0;
			}
			case kMessage_ConfirmWait:
			{
				this->ConfirmWaiting();
				return 0;
			}
			}
			UInt32 result = (this->*ProcessMessage_Original)(msg);
			if (msg->type == kMessage_Close && result != 1 && (this->flags & kFlag_IncMenuModeCounter))
			{
				(*g_ui)->numPauseGame -= 1;
				this->flags &= ~kFlag_IncMenuModeCounter;
			}
			return result;
		}

		bool CanProcess_Hook(BSFixedString & controlID)
		{
			static BSFixedString accept("Accept");
			static BSFixedString activate("Activate");
			if ((controlID == accept || controlID == activate) && !(this->flags & kFlag_PauseGame))
			{
				static BSFixedString sleepWaitMenu("SleepWaitMenu");
				(*g_uiMessageManager)->SendUIMessage(sleepWaitMenu, kMessage_RequestWait);
				return true;
			}
			return (this->*CanProcess_Original)(controlID);
		}

		static void InitHooks()
		{
			Invoke_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DB5338) + 1 * 0x8, &Invoke_Hook);
			ProcessMessage_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DB5338) + 3 * 0x8, &ProcessMessage_Hook);
			CanProcess_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DB5338) + 0xF * 0x8, &CanProcess_Hook);
		}

	private:
		DEFINE_MEMBER_FUNCTION(ConfirmWaiting, void, 0x12B1180);
	};
	SleepWaitMenu::FnInvoke							SleepWaitMenu::Invoke_Original = nullptr;
	SleepWaitMenu::FnProcessMessage					SleepWaitMenu::ProcessMessage_Original = nullptr;
	SleepWaitMenu::FnCanProcess						SleepWaitMenu::CanProcess_Original = nullptr;


	class PipboyMenu : public GameMenuBase
	{
	public:
		using FnProcessMessage = UInt32(__thiscall PipboyMenu::*)(UIMessage *);
		using FnInvoke = void(__thiscall PipboyMenu::*)(Args *);

		static FnProcessMessage ProcessMessage_Original;
		static FnInvoke			Invoke_Original;

		class PipboyInventoryMenu
		{
		public:
			DEFINE_MEMBER_FUNCTION(SelectItem, void, 0xB8DED0, SInt32 selectedIndex);
		};

		UInt32	ProcessMessage_Hook(UIMessage * msg)
		{
#ifdef DEBUG
			if (msg->type == kMessage_Open)
			{
				_MESSAGE("IMENU=%016I64X | MOVIEVIEW=%016I64X | MOVIEROOT=%016I64X", this, this->movie, this->movie->movieRoot);
			}
#endif
			if (msg->type == kMessage_Close && !(this->flags & kFlag_PauseGame))
			{
				if (!(this->flags & kFlag_IncMenuModeCounter))
				{
					(*g_ui)->numPauseGame += 1;
					this->flags |= kFlag_IncMenuModeCounter;
				}
			}
			UInt32 result = (this->*ProcessMessage_Original)(msg);

			if (msg->type == kMessage_Close && result != 1 && (this->flags & kFlag_IncMenuModeCounter))
			{
				(*g_ui)->numPauseGame -= 1;
				this->flags &= ~kFlag_IncMenuModeCounter;
			}
			return result;
		}

		static void InitHooks()
		{
			SafeWrite16(RelocAddr<uintptr_t>(0xB948C5), 0x9090);
			SafeWrite16(RelocAddr<uintptr_t>(0xB93779), 0x9090);
			ProcessMessage_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D4BAF8) + 3 * 0x8, &ProcessMessage_Hook);
		}

		UInt64						unkE0[(0x160 - 0xE0) >> 3];
		PipboyInventoryMenu			pipboyInventoryMenu;			// 160
	};
	STATIC_ASSERT(offsetof(PipboyMenu, pipboyInventoryMenu) == 0x160);
	PipboyMenu::FnProcessMessage					PipboyMenu::ProcessMessage_Original = nullptr;
	PipboyMenu::FnInvoke							PipboyMenu::Invoke_Original = nullptr;


	class PipboyManagerEx : public PipboyManager
	{
	public:
		using FnReceiveEvent = EventResult(__thiscall PipboyManagerEx::*)(BSAnimationGraphEvent *, void *);
		static FnReceiveEvent ReceiveEvent_Original;

		EventResult	ReceiveEvent_Hook(BSAnimationGraphEvent * evn, void * dispatcher)
		{
			static BSFixedString pipboyClosed("pipboyClosed");
			static BSFixedString pipboyMenu("PipboyMenu");
			static BSFixedString levelUpMenu("LevelUpMenu");

			if (evn->name == pipboyClosed)
			{
				using FnSendExtraMessage = void(*)(BSFixedString&, UInt32 type, bool);
				RelocAddr<FnSendExtraMessage> SendExtraMessage(0x204F550);
				SendExtraMessage(levelUpMenu, kMessage_Close, true);
				(*g_pipboyManager)->ClosePipboyMenu(true);
			}
#ifdef DEBUG
			_MESSAGE("%08X|%s", evn->unk00, evn->name.c_str());
#endif
			return (this->*ReceiveEvent_Original)(evn, dispatcher);
		}

		static void InitHooks()
		{
			ReceiveEvent_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2D53CB0) + 1 * 0x8, &ReceiveEvent_Hook);
		}
	};
	PipboyManagerEx::FnReceiveEvent		PipboyManagerEx::ReceiveEvent_Original = nullptr;


	class PipboyHandler : public BSInputEventUser
	{
	public:
		using FnCanProcess = bool(__thiscall PipboyHandler::*)(InputEvent *);
		static FnCanProcess	CanProcess_Original;

		bool CanProcess_Hook(InputEvent * inputEvent)
		{
			if (CoreController::globalControlCounter)
				return false;
			return (this->*CanProcess_Original)(inputEvent);
		}

		static void InitHooks()
		{
			CanProcess_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DB4B38) + 1 * 0x8, &CanProcess_Hook);
		}
	};
	PipboyHandler::FnCanProcess		PipboyHandler::CanProcess_Original = nullptr;


	class MainMenu : public GameMenuBase
	{
	public:
		static void RegisterMenuOpenCloseEvent_Hook(BSTEventDispatcher<MenuOpenCloseEvent> & dispatcher, BSTEventSink<MenuOpenCloseEvent> * pHandler)
		{
			GameplayControlHandler::RegisterCoreControls();
			return RegisterMenuOpenCloseEvent(dispatcher, pHandler);
		}

		static void InitHooks()
		{
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0x0129FFB7), (uintptr_t)RegisterMenuOpenCloseEvent_Hook);
		}
	};


	class DialogueMenu : public GameMenuBase
	{
	public:
		using FnCreateInstance = DialogueMenu*(*)(void *);
		using FnProcessMessage = UInt32(__thiscall DialogueMenu::*)(UIMessage *);

		static FnProcessMessage ProcessMessage_Original;
		UInt32	ProcessMessage_Hook(UIMessage * msg)
		{
			if (msg->type == kMessage_Open && CoreController::globalControlCounter)
				return 1;
			return (this->*ProcessMessage_Original)(msg);
		}

		static DialogueMenu * CreateInstance_Hook(void * pMem)
		{
			RelocAddr<FnCreateInstance>	CreateInstance_Original(0x126ABE0);
			auto * result = CreateInstance_Original(pMem);
			result->context = 5;
			return result;
		}
		static void InitHooks()
		{
			//g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0x126C315), (uintptr_t)CreateInstance_Hook);
			//ProcessMessage_Original = HookUtil::SafeWrite64(RelocAddr<uintptr_t>(0x2DAFBA8) + 3 * 0x8, &ProcessMessage_Hook);
		}
	};
	DialogueMenu::FnProcessMessage					DialogueMenu::ProcessMessage_Original = nullptr;


	class MenuTopicManagerEx : public MenuTopicManager
	{
	public:
		using FnIsDialogueMenuDisabled = bool(__thiscall MenuTopicManagerEx::*)();

		bool IsDialogueMenuDisabled_Hook()
		{
			if (CoreController::globalControlCounter)	return true;
			return this->IsDialogueMenuDisabled();
		}

		static void InitHooks()
		{
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0xCA244E), GetFnAddr(&IsDialogueMenuDisabled_Hook));
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0xCA32C7), GetFnAddr(&IsDialogueMenuDisabled_Hook));
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0xEA64BE), GetFnAddr(&IsDialogueMenuDisabled_Hook));
		}
	};


	class VirtualMachineEx : public GameVM
	{
	public:

		static void UpdateVirtualMachineTimer_Hook(GameVM * vm, Main * main, UInt32 milliseconds)
		{
			if (!main->menuMode && !CoreController::globalControlCounter)
			{
				vm->worldRunningTimer += milliseconds;
			}
		}

		static bool IsInMenuMode_Hook(/*VirtualMachine * vm, UInt32 stackID, ...*/)
		{
			return ((*g_ui)->numPauseGame || CoreController::globalControlCounter);
		}

		static void InitHooks()
		{
			static RelocAddr <uintptr_t> UpdateVirtualMachineTimer_Entry(0x1371AC3);
			struct UpdateVirtualMachineTimer_Code : Xbyak::CodeGenerator
			{
				UpdateVirtualMachineTimer_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					mov(r8d, ecx);
					mov(rcx, rdi);
					mov(rdx, rax);
					mov(rax, (uintptr_t)UpdateVirtualMachineTimer_Hook);
					call(rax);
					jmp(ptr[rip + retnLabel]);
					L(retnLabel);
					dq(UpdateVirtualMachineTimer_Entry.GetUIntPtr() + 0xF);
				}
			};
			void * codeBuf = g_localTrampoline.StartAlloc();
			UpdateVirtualMachineTimer_Code code(codeBuf);
			g_localTrampoline.EndAlloc(code.getCurr());
			g_branchTrampoline.Write5Branch(UpdateVirtualMachineTimer_Entry.GetUIntPtr(), (uintptr_t)codeBuf);
			SafeWrite16(UpdateVirtualMachineTimer_Entry.GetUIntPtr() + 5, 0x9090);

			g_branchTrampoline.Write5Branch(RelocAddr<uintptr_t>(0x1453B50), (uintptr_t)IsInMenuMode_Hook);
			SafeWrite16(RelocAddr<uintptr_t>(0x1453B50).GetUIntPtr() + 5, 0x9090);
		}
	};


	class MainEx : public Main
	{
	public:
		static bool MessageQueueProcessTask_Hook(void * messageQueue, float timeout, UInt32 unk1)
		{
			bool result = MessageQueueProcessTask(messageQueue, timeout, unk1);
			s_taskQueueLock.Enter();
			while (!s_tasks.empty())
			{
				ITaskDelegate * cmd = s_tasks.front();
				s_tasks.pop();
				cmd->Run();
				delete cmd;
			}
			s_taskQueueLock.Leave();
			return result;
		}

		static void IncreaseCullingProcessRef_Hook(void * pObj, bool isDisabled) 
		{
			IncreaseCullingProcessRef(pObj);
			if (!isDisabled || CoreController::globalControlCounter)
			{
				ProcessUserInterface();
			}
		}

		static void InitHooks()
		{
			static RelocAddr <uintptr_t> UpdateUserInterface_Entry(0x0D38FEB);
			struct UpdateUserInterface_Code : Xbyak::CodeGenerator
			{
				UpdateUserInterface_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					mov(dl, bl);
					mov(rax, (uintptr_t)IncreaseCullingProcessRef_Hook);
					call(rax);

					jmp(ptr[rip + retnLabel]);
					L(retnLabel);
					dq(UpdateUserInterface_Entry.GetUIntPtr() + 0x5);
				}
			};
			void * codeBuf = g_localTrampoline.StartAlloc();
			UpdateUserInterface_Code code(codeBuf);
			g_localTrampoline.EndAlloc(code.getCurr());
			g_branchTrampoline.Write5Branch(UpdateUserInterface_Entry.GetUIntPtr(), (uintptr_t)codeBuf);
			SafeWrite8(UpdateUserInterface_Entry.GetUIntPtr() + 7, 0xEB);

			//g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0x0D38FEB), (uintptr_t)IncreaseCullingProcessRef_Hook);
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0x0D3916B), (uintptr_t)MessageQueueProcessTask_Hook);
		}
	};


	class TESAnimationJob
	{
	public:
		static void ProcessUserInterfaceTask_Hook()// non-main thread.
		{
			if (!(*isGameLoading) && !CoreController::globalControlCounter)
			{
				ProcessUserInterface();
			}
			SetThreadEvent(1);
		}

		static void RegisterThreadTask_Hook(void * pObj, void * functor, void * unk2, const char * name)
		{
			RegisterThreadTask(pObj, ProcessUserInterfaceTask_Hook, unk2, name);
		}

		static void InitHooks()
		{
			SafeWrite16(RelocAddr<uintptr_t>(0xCBCD4B), 0x9090);
			UInt8 codes[] = { 0xC3, 0x90, 0x90, 0x90, 0x90 };
			SafeWriteBuf(RelocAddr<uintptr_t>(0xCBCDB8), codes, sizeof(codes));
			g_branchTrampoline.Write5Call(RelocAddr<uintptr_t>(0xCBCFB2), (uintptr_t)RegisterThreadTask_Hook);
		}
	};


	void InstallHooks()
	{
		MainMenu::InitHooks();
		BookMenu::InitHooks();
		VertibirdMenu::InitHooks();
		ContainerMenu::InitHooks();
		BarterMenu::InitHooks();
		ExamineMenu::InitHooks();
		PipboyMenu::InitHooks();
		PipboyManagerEx::InitHooks();
		PipboyHandler::InitHooks();
		DialogueMenu::InitHooks();
		MenuTopicManagerEx::InitHooks();
		SleepWaitMenu::InitHooks();
		PlayerControlsEx::InitHooks();
		PlayerCharacterEx::InitHooks();
		VirtualMachineEx::InitHooks();
		MainEx::InitHooks();
		TESAnimationJob::InitHooks();
	}
}


                                           