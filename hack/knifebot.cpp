#include "../includes.h"

void Aimbot::knife()
{
	struct KnifeTarget_t {bool stab; ang_t angle; LagComp::LagRecord_t* record;};
	KnifeTarget_t target{};

	if (m_targets.empty())
		return;

	for (const auto& t : m_targets)
	{
		if (!t->m_player)
			continue;

		const auto best = g_lagcompensation.GetLatestRecord(t->m_player);

		if (!best.has_value())
			continue;

		for (const auto& a : m_knife_ang)
		{
			if (!CanKnife(best.value(), a, target.stab))
				continue;

			target.angle = a;
			target.record = best.value();
			break;
		}

		const auto last = g_lagcompensation.GetOldestRecord(t->m_player);

		if (!last.has_value() || last.value() == best.value())
			continue;

		for (const auto& a : m_knife_ang)
		{
			if (!CanKnife(last.value(), a, target.stab))
				continue;

			target.angle = a;
			target.record = last.value();
			break;
		}

		if (target.record)
			break;
	}

	if (target.record)
	{
		g_cl.m_cmd->m_tick = game::TIME_TO_TICKS(target.record->m_flSimulationTime + g_cl.m_lerp);

		g_cl.m_cmd->m_view_angles = target.angle;

		g_cl.m_cmd->m_buttons |= target.stab ? IN_ATTACK2 : IN_ATTACK;

		*g_cl.m_packet = false;
	}
}

bool Aimbot::CanKnife(LagComp::LagRecord_t* record, ang_t angle, bool& stab)
{
	vec3_t forward;
	math::AngleVectors(angle, &forward);

	CGameTrace trace;
	KnifeTrace(forward, false, &trace);

	if (!g_cl.m_weapon || !trace.m_entity || trace.m_entity != record->m_pEntity)
		return false;

	bool armor = record->m_pEntity->m_ArmorValue() > 0;
	bool first = g_cl.m_weapon->m_flNextPrimaryAttack() + 0.4f < g_csgo.m_globals->m_curtime;
	bool back = KnifeIsBehind(record);

	int stab_dmg = m_knife_dmg.stab[armor][back];
	int slash_dmg = m_knife_dmg.swing[first][armor][back];
	int swing_dmg = m_knife_dmg.swing[false][armor][back];

	int health = record->m_pEntity->m_iHealth();

	if (health <= slash_dmg)
		stab = false;
	else if (health <= stab_dmg)
		stab = true;
	else if (health > (slash_dmg + swing_dmg + stab_dmg))
		stab = true;
	else
		stab = false;

	if (stab && !KnifeTrace(forward, true, &trace))
		return false;

	return true;
}

bool Aimbot::KnifeTrace(vec3_t dir, bool stab, CGameTrace* trace)
{
	float range = stab ? 32.f : 48.f;

	vec3_t start = g_cl.m_shoot_pos;
	vec3_t end = start + (dir * range);

	CTraceFilterSimple filter;
	filter.SetPassEntity(g_cl.m_local);
	g_csgo.m_engine_trace->TraceRay(Ray(start, end), MASK_SOLID, &filter, trace);

	if (trace->m_fraction >= 1.f)
	{
		g_csgo.m_engine_trace->TraceRay(Ray(start, end, {-16.f, -16.f, -18.f}, {16.f, 16.f, 18.f}), MASK_SOLID, &filter, trace);
		return trace->m_fraction < 1.f;
	}

	return true;
}

bool Aimbot::KnifeIsBehind(LagComp::LagRecord_t* record)
{
	vec3_t delta{record->m_vecOrigin - g_cl.m_shoot_pos};
	delta.z = 0.f;
	delta.normalize();

	vec3_t target;
	math::AngleVectors(record->m_angAbsAngles, &target);
	target.z = 0.f;

	return delta.dot(target) > 0.475f;
}