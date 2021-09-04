#include "../../../includes.h"

bool Hooks::IsConnected()
{
	if (!this || !g_csgo.m_engine)
		return false;

	Stack stack;

	static const Address IsLoadoutAllowed{ pattern::find(g_csgo.m_client_dll, XOR("84 C0 75 04 B0 01 5F")) };

	if (g_cfg[XOR("misc_unlock_inventory")].get<bool>() && stack.ReturnAddress() == IsLoadoutAllowed)
		return false;

	return g_hooks.m_engine.GetOldMethod< IsConnected_t >(IVEngineClient::ISCONNECTED)(this);
}

void Hooks::FireEvents()
{
	g_hooks.m_engine.GetOldMethod< IsConnected_t >(IVEngineClient::FIREEVENTS)(this);
}

bool Hooks::IsHLTV()
{
	if (!this || !g_csgo.m_engine)
		return false;

	Stack stack;

	static const Address SetupVelocity{ pattern::find(g_csgo.m_client_dll, XOR("84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0")) };

	if (g_anims.m_bEnablePVS)
		return true;

	if (stack.ReturnAddress() == SetupVelocity)
		return true;

	return g_hooks.m_engine.GetOldMethod< IsHLTV_t >(IVEngineClient::ISHLTV)(this);
}

bool Hooks::IsPaused() 
{
	static DWORD* return_to_extrapolation = (DWORD*)(pattern::find(g_csgo.m_client_dll,
		XOR("FF D0 A1 ?? ?? ?? ?? B9 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? FF 50 34 85 C0 74 22 8B 0D ?? ?? ?? ??")) + 0x29);

	if (_ReturnAddress() == (void*)return_to_extrapolation)
		return true;

	return g_hooks.m_engine.GetOldMethod< IsPaused_t >(IVEngineClient::ISPAUSED)(this);
}