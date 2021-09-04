#include "../../../includes.h"

bool Hooks::InPrediction()
{
	Stack stack;
	ang_t* angles;

	static const Address CalcPlayerView_ret1{ pattern::find(g_csgo.m_client_dll, XOR("84 C0 75 0B 8B 0D ? ? ? ? 8B 01 FF 50 4C")) };
	static const Address CalcPlayerView_ret2{ pattern::find(g_csgo.m_client_dll, XOR("84 C0 75 08 57 8B CE E8 ? ? ? ? 8B 06")) };

	static const Address MaintainSequenceTransitionsAddr{ pattern::find(g_csgo.m_client_dll, XOR("84 C0 74 17 8B 87")) };

	if (stack.ReturnAddress() == MaintainSequenceTransitionsAddr)
		return false;

	if (g_cl.m_local && g_cfg[XOR("visuals_misc_remove_visual_recoil")].get<bool>())
	{
		if (stack.ReturnAddress() == CalcPlayerView_ret1)
			return true;

		if (stack.ReturnAddress() == CalcPlayerView_ret2)
		{
			angles = stack.next().arg(0xC).to< ang_t* >();

			if (angles)
			{
				*angles -= g_cl.m_local->m_viewPunchAngle()
					+ (g_cl.m_local->m_aimPunchAngle() * g_csgo.weapon_recoil_scale->GetFloat())
					* g_csgo.view_recoil_tracking->GetFloat();
			}

			return true;
		}
	}

	return g_hooks.m_prediction.GetOldMethod< InPrediction_t >(CPrediction::INPREDICTION)(this);
}

void Hooks::RunCommand(Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper)
{
	if (!ent || ent->index() != g_csgo.m_engine->GetLocalPlayer())
		return g_hooks.m_prediction.GetOldMethod< RunCommand_t >(CPrediction::RUNCOMMAND)(this, ent, cmd, movehelper);

	if (cmd->m_tick >= (g_cl.m_tick + int(1 / g_csgo.m_globals->m_interval) + 8))
	{
		cmd->m_predicted = true;
		return;
	}

	Player* player = (Player*)ent;

	int backup_tickbase = player->m_nTickBase();
	float backup_curtime = g_csgo.m_globals->m_curtime;

	if (cmd->m_command_number == g_tickbase.m_prediction.m_shifted_command)
	{
		player->m_nTickBase() = (g_tickbase.m_prediction.m_original_tickbase - g_tickbase.m_prediction.m_shifted_ticks + 1);
		++player->m_nTickBase();

		g_csgo.m_globals->m_curtime = game::TICKS_TO_TIME(player->m_nTickBase());
	}

	float m_flVelModBackup = player->m_flVelocityModifier();
	if (g_cl.m_update && cmd->m_command_number == g_csgo.m_cl->nLastCommandAck + 1)
		player->m_flVelocityModifier() = g_inputpred.m_stored_variables.m_flVelocityModifier;

	g_hooks.m_prediction.GetOldMethod< RunCommand_t >(CPrediction::RUNCOMMAND)(this, ent, cmd, movehelper);

	if (!g_cl.m_update)
		player->m_flVelocityModifier() = m_flVelModBackup;

	if (cmd->m_command_number == g_tickbase.m_prediction.m_shifted_command)
	{
		player->m_nTickBase() = backup_tickbase;

		g_csgo.m_globals->m_curtime = backup_curtime;
	}

	g_netdata.store_netvars();

	g_inputpred.UpdateViewmodelData();
}

void Hooks::SetupMove(Player* ent, CUserCmd* cmd, IMoveHelper* movehelper, CMoveData* movedata)
{
	if (!g_csgo.m_engine->IsInGame())
		return g_hooks.m_prediction.GetOldMethod<SetupMove_t>(CPrediction::SETUPMOVE)(this, ent, cmd, movehelper, movedata);

	g_hooks.m_prediction.GetOldMethod<SetupMove_t>(CPrediction::SETUPMOVE)(this, ent, cmd, movehelper, movedata);

	if (!ent || ent->index() != g_csgo.m_engine->GetLocalPlayer())
		return;
}