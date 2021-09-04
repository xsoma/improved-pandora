#include "../../../includes.h"

CMatchSessionOnlineHost* Hooks::GetMatchSession( )
{
	static const Address BYieldingRunJobFromMsg{ pattern::find( g_csgo.m_client_dll, XOR( "8B D8 85 DB 0F 84 ? ? ? ? 8B 13 8B CB 68 ? ? ? ? 68" ) ) };

	return g_hooks.m_match_framework.GetOldMethod< GetMatchSession_t >( CMatchFramework::GETMATCHSESSION )( this );
}