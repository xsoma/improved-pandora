#include "prediction_system.h"

c_prediction_system g_inputpred{ };;

void c_prediction_system::update()
{
	bool valid{ g_csgo.m_cl->iDeltaTick > 0 };

	if (m_stored_variables.m_flVelocityModifier < 1.0)
		*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(g_csgo.m_prediction + 0x24)) = 1;

	if (g_cl.m_stage == FRAME_NET_UPDATE_END)
	{
		int start = g_csgo.m_cl->nLastCommandAck;
		int stop = g_csgo.m_cl->nLastOutgoingCommand + g_csgo.m_cl->iChokedCommands;

		g_csgo.m_prediction->Update(g_csgo.m_cl->iDeltaTick, valid, start, stop);
	}
}

void c_prediction_system::predict()
{
	if (!g_cl.m_local->alive())
		return;

	g_csgo.m_prediction->m_in_prediction = true;

	g_csgo.m_pPredictionPlayer = g_cl.m_local;
	*g_csgo.m_nPredictionRandomSeed = g_cl.m_cmd->m_random_seed;

	m_stored_variables.m_curtime = g_csgo.m_globals->m_curtime;
	m_stored_variables.m_frametime = g_csgo.m_globals->m_frametime;

	g_csgo.m_globals->m_curtime = game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase());
	g_csgo.m_globals->m_frametime = g_csgo.m_prediction->m_engine_paused ? 0.0f : g_csgo.m_globals->m_interval;

	g_csgo.m_move_helper->SetHost(g_cl.m_local);
	g_csgo.m_game_movement->StartTrackPredictionErrors(g_cl.m_local);

	CMoveData m_move_data;
	memset(&m_move_data, 0, sizeof(CMoveData));

	g_csgo.m_prediction->SetupMove(g_cl.m_local, g_cl.m_cmd, g_csgo.m_move_helper, &m_move_data);
	g_csgo.m_game_movement->ProcessMovement(g_cl.m_local, &m_move_data);
	g_csgo.m_prediction->FinishMove(g_cl.m_local, g_cl.m_cmd, &m_move_data);

	g_csgo.m_game_movement->FinishTrackPredictionErrors(g_cl.m_local);
	g_csgo.m_move_helper->SetHost(nullptr);
}

void c_prediction_system::restore()
{
	if (!g_cl.m_cmd || !g_cl.m_local)
		return;

	g_csgo.m_prediction->m_in_prediction = false;

	*g_csgo.m_nPredictionRandomSeed = -1;
	g_csgo.m_pPredictionPlayer = nullptr;

	g_csgo.m_globals->m_curtime = m_stored_variables.m_curtime;
	g_csgo.m_globals->m_frametime = m_stored_variables.m_frametime;
}

void c_prediction_system::CorrectViewmodelData()
{
	if (!g_cl.m_processing)
		return;

	if (g_cl.m_local->m_hViewModel() != 0xFFFFFFFF)
	{
		Entity* const pViewModel = g_csgo.m_entlist->GetClientEntityFromHandle(g_cl.m_local->m_hViewModel());

		if (!pViewModel)
			return;

		pViewModel->m_flAnimTime() = StoredViewmodel.m_flViewmodelAnimTime;
		pViewModel->m_flCycle() = StoredViewmodel.m_flViewmodelCycle;
	}
}

void c_prediction_system::UpdateViewmodelData()
{
	if (!g_cl.m_processing)
		return;

	if (g_cl.m_local->m_hViewModel() != 0xFFFFFFFF)
	{
		Entity* const pViewModel = g_csgo.m_entlist->GetClientEntityFromHandle(g_cl.m_local->m_hViewModel());

		if (!pViewModel)
			return;

		StoredViewmodel.m_flViewmodelCycle = pViewModel->m_flCycle();
		StoredViewmodel.m_flViewmodelAnimTime = pViewModel->m_flAnimTime();
	}
}