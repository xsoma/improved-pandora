#include "../../../includes.h"

void Hooks::LevelInitPreEntity(const char* map)
{
	float rate{1.f / g_csgo.m_globals->m_interval};

	g_csgo.cl_updaterate->SetValue(rate);
	g_csgo.cl_cmdrate->SetValue(rate);

	g_aimbot.reset();
	g_visuals.m_hit_start = g_visuals.m_hit_end = g_visuals.m_hit_duration = 0.f;

	if (g_cl.m_fake_state)
		g_cl.m_fake_state = nullptr;

	g_hooks.m_client.GetOldMethod<LevelInitPreEntity_t>(CHLClient::LEVELINITPREENTITY)(this, map);
}

void Hooks::LevelInitPostEntity()
{
	g_cl.OnMapload();

	g_hooks.m_client.GetOldMethod<LevelInitPostEntity_t>(CHLClient::LEVELINITPOSTENTITY)(this);
}

void Hooks::LevelShutdown()
{
	g_cl.m_local = nullptr;
	g_cl.m_weapon = nullptr;
	g_cl.m_processing = false;
	g_cl.m_weapon_info = nullptr;
	g_cl.m_round_end = false;

	g_aimbot.reset();

	g_cl.m_sequences.clear();

	g_hooks.m_client.GetOldMethod<LevelShutdown_t>(CHLClient::LEVELSHUTDOWN)(this);
}

void Hooks::FrameStageNotify(Stage_t stage)
{
	if (!g_csgo.m_engine->IsInGame())
		return g_hooks.m_client.GetOldMethod< FrameStageNotify_t >(CHLClient::FRAMESTAGENOTIFY)(this, stage);

	if (stage != FRAME_START)
		g_cl.m_stage = stage;

	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());

	if (stage == FRAME_RENDER_START)
	{
		g_cl.UpdateAnimations();

		g_anims.UpdateLocalAnimations();

		g_anims.UpdateFakeAnimations();

		g_visuals.DrawBeams();

		g_shots.OnFrameStage();
	}

	g_skins.AgentChanger(stage);

	static int m_iLastCmdAck = 0;
	static float m_flNextCmdTime = 0.f;

	if (g_cl.m_local)
	{
		int framstage_minus2 = stage - 2;

		if (framstage_minus2)
		{

		}
		else
		{
			if (g_csgo.m_cl && (m_iLastCmdAck != g_csgo.m_cl->nLastCommandAck || m_flNextCmdTime != g_csgo.m_cl->flNextCmdTime))
			{
				if (g_inputpred.m_stored_variables.m_flVelocityModifier != g_cl.m_local->get< float >(0xA38C))
				{
					*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(g_csgo.m_prediction + 0x24)) = 1;
					g_inputpred.m_stored_variables.m_flVelocityModifier = g_cl.m_local->get< float >(0xA38C);
				}

				m_iLastCmdAck = g_csgo.m_cl->nLastCommandAck;
				m_flNextCmdTime = g_csgo.m_cl->flNextCmdTime;
			}
		}
	}

	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >(CHLClient::FRAMESTAGENOTIFY)(this, stage);

	if (stage == FRAME_RENDER_START)
	{
		g_cl.m_rate = (int)std::round(1.f / g_csgo.m_globals->m_interval);

		static bool needs_full_update = false;

		if (g_cl.m_local)
		{
			if (!g_cl.m_local->alive())
			{
				needs_full_update = true;
			}
			else
			{
				if (needs_full_update)
				{
					g_skins.m_update = true;
					needs_full_update = false;
				}
			}
		}
	}
	else if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		g_inputpred.CorrectViewmodelData();

		g_cl.ClanTag();

		g_skins.think();
	}
	else if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END)
	{
		g_visuals.NoSmoke();
	}
	else if (stage == FRAME_NET_UPDATE_END)
	{
		g_netdata.restore_netvars();

		for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
			Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

			if (!player || player->m_bIsLocalPlayer())
				continue;

			AimPlayer* data = &g_aimbot.m_players[i - 1];
			data->OnNetUpdate(player);

			auto hViewModel = player->m_hViewModel();

			if (hViewModel != 0xFFFFFFFF)
			{
				auto pViewModel = g_csgo.m_entlist->GetClientEntityFromHandle<Entity*>(hViewModel);

				if (pViewModel)
				{
					game::UpdateAllViewmodelAddons(pViewModel);
				}
			}
		}

		g_lagcompensation.PostPlayerUpdate();
	}
}

bool __cdecl Detours::ReportHit(Hit_t* info)
{
	if (info)
	{
		WorldHitmarkerData_t data;

		data.m_alpha = 1.f;
		data.m_time = g_inputpred.m_stored_variables.m_curtime;

		data.m_pos_x = info->m_pos_x;
		data.m_pos_y = info->m_pos_y;
		data.m_pos_z = info->m_pos_z;

		g_visuals.m_world_hitmarkers.push_back(data);
	}

	return g_detours.oReportHit(info);
}

void __cdecl Detours::CL_Move(float accumulated_extra_samples, bool bFinalTick)
{
	if (!g_hvh.m_fake_duck)
	{
		if (g_cl.m_processing && g_tickbase.m_shift_data.m_should_attempt_shift && g_tickbase.m_shift_data.m_needs_recharge)
		{
			--g_tickbase.m_shift_data.m_needs_recharge;

			if (g_tickbase.m_shift_data.m_needs_recharge == 0)
				g_tickbase.m_shift_data.m_should_be_ready = true;

			return;
		}
	}

	return g_detours.oCL_Move(accumulated_extra_samples, bFinalTick);
}