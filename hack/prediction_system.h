#pragma once
#include "../includes.h"

class c_prediction_system
{
public:
	struct Variables_t
	{
		float m_frametime;
		float m_curtime;
		float m_flVelocityModifier;

		vec3_t m_vecVelocity;
		vec3_t m_vecOrigin;
		int m_fFlags;
	} m_stored_variables;

	struct Misc_t
	{
		int m_iRunCommandTickbase;
		bool m_bOverrideModifier;
	} Misc;

	struct viewmodel_t
	{
		float m_flViewmodelCycle;
		float m_flViewmodelAnimTime;
	} StoredViewmodel;

	struct latency_calc_t
	{
		int command_num = -1;
		int m_tick = 0;
	};

	latency_calc_t m_tick_history[150] = {};
public:
	void update();
	void predict();
	void restore();

	void UpdateViewmodelData();
	void CorrectViewmodelData();
};

extern c_prediction_system g_inputpred;