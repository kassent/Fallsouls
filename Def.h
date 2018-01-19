#pragma once
#include "f4se_common/Utilities.h"
#include "f4se_common/Relocation.h"

#include "f4se/GameTypes.h"
#include "f4se/GameUtilities.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/GameCamera.h"
#include "f4se/GameInput.h"
#include "f4se/GameEvents.h"
#include "f4se/GameThreads.h"
#include "f4se/GameReferences.h"
#include "f4se/PapyrusVM.h"

#include <string>
#include <vector>
#include <memory>


namespace FallSouls
{
	class TlsManager
	{
	public:
		TlsManager(uint32_t thread);
		~TlsManager();

	private:
		uint32_t	tlsID;
	};

	enum Type
	{
		kMessage_Refresh = 0,
		kMessage_Open,
		kMessage_Close = 3,
		kMessage_Scaleform = 5,
		kMessage_Message
	};


	class UIMessage;

	class UIMessageManager
	{
	public:

		DEFINE_MEMBER_FUNCTION(SendUIMessage, void, 0x204C580, BSFixedString& menuName, UInt32 type);//48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 20 44 8B 0D ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 8B E9 4A 8B 34 C8
		DEFINE_MEMBER_FUNCTION(SendUIMessageEx, void, 0x12BA470, BSFixedString& menuName, UInt32 type, UIMessage * pExtraData);//seems it's a template function,different menu has different ex function.E8 ? ? ? ? 48 8B 4C 24 ? 48 85 C9 74 0A 48 8B 01 BA ? ? ? ? FF 10 48 8B 6C 24 ? 48 8B 7C 24 ?
	};
	extern RelocPtr<UIMessageManager*>	g_uiMessageManager;


	class UIMessage
	{
	public:
		UIMessage();
		virtual ~UIMessage();

		//void						** vtbl; //02EC5660
		BSFixedString				name;		// 08
		UInt32						type;		// 10 init'd 0xD

	private:
		//DEFINE_MEMBER_FUNCTION(ctor, UIMessage*, 0x204E8D0);
		//DEFINE_MEMBER_FUNCTION(dtor, void, 0x204E900, bool);
	};
	STATIC_ASSERT(sizeof(UIMessage) == 0x18);


	class IUIMessageData : public UIMessage
	{
	public:
		IUIMessageData();
		~IUIMessageData();

		//void						** vtbl; //02CB20D0
	};

	class BSUIScaleformData : public IUIMessageData
	{
	public:
		virtual ~BSUIScaleformData();

		//void						** _vtbl;		// 00 - 02EC5640
		void						* event;		// 18
	};
	STATIC_ASSERT(sizeof(BSUIScaleformData) == 0x20);


	class BSUIMessageData : public IUIMessageData
	{
	public:
		virtual ~BSUIMessageData();			

		//void						** vtbl;	//02D372B8								// @members
		BSString					* unk08;	// 18 ????
		BSFixedString				name;		// 20
		UInt32						unk28;		// 28
	};
	STATIC_ASSERT(sizeof(BSUIMessageData) == 0x30);
}

namespace FallSouls
{
	using _PlayUISound = void(*)(const char *);
	extern RelocAddr<_PlayUISound>	PlayUISound;

	class IMenu;
	class UI;
	class ScaleformArgs;
	class DelegateArgs;

	class BSScaleformManager
	{
	public:
		virtual ~BSScaleformManager();
		virtual void Unk_01(void); // Init image loader?

		UInt64					unk08;			// 08 - 0
		GFxStateBag				* stateBag;		// 10
		void					* unk18;		// 18
		void					* unk20;		// 20
		BSScaleformImageLoader	* imageLoader;	// 28

		DEFINE_MEMBER_FUNCTION(LoadMovie, bool, 0x21104C0, IMenu * menu, GFxMovieView *&, const char * name, const char * stagePath, UInt32 flags); //48 8B C4 4C 89 40 18 48 89 48 08 55 56 57 41 54 41 57
	};
	extern RelocPtr <BSScaleformManager *> g_scaleformManager;

	class DelegateArgs
	{
	public:
		DelegateArgs(IMenu * menu, ScaleformArgs * args);
		std::string				menuName;
		UInt32					optionID;
		std::vector<GFxValue>	parameters;
	};

	class ScaleformArgs
	{
	public:

		ScaleformArgs(IMenu * menu, const DelegateArgs & delegateArgs);

		GFxValue		* result;	// 00
		GFxMovieView	* movie;	// 08
		GFxValue		* thisObj;	// 10
		GFxValue		* unk18;	// 18
		const GFxValue	* args;		// 20
		UInt32			numArgs;	// 28
		UInt32			pad2C;		// 2C
		UInt32			optionID;	// 30 pUserData
	};
	STATIC_ASSERT(offsetof(ScaleformArgs, optionID) == 0x30);

	class GFxFunctionHandler
	{
	public:
		using Args = ScaleformArgs;

		GFxFunctionHandler() : refCount(1) {};
		virtual void *	ReleaseThis(bool releaseMem) = 0;
		virtual void	Invoke(Args * args) = 0; //redefine GFxValue !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		//	void	** _vtbl;			// 00
		volatile	SInt32	refCount;	// 08
		UInt32		pad0C;				// 0C
	};

	class SWFToCodeFunctionHandler : public GFxFunctionHandler
	{
	public:
		virtual void	RegisterFunctions() = 0;	// 02

		DEFINE_MEMBER_FUNCTION(RegisterFunction, void, 0x210FD80, const char * name, UInt32 index); //40 53 48 83 EC 50 48 8B DA 48 8B D1 48 8B 0D ? ? ? ? 48 85 C9
	};

	class IMenu : public SWFToCodeFunctionHandler, //02EDDBD8
				public BSInputEventUser
	{
	public:
		enum
		{
			//Confirmed
			kFlag_PauseGame = 0x01,
			kFlag_ShowCursor = 0x04,
			kFlag_EnableMenuControl = 0x08, // 1, 2
			kFlag_ShaderWorld = 0x20,
			kFlag_Open = 0x40,//set it after open.
			kFlag_BlurBackground = 0x400000, 
			kFlag_DoNotPreventGameSave = 0x800,
			kFlag_ApplyDropDownFilter = 0x8000,
			kFlag_Singleton = 0x200,
			kFlag_IncMenuModeCounter = 0x80000000,
			kFlag_FallSoulsMenu = 0x40000000,
			kFlag_DoNotDeleteOnClose = 0x02,
			//Unconfirmed
			kFlag_Modal = 0x10,
			kFlag_PreventGameLoad = 0x80,
			kFlag_Unk0100 = 0x100,
			kFlag_Unk0400 = 0x400,
			kFlag_Unk1000 = 0x1000,
			kFlag_ItemMenu = 0x2000,
			kFlag_StopCrosshairUpdate = 0x4000,
			kFlag_Unk10000 = 0x10000	// mouse cursor
		};
		virtual UInt32	ProcessMessage(UIMessage * msg) = 0;//???
		virtual void	DrawNextFrame(float unk0, void * unk1) = 0; //210E8C0
		virtual void *	Unk_05(void) { return nullptr; }; //return 0;
		virtual void *	Unk_06(void) { return nullptr; }; //return 0;
		virtual bool	Unk_07(UInt32 unk0, void * unk1) = 0;
		virtual void	Unk_08(UInt8 unk0) = 0;
		virtual void	Unk_09(BSFixedString & menuName, bool unk1) = 0;            //UInt64
		virtual void	Unk_0A(void) = 0;
		virtual void	Unk_0B(void) = 0;
		virtual void	Unk_0C(void) = 0;
		virtual bool	Unk_0D(bool unk0) = 0;
		virtual bool	Unk_0E(void) { return false; };
		virtual bool	CanProcessControl(BSFixedString & controlID) { return false; };
		virtual bool	Unk_10(void) = 0;
		virtual void	Unk_11(void) = 0;
		virtual void	Unk_12(void * unk0) = 0;

		GFxValue		stage;			// 20
		GFxMovieView	* movie;		// 40
		BSFixedString	unk48;			// 48
		BSFixedString	menuName;		// 50
		UInt32			flags;			// 58

		/*
								A A A A A A A A B B B B B B B B C C C C C C C C D D D D D D D D	
		LoadingMenu				0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 1 0 1 1 0 0 0 0 0 1		depth: 000E		context: 0003
		FavoritesMenu			0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1 1 0 1 0 0 0 0 0 0 1 0 0 0 0 0 0		depth: 0006		context: 0001
		FaderMenu				0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 0		depth: 0006		context: 0022
		CursorMenu				0 0 0 0 0 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0014		context: 0022
		VATSMenu				0 0 0 0 0 0 0 0 1 0 0 0 1 1 0 1 1 0 0 0 0 1 0 0 0 1 0 0 0 1 0 0		depth: 0006		context: 000D
		VignetteMenu			0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0003		context: 0022
		LevelUpMenu				0 0 0 0 0 1 0 0 0 0 0 0 1 1 0 1 0 0 0 0 0 1 0 0 1 1 0 0 0 1 1 1		depth: 0009		context: 0022
		MessageBoxMenu			0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 1 0 0 1 1 0 1 1 1 0 1		depth: 000A		context: 0022
		ExamineMenu				0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 0 1 0 1 0 0 1 0 1 0 1 0 0 0 1 0 1		depth: 0009		context: 0022
		ExamineConfirmMenu		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 0 1 0 1 1 1 0 1		depth: 0011		context: 0022
		PowerArmorHUDMenu		0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0005		context: 0022
		PromptMenu				0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 1 1 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0005		context: 0022
		SitWaitMenu				0 0 0 0 0 0 0 0 1 0 0 0 1 1 0 0 1 1 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0006		context: 0012
		SleepWaitMenu			0 0 0 0 1 0 0 0 0 1 0 0 1 1 0 1 1 0 0 0 1 0 0 1 1 1 0 0 1 1 0 1		depth: 000A		context: 0022
		DialogueMenu			0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0006		context: 0022
		BarterMenu				0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 1 1 0 1 0 0 1 0 1 0 1 0 0 1 1 0 1		depth: 0006		context: 0022
		LockpickingMenu			0 0 0 0 0 0 0 0 0 1 0 0 1 1 0 0 1 0 0 0 0 0 0 0 0 1 1 0 0 0 0 1		depth: 0006		context: 000C
		WorkshopMenu			0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 1 0 0 0 0 0 0 1 0 1 0 0 0 0 0 0		depth: 0006		context: 0010
		SPECIALMenu				0 0 0 0 1 0 0 0 0 1 0 0 1 1 0 1 1 0 0 0 0 1 0 0 1 1 1 0 1 1 0 1		depth: 0006		context: 0022
		Console					0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 1 0 0 0 1 1 1		depth: 0013		context: 0006
		PauseMenu				0 0 0 0 1 0 0 0 0 1 0 0 1 1 0 0 1 0 0 0 1 1 1 0 0 1 0 1 1 1 0 1		depth: 000B		context: 0022
		BookMenu				0 0 0 0 1 0 0 0 0 1 1 0 1 1 0 0 1 0 0 0 0 0 0 1 0 1 1 0 1 0 0 1		depth: 0009		context: 0008

		ContainerMenu			0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 1 1 0 1 0 0 1 0 1 0 1 0 0 1 1 0 1		depth: 0006		context: 0022
		RobotModMenu			0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 0 1 0 1 0 0 1 0 1 0 1 0 0 0 1 0 0		depth: 0009		context: 0022
		PowerArmorModMenu		0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 0 1 0 1 0 0 1 0 1 0 1 0 0 0 1 0 0		depth: 0009		context: 0022
		CookingMenu				0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 0 1 0 1 0 0 1 0 1 0 1 0 0 0 1 0 0		depth: 0009		context: 0022

		HUDMenu					0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 1 0 0 1 0 0 0 0 1 0 0 0 0 0 0		depth: 0005		context: 0022
		PipboyMenu				0 0 0 0 0 0 0 0 1 0 1 0 1 1 0 0 1 0 1 0 0 0 0 1 0 1 0 0 0 1 0 1		depth: 0008		context: 0022
		PipboyHolotapeMenu		0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 1 0 0 1		depth: 0009		context: 0022	
		TerminalMenu			0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 0 1 0 0 1 1 0 0		depth: 0009		context: 0022
		TerminalMenuButtons		0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0		depth: 0006		context: 0022
		*/
		UInt32			unk5C;			// 5C
		UInt32			unk60;			// 60	init'd as DWord then Byte
		UInt8			depth;			// 64   defalut is 6.
		UInt32			context;		// 68	init'd in IMenu::IMenu
		UInt32			pad6C;			// 6C
	};
	STATIC_ASSERT(offsetof(IMenu, movie) == 0x40);
	STATIC_ASSERT(offsetof(IMenu, flags) == 0x58);

	// E0
	class GameMenuBase : public IMenu
	{
	public:
		GameMenuBase(){ this->InitializeThis(); }
		virtual void	Invoke(Args * args) override { }
		virtual void	RegisterFunctions() override { } //210FAA0
		virtual UInt32	ProcessMessage(UIMessage * msg) override { return this->ProcessMessage_Imp(msg); };//???
		virtual void	DrawNextFrame(float unk0, void * unk1) override { return this->DrawNextFrame_Imp(unk0, unk1); }; //render,HUD menu uses this function to update its HUD components.
		virtual bool	Unk_07(UInt32 unk0, void * unk1) override { return this->Unk07_Imp(unk0, unk1); };
		virtual void	Unk_08(UInt8 unk0) override { return this->Unk08_Imp(unk0); };
		virtual void	Unk_09(BSFixedString& menuName, bool unk1) override { return this->Unk09_Imp(menuName, unk1); };            //UInt64
		virtual void	Unk_0A(void) override  { return this->Unk0A_Imp(); };
		virtual void	Unk_0B(void) override  { return this->Unk0B_Imp(); }
		virtual void	Unk_0C(void) override  { return this->Unk0C_Imp(); };
		virtual bool	Unk_0D(bool unk0) override  { return this->Unk0D_Imp(unk0); }
		virtual bool	Unk_0E(void) override { return false; };
		virtual bool	CanProcessControl(BSFixedString & controlID) override { return false; };
		virtual bool	Unk_10(void) override { return this->Unk10_Imp(); } //90 - E0
		virtual void	Unk_11(void) override { return this->Unk11_Imp(); };
		virtual void	Unk_12(void * unk0) override { return this->Unk12_Imp(unk0); }
		virtual void	Unk_13(void * unk0, void * unk1) { return this->Unk13_Imp(unk0, unk1); }
		/*
		.rdata:0000000002D45E78 off_2D45E78     dq offset sub_B32D60					0
		.rdata:0000000002D45E80                 dq offset nullsub_4774					1
		.rdata:0000000002D45E88                 dq offset nullsub_4775					2
		.rdata:0000000002D45E90                 dq offset sub_210E840					3
		.rdata:0000000002D45E98                 dq offset sub_210E8C0					4
		.rdata:0000000002D45EA0                 dq offset nullsub_4776					5
		.rdata:0000000002D45EA8                 dq offset nullsub_4771					6
		.rdata:0000000002D45EB0                 dq offset sub_210ED00					7
		.rdata:0000000002D45EB8                 dq offset sub_B328D0					8
		.rdata:0000000002D45EC0                 dq offset sub_210EF40					9
		.rdata:0000000002D45EC8                 dq offset sub_B32940					A
		.rdata:0000000002D45ED0                 dq offset sub_B32A00					B
		.rdata:0000000002D45ED8                 dq offset sub_B32A40					C
		.rdata:0000000002D45EE0                 dq offset sub_210F090					D
		.rdata:0000000002D45EE8                 dq offset sub_210F0D0					E
		.rdata:0000000002D45EF0                 dq offset sub_B088D0					F
		.rdata:0000000002D45EF8                 dq offset sub_B326F0					10
		.rdata:0000000002D45F00                 dq offset sub_B32780					11
		.rdata:0000000002D45F08                 dq offset sub_B327F0					12
		.rdata:0000000002D45F10                 dq offset sub_B32840					13
		*/

		tArray<BSGFxDisplayObject*>		subcomponents;					// 70
		BSGFxShaderFXTarget				* shaderTarget;					// 88
		UInt64							unk90[(0xE0 - 0x90) >> 3];		// 90

	private:
		DEFINE_MEMBER_FUNCTION(DrawNextFrame_Imp, void, 0x210E8C0, float unk0, void * unk1); //40 53 48 83 EC 20 48 83 79 ? ? 48 8B D9 74 1E
		DEFINE_MEMBER_FUNCTION(ProcessMessage_Imp, UInt32, 0x210E840, UIMessage * msg);//
		DEFINE_MEMBER_FUNCTION(Unk07_Imp, bool, 0x210ED00, UInt32 unk0, void * unk1);
		DEFINE_MEMBER_FUNCTION(Unk08_Imp, void, 0xB328D0, UInt8 unk0);
		DEFINE_MEMBER_FUNCTION(Unk09_Imp, void, 0x210EF40, BSFixedString & menuName, bool unk1);
		DEFINE_MEMBER_FUNCTION(Unk0A_Imp, void, 0xB32940);
		DEFINE_MEMBER_FUNCTION(Unk0B_Imp, void, 0xB32A00);
		DEFINE_MEMBER_FUNCTION(Unk0C_Imp, void, 0xB32A40);
		DEFINE_MEMBER_FUNCTION(Unk0D_Imp, bool, 0x210F090, bool unk0);
		DEFINE_MEMBER_FUNCTION(Unk10_Imp, bool, 0xB326F0);
		DEFINE_MEMBER_FUNCTION(Unk11_Imp, void, 0xB32780);
		DEFINE_MEMBER_FUNCTION(Unk12_Imp, void, 0xB327F0, void * unk0);
		DEFINE_MEMBER_FUNCTION(Unk13_Imp, void, 0xB32840, void * unk0, void * unk1);
		DEFINE_MEMBER_FUNCTION(InitializeThis, void *, 0xB32360);
	protected:
		DEFINE_MEMBER_FUNCTION(ReleaseParent, void *, 0xB32420);
	};
	STATIC_ASSERT(offsetof(GameMenuBase, shaderTarget) == 0x88);

	// 20
	template <class T>
	class HUDContextArray
	{
	public:
		T			* entries;	// 00
		UInt32		count;		// 08
		UInt32		unk0C;		// 0C
		UInt32		flags;		// 10
		UInt32		unk14;		// 14
		UInt32		unk18;		// 18
		bool		unk1C;		// 1C
	};

	// F8
	class HUDComponentBase : public BSGFxShaderFXTarget
	{
	public:
		HUDComponentBase(GFxValue * parent, const char * componentName, HUDContextArray<BSFixedString> * contextList);
		virtual ~HUDComponentBase();

		virtual bool Unk_02() { return false; }
		virtual void Unk_03() { }
		virtual void UpdateComponent() { CALL_MEMBER_FN(this, Impl_UpdateComponent)(); } // Does stuff
		virtual void UpdateVisibilityContext(void * unk1);
		virtual void ColorizeComponent();
		virtual bool IsVisible() { return CALL_MEMBER_FN(this, Impl_IsVisible)(); }
		virtual bool Unk_08() { return contexts.unk1C; }

		UInt64							unkB0;			// B0
		UInt64							unkB8;			// B8
		UInt64							unkC0;			// C0
		HUDContextArray<BSFixedString>	contexts;		// C8
		UInt32							unkE8;			// E8
		UInt32							unkEC;			// EC
		UInt8							unkF0;			// F0
		UInt8							unkF1;			// F1
		bool							isWarning;		// F2 - This chooses the warning color over the default color
		UInt8							padF3[5];		// F3

		MEMBER_FN_PREFIX(HUDComponentBase);
		DEFINE_MEMBER_FN(Impl_ctor, HUDComponentBase *, 0x00A228F0, GFxValue * parent, const char * componentName, HUDContextArray<BSFixedString> * contextList);
		DEFINE_MEMBER_FN(Impl_IsVisible, bool, 0x00A22C30);
		DEFINE_MEMBER_FN(Impl_UpdateComponent, void, 0x00A22990);
	};
	STATIC_ASSERT(offsetof(HUDComponentBase, contexts) == 0xC8);
	STATIC_ASSERT(offsetof(HUDComponentBase, unkE8) == 0xE8);
	STATIC_ASSERT(sizeof(HUDComponentBase) == 0xF8);

	typedef bool(*_HasHUDContext)(HUDContextArray<BSFixedString> * contexts, void * unk1);
	extern RelocAddr <_HasHUDContext> HasHUDContext;


	// 110
	class HUDComponents
	{
	public:
		UInt64								unk00;					// 00
		HUDComponentBase					* components[0x1E];		// 08
		UInt64								unk98;					// 98
		UInt64								unk100;					// 100
		UInt32								numComponents;			// 108 - 0x1E

		MEMBER_FN_PREFIX(HUDComponents);
		DEFINE_MEMBER_FN(Impl_Destroy, void, 0x01282730);	// 3DD133AB9DDB89D138FB8958EB3A68CBF2F15DD9+FE
	};

	// 220
	class HUDMenu : public GameMenuBase
	{
	public:
		BSTEventSink<UserEventEnabledEvent> inputEnabledSink;		// E0
		BSTEventSink<RequestHUDModesEvent>	requestHudModesSink;	// E8
		HUDComponents						children;				// F0
		UInt64								unk200;					// 200
		UInt64								unk208;					// 208
		UInt64								unk210;					// 210
		UInt64								unk218;					// 218
	};
	STATIC_ASSERT(offsetof(HUDMenu, unk200) == 0x200);

	// 00C
	class MenuTableItem
	{
	public:
		typedef IMenu * (*CallbackType)(void);
		BSFixedString	name;				// 000
		IMenu			* menuInstance;		// 008	0 if the menu is not currently open
		CallbackType	menuConstructor;	// 010
		void			* unk18;			// 018

		bool operator==(const MenuTableItem & rhs) const { return name == rhs.name; }
		bool operator==(const BSFixedString a_name) const { return name == a_name; }
		operator UInt64() const { return (UInt64)name.data->Get<char>(); }

		static inline UInt32 GetHash(BSFixedString * key)
		{
			UInt32 hash;
			CalculateCRC32_64(&hash, (UInt64)key->data, 0);
			return hash;
		}

		void Dump(void)
		{
			_MESSAGE("\t\tname: %s", name.data->Get<char>());
			_MESSAGE("\t\tinstance: %08X", menuInstance);
			_MESSAGE("\t\tconstructor: %08X", (uintptr_t)menuConstructor - RelocationManager::s_baseAddr);
		}
	};


	extern RelocPtr <SimpleLock>		globalMenuStackLock;
	extern RelocPtr <SimpleLock>		globalMenuTableLock;
	// 250 ?
	class UI
	{
	public:
		virtual ~UI();

		virtual void	Unk_01(void);

		typedef IMenu*	(*CreateFunc)(void);
		typedef tHashSet<MenuTableItem, BSFixedString> MenuTable;

		bool	IsMenuOpen(BSFixedString * menuName);
		IMenu * GetMenu(BSFixedString * menuName);
		IMenu * GetMenuByMovie(GFxMovieView * movie);
		void	Register(const char* name, CreateFunc creator)
		{
			CALL_MEMBER_FN(this, RegisterMenu)(name, creator, 0);
		}

		template<typename T>
		void ForEachMenu(T & menuFunc)
		{
			menuTable.ForEach(menuFunc);
		}

		UInt64									unk08;
		UInt64									unk10;
		BSTEventDispatcher<MenuOpenCloseEvent>	menuOpenCloseEventSource;
		BSTEventDispatcher<MenuModeChangeEvent>	menuModeChangeEventSource;	// 70
		UInt64									unk70[(0x190 - 0xC8) / 8];	// 458
		tArray<IMenu*>							menuStack;		// 190
		MenuTable								menuTable;		// 1A8
		UInt32									threadID;        // 1D8
		UInt32									unk1DC;        // 1DC
		UInt32									numPauseGame;   // 1E0 ((0x78 * 4)isInMenuMode when in menu mode, player control will get disabled.
		volatile	SInt32						numFlag2000;	// 1E4
		volatile	SInt32						numFlag80;		// 1E8
		UInt32									numFlag20;		// 1EC													// ...

		inline UInt32 GetFlagCount(UInt32 flag)
		{
			UInt32 result = 0;
			globalMenuStackLock->LockThis();
			for (UInt32 i = 0; i < menuStack.count; ++i)
			{
				auto * pMenu = menuStack[i];
				if (pMenu && pMenu->flags & flag)
					++result;
			}
			globalMenuStackLock->ReleaseThis();
			return result;
		}

		void DumpMenuTable()
		{
			ForEachMenu([](MenuTableItem * item)->bool {
				item->Dump();
				_MESSAGE("");
				return true;
			});
		}

	protected:
		MEMBER_FN_PREFIX(UI);
		DEFINE_MEMBER_FN(RegisterMenu, void, 0x020436E0, const char * name, CreateFunc creator, UInt64 unk1); //40 53 56 57 41 56 41 57 48 83 EC 50 44 8B 15 ? ? ? ?
		DEFINE_MEMBER_FN(IsMenuOpen, bool, 0x2041B50, BSFixedString * name); //E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 48 8B 3D ? ? ? ?
		DEFINE_MEMBER_FUNCTION(ProcessMessage, void, 0x2041E20);//40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 8B 15 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B E9
	};
	STATIC_ASSERT(sizeof(UI) == 0x1F0);
	extern RelocPtr <UI*> g_ui;

	using _RegisterMenuOpenCloseEvent = void(*)(BSTEventDispatcher<MenuOpenCloseEvent> & dispatcher, BSTEventSink<MenuOpenCloseEvent> * pHandler); 
	extern RelocAddr<_RegisterMenuOpenCloseEvent>	RegisterMenuOpenCloseEvent;//call E8 ? ? ? ? 4C 8B AC 24 ? ? ? ? 4C 8B A4 24 ? ? ? ? 48 8B B4 24 ? ? ? ? 48 8D 5D D0


	class Main
	{
	public:
		UInt64			unk00[0x40 >> 3];
		SInt32			mainThreadID;			// 40
		UInt64			unk40[(0x1D0 - 0x48) >> 3];
		bool			menuMode;				// 1D0

		DEFINE_MEMBER_FUNCTION(UpdateGameState, void, 0xD388C0);
	};
	STATIC_ASSERT(offsetof(Main, menuMode) == 0x1D0);
	extern RelocPtr<Main*>	g_main;

	// 87A0?
	class GameVM : public IClientVM
	{
	public:
		virtual ~GameVM();

		virtual void	Unk_01();
		virtual void	Unk_02();

		IStackCallbackSaveInterface m_callbackSaveInterface;	// 08

		enum
		{
			kEventSink_Stats = 0,
			kEventSink_InputEnableLayerDestroyed,
			kEventSink_PositionPlayer,
			kEventSink_FormDelete,
			kEventSink_FormIDRemap,
			kEventSink_InitScript,
			kEventSink_ResolveNPCTemplates,
			kEventSink_UniqueIDChange,
			kEventSink_NumEvents
		};

		void			* m_eventSinks[kEventSink_NumEvents];	// 10
		UInt64			unk50[(0xB0 - 0x50) >> 3];				// 50

		VirtualMachine			* m_virtualMachine;				// B0
		IVMSaveLoadInterface	* m_saveLoadInterface;			// B8
		IVMDebugInterface		* m_debugInterface;				// C0
		SimpleAllocMemoryPagePolicy	m_memoryPagePolicy;			// C8

		UInt64			unkF0[(0x1E0 - 0xF0) >> 3];				// F0
		IObjectHandlePolicy		* m_objectHandlePolicy;			// 1E0
		UInt64				unk1E8[(0x600 - 0x1E8) >> 3];		// 1E8


		UInt32				unk600;								// 600
		SInt32				threadID;							// 604
		volatile SInt32		lockCount;							// 608
		UInt32				worldRunningTimer;					// 60C
		UInt32				gameRunningTimer;					// 610

																// ...


		DEFINE_MEMBER_FUNCTION(SendPapyrusEvent, void, 0x01374A10, UInt64 handle, const BSFixedString & eventName, std::function<bool(void*)> functor); // Signature not correct yet
	};
	STATIC_ASSERT(offsetof(GameVM, worldRunningTimer) == 0x60C);


	class TimerEventHandler
	{
	public:
		void				** vtbl;
		UInt64				singleton;
		GameVM				* vm;
	};

	extern RelocPtr<TimerEventHandler*>	g_timerEventHandler;//5A12280


															// 148
	class InputManager
	{
	public:
		enum
		{
			kContext_Gameplay = 0,
			kContext_BaseMenuControl = 1,
			kContext_StickMenuContol = 2,
			kContext_LoadingMenu = 0x3,
			kContext_Cusror,
			kContext_Cursor2,
			kContext_Console = 6,
			kContext_KeyboardBookMenu = 8,
			kContext_ControllerBookMenu,
			kContext_LockpickingMenu = 0xC,
			kContext_VATSMenu = 0xD,
			kContext_WorkshopMenu = 0x10,
			kContext_SitWaitMenu = 0x12,
			kContextCount = (0x108 / 8) // 33
		};

		struct InputContext
		{
			// 18
			struct Mapping
			{
				BSFixedString	name;		// 00
				UInt32			buttonID;	// 08
				UInt32			sortIndex;	// 0C
				UInt32			unk10;		// 10
				UInt32			unk14;		// 14
			};

			tArray<Mapping>	keyboardMap;
			tArray<Mapping>	mouseMap;
			tArray<Mapping>	gamepadMap;
		};

		void		* unk00;					// 000
		InputContext * context[kContextCount];	// 008
		tArray<UInt32>	unk110;					// 110
		tArray<InputContext::Mapping>	unk128;	// 128
		UInt8			allowTextInput;			// 140
		UInt8			unk141;					// 141
		UInt8			unk142;					// 142
		UInt8			unk143;					// 143
		UInt32			unk144;					// 144

		UInt8			AllowTextInput(bool allow);
		UInt32			GetMappedKey(BSFixedString name, UInt32 deviceType, UInt32 contextIdx);
		BSFixedString	GetMappedControl(UInt32 buttonID, UInt32 deviceType, UInt32 contextIdx);

		DEFINE_MEMBER_FUNCTION(EnableControlContext, void, 0x1B28000, UInt32 context);
		DEFINE_MEMBER_FUNCTION(DisableControlContext, void, 0x1B280F0, UInt32 context);
	};
	STATIC_ASSERT(offsetof(InputManager, unk110) == 0x110);
	STATIC_ASSERT(offsetof(InputManager, unk128) == 0x128);
	STATIC_ASSERT(offsetof(InputManager, allowTextInput) == 0x140);

	extern RelocPtr <InputManager*> g_inputMgr;


	// EF0
	class InputEventTable
	{
	public:
		UInt64				unk00[0xED8 >> 3];
		SimpleLock			inputIDLock;
		UInt64				unkEE0[2];
		UInt32				inputID;
		//ButtonEvent			buttonEvents[30];
		//CharacterEvent		characterEvents[15];
		//MouseMoveEvent		mouseMoveEvents[3];
		//CursorMoveEvent		cursorMoveEvents[3];
		//ThumbstickEvent		thumbstickEvents[6];
		//DeviceConnectEvent	deviceConnectEvents[3];
		//KinectEvent			kinectEvents[3];
	};
	STATIC_ASSERT(offsetof(InputEventTable, inputID) == 0xEF0);
	//STATIC_ASSERT(sizeof(InputEventTable) == 0xEF0);
	extern RelocPtr <InputEventTable*> g_inputEventTable;

	class PlayerInputHandler : public BSInputEventUser
	{
	public:
		virtual ~PlayerInputHandler();
	};
	//0x260??
	class PlayerControls : public BSInputEventReceiver
	{
	public:

		enum
		{
			kControl_Movement,
			kControl_Look,
			kControl_Sprint,
			kControl_ReadyWeapon,
			kControl_AutoMove,
			kControl_ToggleRun,
			kControl_Activate,
			kControl_Jump,
			kControl_AttackBlock,
			kControl_Run,
			kControl_Sneak,
			kControl_TogglePOV,
			kControl_MeleeThrow,
			kControl_GrabRotation,
			kControl_Total = 0xE
		};
		virtual ~PlayerControls(); // Executes receiving function
		//void	** vtbl; //02D721D0
		SInt32								unk08;	// 08 - Negative 1 is special case
		UInt32								unk0C;	// 0C
		BSTEventSink<MenuOpenCloseEvent>	menuOpenCloseEventSink;	// 10
		BSTEventSink<MenuModeChangeEvent>	menuModeChangeEventSink; // 18
		BSTEventSink<TESFurnitureEvent>		furnitureEventSink; // 20
		BSTEventSink<UserEventEnabledEvent>	userEventEnabledEventSink; // 28
		BSTEventSink<void*>					movementPlayerControls; // 30
		BSTEventSink<void*>					quickContainerStateEventSink; // 38
		UInt64								unk40[(0x70 - 0x40) >> 3]; //0x88
		UInt32								unk70;
		bool								enabled;	// 74
		UInt64								unk78[(0x88 - 0x78) >> 3];
		UInt8								autoRun;		// 88
		tArray<PlayerInputHandler*>			handlers;		// 90
		tArray<PlayerInputHandler*>			unkA8;		// A8
		UInt64								unkC0[(0x1C0 - 0xC0) >> 3];
		tArray<void*>						unk1C0;		// A8
		UInt32								unk1D8;
		UInt32								unk1DC;
		PlayerInputHandler					* controlHandlers[kControl_Total];

		DEFINE_MEMBER_FUNCTION(DisableControl, void, 0xF43000);
		DEFINE_MEMBER_FUNCTION(ForceRelease, void, 0xF43080);
		DEFINE_MEMBER_FUNCTION(ResetControl, void, 0xF43230, float, float);
		DEFINE_MEMBER_FUNCTION(CanProcessUerEvent, bool, 0xF44010);
	};
	STATIC_ASSERT(offsetof(PlayerControls, enabled) == 0x74);
	STATIC_ASSERT(offsetof(PlayerControls, autoRun) == 0x88);
	STATIC_ASSERT(offsetof(PlayerControls, handlers) == 0x90);
	STATIC_ASSERT(offsetof(PlayerControls, unkA8) == 0xA8);
	STATIC_ASSERT(offsetof(PlayerControls, unk1C0) == 0x1C0);
	//5A12268
	extern RelocPtr <PlayerControls*> g_playerControls;

	class InputLayerManager
	{
	public:
		enum InputLayer1 
		{
			kInputLayer_Movement = (1 << 0),
			kInputLayer_Looking = (1 << 1),
			kInputLayer_Activate = (1 << 2),
			kInputLayer_Menu = (1 << 3),
			kInputLayer_Console = (1 << 4),
			kInputLayer_POVSwitch = (1 << 5),
			kInputLayer_Fighting = (1 << 6),
			kInputLayer_Sneaking = (1 << 7),
			kInputLayer_MainFourMenu = (1 << 8),
			kInputLayer_WheelZoom = (1 << 9),
			kInputLayer_Jumping = (1 << 10),
			kInputLayer_VATS = (1 << 11)
		};

		enum InputLayer2 
		{
			kInputLayer_JournalTabs = (1 << 0),
			kInputLayer_Activation = (1 << 1),
			kInputLayer_FastTravel = (1 << 2),
			kInputLayer_POVChange = (1 << 3),
			kInputLayer_VATS_2 = (1 << 4),
			kInputLayer_Favorites = (1 << 5),
			kInputLayer_PipboyLight = (1 << 6),
			kInputLayer_ZKey = (1 << 7),
			kInputLayer_Running = (1 << 8),
			kInputLayer_Cursor = (1 << 9),
			kInputLayer_Sprinting = (1 << 10)
		};
		struct InputLayer
		{
			UInt32			controlState;

			DEFINE_MEMBER_FUNCTION(IsInUse, bool, 0x1B21EC0);
		};

		DEFINE_MEMBER_FUNCTION(GenerateInputLayer, void, 0x1B204D0, InputLayer*&, const char * name);
		DEFINE_MEMBER_FUNCTION(SetControlState1, void, 0x1B21900, UInt32 controlState, UInt32 modifiedState, bool enabled, UInt32 priority);
		DEFINE_MEMBER_FUNCTION(SetControlState2, void, 0x1B21A20, UInt32 controlState, UInt32 modifiedState, bool enabled, UInt32 priority);
	};
	using InputLayer = InputLayerManager::InputLayer;
	extern RelocPtr <InputLayerManager *> g_inputLayerMgr;


	class MenuTopicManager : public BSTEventSink<MenuOpenCloseEvent>
	{
	public:
		virtual ~MenuTopicManager();

		BSTEventSink<void*>       positionPlayerEventSink;		// 08
		UInt32					  unk10;						// 10
		UInt32					  refHandle;					// 14

		DEFINE_MEMBER_FUNCTION(IsDialogueMenuDisabled, bool, 0xCA26A0);
	};
	extern RelocPtr <MenuTopicManager *> g_menuTopicMgr; //5906BB0


	class JobListManager
	{
	public:

		enum
		{
			kState_Paused = 0,
			kState_Running,
			kState_Loading
		};
		class ServingThread : public BSThread
		{
		public:
			UInt64			unk50[(0x68 - 0x50) >> 3]; //singleton
			UInt32			gameState;
		};
	};
	STATIC_ASSERT(offsetof(JobListManager::ServingThread, gameState) == 0x68);
	extern RelocPtr<JobListManager::ServingThread*>	g_servingThread;

	class ItemMenuDataManager
	{
	public:

	};

	extern RelocPtr <ItemMenuDataManager *> g_itemMenuDataMgr; //0x590CA00
}

