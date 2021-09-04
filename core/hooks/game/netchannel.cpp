#include "../../../includes.h"

int Hooks::SendDatagram(void* data)
{
	if (!g_csgo.m_net)
		return g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >(INetChannel::SENDDATAGRAM)(this, data);

	int backup1 = g_csgo.m_net->m_in_rel_state;
	int backup2 = g_csgo.m_net->m_in_seq;

	if (g_cfg[XOR("rage_extended_backtrack")].get<bool>())
	{
		float correct = std::max(0.f, (200.f / 1000.f) - g_cl.m_latency - g_cl.m_lerp);

		g_csgo.m_net->m_in_seq += 2 * (64 - 3) - static_cast<uint32_t>((64 - 3) * correct);
	}

	int ret = g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >(INetChannel::SENDDATAGRAM)(this, data);

	g_csgo.m_net->m_in_rel_state = backup1;
	g_csgo.m_net->m_in_seq = backup2;

	return ret;
}

bool Hooks::SendNetMsg(INetMessage& msg, bool bForceReliable, bool bVoice)
{
	if (!g_csgo.m_net)
		return g_hooks.m_net_channel.GetOldMethod<SendNetMsg_t>(INetChannel::SENDNETMSG)(this, msg, bForceReliable, bVoice);

	if (g_cfg[XOR("misc_bypass_sv_pure")].get<bool>())
		if (msg.GetType() == 14)
			return false;

	if (msg.GetGroup() == 9)
		bVoice = true;

	return g_hooks.m_net_channel.GetOldMethod<SendNetMsg_t>(INetChannel::SENDNETMSG)(this, msg, bForceReliable, bVoice);
}

void Hooks::ProcessPacket(void* packet, bool header)
{
	if (!g_csgo.m_net)
		return g_hooks.m_net_channel.GetOldMethod< ProcessPacket_t >(INetChannel::PROCESSPACKET)(this, packet, header);

	g_hooks.m_net_channel.GetOldMethod< ProcessPacket_t >(INetChannel::PROCESSPACKET)(this, packet, header);

	g_cl.UpdateIncomingSequences();

	for (CEventInfo* it{ g_csgo.m_cl->m_events }; it != nullptr; it = it->m_next)
	{
		if (!it->m_class_id)
			continue;

		it->m_fire_delay = 0.f;
	}

	g_csgo.m_engine->FireEvents();
}