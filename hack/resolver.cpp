#include "../includes.h"

Resolver g_resolver{ };;

float AngleDiff(float destAngle, float srcAngle) 
{
	float delta;

	delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

/*void Resolver::DetectSide(LagComp::LagRecord_t* record)
{
	vec3_t current;
	float right_two, left_two;
	CGameTrace tr;
	Ray ray, ray2, ray3;
	CTraceFilter filter;

	vec3_t right(record->m_pEntity->m_angEyeAngles().x, record->m_pEntity->m_angEyeAngles().y + record->m_pEntity->GetMaxBodyRotation(), record->m_pEntity->m_angEyeAngles().z);
	vec3_t left(record->m_pEntity->m_angEyeAngles().x, record->m_pEntity->m_angEyeAngles().y - record->m_pEntity->GetMaxBodyRotation(), record->m_pEntity->m_angEyeAngles().z);
	current = record->m_pEntity->m_angEyeAngles();

	filter.skip_entity = record->m_pEntity;
	ray.init(current, right);
	ray2.init(current, left);
	bool failed = false;
	float right_one, left_one;

	right_one = current.y - right.y;
	left_one = current.y - left.y;

	g_csgo.m_engine_trace->TraceRay(ray, MASK_SHOT | CONTENTS_HITBOX, &filter, &tr);
	right_two = tr.m_endpos.length_2d() - tr.m_startpos.length_2d();

	g_csgo.m_engine_trace->TraceRay(ray2, MASK_SHOT | CONTENTS_HITBOX, &filter, &tr);
	left_two = tr.m_endpos.length_2d() - tr.m_startpos.length_2d();

	// if extending we have an easier time finding their real
	if (record->m_pLayers[3].m_cycle == 0.f && (record->m_pLayers[3].m_weight == 0.f))
	{
		if (fabs(right_one) >= 58.f)		
			side = 1;
		else if (fabs(left_one) >= 58.f)
			side = -1;
		if (fabs(right_one) <= 58.f / 2)
			side = 1;
		else if (fabs(left_one) <= 58.f / 2)
			side = -1;

	}
	// else we use tracing
	else
	{
		if (fabs(right_two) >= 58.f)
			side = 1;
		else if (fabs(left_two) >= 58.f)
			side = -1;

		if (fabs(right_one) <= 58.f / 2)
			side = 1;
		else if (fabs(left_one) <= 58.f / 2)
			side = -1;
	}
}*/

void Resolver::ResolveAngles(Player* player, LagComp::LagRecord_t* record)
{
	AimPlayer* data = &g_aimbot.m_players[record->m_pEntity->index() - 1];

	if (!record->m_bDidShot)
	{
		if (g_cfg[XOR("cheat_mode")].get<int>() == 1)
		{
			switch (data->m_missed_shots % 4) 
			{
			case 0:
			case 1:
				record->m_angEyeAngles.x = record->m_pEntity->m_angEyeAngles().x = 90.f;
				break;
			case 2:
			case 3:
				record->m_angEyeAngles.x = record->m_pEntity->m_angEyeAngles().x = -90.f;
				break;
			}
		}

		ResolveStand(data, record);
	}
}

void Resolver::ResolveStand(AimPlayer* data, LagComp::LagRecord_t* record)
{
	float max_rotation = record->m_pEntity->GetMaxBodyRotation();

	if (!record->m_pState)
		return;

	const auto info = g_anims.GetAnimationInfo(record->m_pEntity);

	if (!info)
		return;

	float goal_feet_yaw = record->m_pState->m_flGoalFeetYaw;
	float middle = record->m_pState->m_flEyeYaw;

	info->m_flBrute = goal_feet_yaw; // set the angle \\

	float eye_feet_delta = AngleDiff(record->m_pState->m_flEyeYaw, goal_feet_yaw);

	if (eye_feet_delta <= 57)
	{
		if (eye_feet_delta > -57)
			record->m_pState->m_flGoalFeetYaw = middle; // LBY based desync fix
		else if (eye_feet_delta == 0.f) // no desync || real on side with fake middle angle.
		{
			const auto slow_walk = record->m_pState->m_flFeetYawRate >= 0.01f && record->m_pState->m_flFeetYawRate <= 0.8f;

			if (slow_walk) // if player is able to use desync //
			{
				if (goal_feet_yaw != record->m_pState->m_flCurrentFeetYaw) // set previos server goal feet angle -> REAL Choked Angle //
					goal_feet_yaw = record->m_pState->m_flCurrentFeetYaw;
			}
		}
	}
	else
	{
		for (; record->m_pState->m_flGoalFeetYaw > 180.0; 
			record->m_pState->m_flGoalFeetYaw = record->m_pState->m_flGoalFeetYaw - 360.0);
		for (; record->m_pState->m_flGoalFeetYaw < -180.0; 
			record->m_pState->m_flGoalFeetYaw + 360.0);

		record->m_pState->m_flGoalFeetYaw = middle; // LBY based desync fix
	}

	info->m_flBrute = goal_feet_yaw; // set the new angle //

	ResolveShot(data, record);

	switch (data->m_missed_shots % 4)
	{
	case 0:
		info->m_flBrute = data->m_last_resolve = 58.f;
		break;
	case 1:
		info->m_flBrute = data->m_last_resolve = -58.f;
		break;
	case 2:
		info->m_flBrute = data->m_last_resolve = 29.f;
		break;
	case 3:
		info->m_flBrute = data->m_last_resolve = -29.f;
		break;
	}


	record->m_pState->m_flGoalFeetYaw = math::NormalizedAngle(info->m_flBrute); // normalize angle //
}

float Resolver::ResolveShot(AimPlayer* data, LagComp::LagRecord_t* record)
{
	float flPseudoFireYaw = math::NormalizedAngle(math::CalcAngle(record->m_pMatrix[8].GetOrigin(), g_cl.m_local->m_BoneCache().m_pCachedBones[0].GetOrigin()).y);

	if (data->m_extending)
	{
		float flLeftFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_angEyeAngles.y + 58.f)));
		float flRightFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_angEyeAngles.y - 58.f)));

		return flLeftFireYawDelta > flRightFireYawDelta ? -58.f : 58.f;
	}
	else
	{
		float flLeftFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_angEyeAngles.y + 29.f)));
		float flRightFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_angEyeAngles.y - 29.f)));

		return flLeftFireYawDelta > flRightFireYawDelta ? -29.f : 29.f;
	}
}