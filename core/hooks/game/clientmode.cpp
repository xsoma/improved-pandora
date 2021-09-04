#include "../../../includes.h"

bool Hooks::ShouldDrawParticles()
{
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >(IClientMode::SHOULDDRAWPARTICLES)(this);
}

bool Hooks::ShouldDrawFog()
{
	if (g_cfg[XOR("visuals_misc_remove_fog")].get<bool>())
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >(IClientMode::SHOULDDRAWFOG)(this);
}

void Hooks::OverrideView(CViewSetup* view) {
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());

	g_visuals.ThirdpersonThink();

	if (g_cl.m_processing)
		if (g_hvh.m_fake_duck)
			view->m_origin.z = g_cl.m_local->GetAbsOrigin().z + 64.f;

	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >(IClientMode::OVERRIDEVIEW)(this, view);

	if (g_cl.m_local)
		game::SetPostProcess(false);
}

bool Hooks::CreateMove(float time, CUserCmd* cmd)
{
	Stack stack;
	bool ret;

	ret = g_hooks.m_client_mode.GetOldMethod< CreateMove_t >(IClientMode::CREATEMOVE)(this, time, cmd);

	if (!cmd || !cmd->m_command_number)
		return ret;

	if (ret)
		g_csgo.m_engine->SetViewAngles(cmd->m_view_angles);

	cmd->m_random_seed = g_csgo.MD5_PseudoRandom(cmd->m_command_number) & 0x7fffffff;

	g_cl.m_packet = stack.next().local(0x1c).as< bool* >();
	*g_cl.m_packet = true;

	g_cl.m_final_packet = stack.next().local(0x1b).as< bool* >();

	if (GUI::ctx->open)
	{
		if (cmd->m_buttons & IN_ATTACK)
			cmd->m_buttons &= ~IN_ATTACK;

		if (cmd->m_buttons & IN_ATTACK2)
			cmd->m_buttons &= ~IN_ATTACK2;
	}

	if (g_cl.m_local && !g_cl.m_local->alive())
		g_inputpred.m_stored_variables.m_curtime = g_csgo.m_globals->m_curtime;

	if (g_tickbase.m_shift_data.m_should_attempt_shift && !g_tickbase.m_shift_data.m_should_disable)
	{
		if (g_cfg[XOR("rage_exploit_charged")].get<bool>() && g_cl.m_goal_shift == 7 && (g_tickbase.m_shift_data.m_prepare_recharge || g_tickbase.m_shift_data.m_did_shift_before && !g_tickbase.m_shift_data.m_should_be_ready))
		{
			if (cmd->m_buttons & IN_ATTACK)
				cmd->m_buttons &= ~IN_ATTACK;
		}
	}

	g_cl.OnTick(cmd);

	if (cmd->m_command_number >= g_cl.m_shot_command_number && g_cl.m_shot_command_number >= cmd->m_command_number - g_csgo.m_cl->iChokedCommands)
		g_cl.m_angle = g_csgo.m_input->m_commands[g_cl.m_shot_command_number % 150].m_view_angles;
	else
		g_cl.m_angle = cmd->m_view_angles;

	if (g_tickbase.m_shift_data.m_should_attempt_shift && !g_tickbase.m_shift_data.m_should_disable)
	{
		if (g_cfg[XOR("rage_exploit_charged")].get<bool>() && g_cl.m_goal_shift == 7 && (g_tickbase.m_shift_data.m_prepare_recharge || g_tickbase.m_shift_data.m_did_shift_before && !g_tickbase.m_shift_data.m_should_be_ready))
		{
			if (cmd->m_buttons & IN_ATTACK)
				cmd->m_buttons &= ~IN_ATTACK;
		}
	}

	g_aimbot.m_stop = false;
	g_aimbot.m_slow_motion_slowwalk = false;
	g_aimbot.m_slow_motion_fakewalk = false;

	g_cl.m_animate = true;
	g_cl.m_update_fake = true;

	return false;
}

bool Hooks::DoPostScreenSpaceEffects(CViewSetup* setup)
{
	g_visuals.RenderGlow();

	g_cl.m_in_glow = true;
	auto result = g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >(IClientMode::DOPOSTSPACESCREENEFFECTS)(this, setup);
	g_cl.m_in_glow = false;

	return result;
}