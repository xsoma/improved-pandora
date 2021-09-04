#include "../includes.h"

float penetration::scale(Player* player, CGameTrace& enterTrace, WeaponInfo* weaponData, float& currentDamage)
{
	auto is_armored = [&]()->bool
	{
		auto has_helmet = player->m_bHasHelmet();
		auto armor_value = player->m_ArmorValue();

		if (armor_value > 0)
		{
			switch (enterTrace.m_hitgroup)
			{
			case HITGROUP_GENERIC:
			case HITGROUP_CHEST:
			case HITGROUP_STOMACH:
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
			case HITGROUP_GEAR:
				return true;
			case HITGROUP_HEAD:
				return has_helmet || player->m_bHasHeavyArmor();
			default:
				return player->m_bHasHeavyArmor();
			}
		}

		return false;
	};

	static auto mp_damage_scale_ct_head = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_ct_head"));
	static auto mp_damage_scale_t_head = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_t_head"));

	static auto mp_damage_scale_ct_body = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_ct_body"));
	static auto mp_damage_scale_t_body = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_t_body"));

	auto head_scale = player->m_iTeamNum() == 3 ? mp_damage_scale_ct_head->GetFloat() : mp_damage_scale_t_head->GetFloat();
	auto body_scale = player->m_iTeamNum() == 3 ? mp_damage_scale_ct_body->GetFloat() : mp_damage_scale_t_body->GetFloat();

	auto armor_heavy = player->m_bHasHeavyArmor();
	auto armor_value = (float)player->m_ArmorValue();

	if (armor_heavy)
		head_scale *= 0.5f;

	switch (enterTrace.m_hitgroup)
	{
	case HITGROUP_HEAD:
		currentDamage *= 4.0f * head_scale;
		break;
	case HITGROUP_STOMACH:
		currentDamage *= 1.25f * body_scale;
		break;
	case HITGROUP_CHEST:
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
	case HITGROUP_GEAR:
		currentDamage *= body_scale;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		currentDamage *= 0.75f * body_scale;
		break;
	}

	if (is_armored())
	{
		auto armor_scale = 1.0f;
		auto armor_ratio = weaponData->flArmorRatio * 0.5f;
		auto armor_bonus_ratio = 0.5f;

		if (armor_heavy)
		{
			armor_ratio *= 0.2f;
			armor_bonus_ratio = 0.33f;
			armor_scale = 0.25f;
		}

		auto new_damage = currentDamage * armor_ratio;
		auto estiminated_damage = (currentDamage - currentDamage * armor_ratio) * armor_scale * armor_bonus_ratio;

		if (estiminated_damage > armor_value)
			new_damage = currentDamage - armor_value / armor_bonus_ratio;

		currentDamage = new_damage;
	}

	return std::floor(currentDamage);
}

bool penetration::TraceToExit(const vec3_t& start, const vec3_t& dir, vec3_t& out, CGameTrace* enter_trace, CGameTrace* exit_trace)
{
	static CTraceFilterSimple_game filter{};

	float  dist{};
	vec3_t new_end;
	int    contents, first_contents{};

	while (dist <= 90.0f)
	{
		dist += 4.0f;

		out = start + (dir * dist);

		if (!first_contents)
			first_contents = g_csgo.m_engine_trace->GetPointContents(out, MASK_SHOT, nullptr);

		contents = g_csgo.m_engine_trace->GetPointContents(out, MASK_SHOT, nullptr);

		if ((contents & MASK_SHOT_HULL) && (!(contents & CONTENTS_HITBOX) || (contents == first_contents)))
			continue;

		new_end = out - (dir * 4.f);

		g_csgo.m_engine_trace->TraceRay(Ray(out, new_end), MASK_SHOT, nullptr, exit_trace);

		if (g_csgo.sv_clip_penetration_traces_to_players->GetInt())
			game::UTIL_ClipTraceToPlayers(out, new_end, MASK_SHOT, nullptr, exit_trace, -60.0f);

		if (exit_trace->m_startsolid && (exit_trace->m_surface.m_flags & SURF_HITBOX))
		{
			filter.SetPassEntity(exit_trace->m_entity);

			g_csgo.m_engine_trace->TraceRay(Ray(out, start), MASK_SHOT_HULL, (ITraceFilter*)&filter, exit_trace);

			if (exit_trace->hit() && !exit_trace->m_startsolid)
			{
				out = exit_trace->m_endpos;
				return true;
			}

			continue;
		}

		if (!exit_trace->hit() || exit_trace->m_startsolid)
		{
			if (game::IsBreakable(enter_trace->m_entity))
			{
				*exit_trace = *enter_trace;
				exit_trace->m_endpos = start + dir;
				return true;
			}

			continue;
		}

		if ((exit_trace->m_surface.m_flags & SURF_NODRAW))
		{
			if (game::IsBreakable(exit_trace->m_entity) && game::IsBreakable(enter_trace->m_entity))
			{
				out = exit_trace->m_endpos;
				return true;
			}

			if (!(enter_trace->m_surface.m_flags & SURF_NODRAW))
				continue;
		}

		if (exit_trace->m_plane.m_normal.dot(dir) <= 1.0f)
		{
			out -= (dir * (exit_trace->m_fraction * 4.0f));
			return true;
		}
	}

	return false;
}

void penetration::ClipTraceToPlayer(const vec3_t& start, const vec3_t& end, uint32_t mask, CGameTrace* tr, Player* player, float min)
{
	vec3_t     pos, to, dir, on_ray;
	float      len, range_along, range;
	Ray        ray;
	CGameTrace new_trace;

	pos = player->m_vecOrigin() + ((player->m_vecMins() + player->m_vecMaxs()) * 0.5f);
	to = pos - start;
	dir = start - end;
	len = dir.normalize();
	range_along = dir.dot(to);

	if (range_along < 0.f)
		range = -(to).length();
	else if (range_along > len)
		range = -(pos - end).length();
	else
	{
		on_ray = start + (dir * range_along);
		range = (pos - on_ray).length();
	}

	if (range <= 60.f)
	{
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), mask, player, &new_trace);

		if (tr->m_fraction > new_trace.m_fraction)
			*tr = new_trace;
	}
}

bool penetration::run(PenetrationInput_t* in, PenetrationOutput_t* out)
{
	static CTraceFilterSimple_game filter{};

	int			  pen{ 4 }, enter_material, exit_material;
	float		  damage, penetration, penetration_mod, player_damage, remaining, trace_len{}, total_pen_mod, damage_mod, modifier, damage_lost;
	surfacedata_t* enter_surface, * exit_surface;
	bool		  nodraw, grate;
	vec3_t		  start, dir, end, pen_end;
	CGameTrace	  trace, exit_trace;
	Weapon* weapon;
	WeaponInfo* weapon_info;

	if (in->m_from == g_cl.m_local)
	{
		weapon = g_cl.m_weapon;
		weapon_info = g_cl.m_weapon_info;
		start = g_cl.m_shoot_pos;
	}
	else
	{
		weapon = in->m_from->GetActiveWeapon();
		if (!weapon)
			return false;

		weapon_info = weapon->GetWpnData();
		if (!weapon_info)
			return false;

		start = in->m_from->Weapon_ShootPosition();
	}

	damage = (float)weapon_info->iDamage;
	penetration = weapon_info->flPenetration;

	penetration_mod = std::max(0.f, (3.f / penetration) * 1.25f);

	dir = (in->m_pos - start).normalized();

	filter.SetPassEntity(in->m_from);

	while (damage > 0.f)
	{
		remaining = weapon_info->flRange - trace_len;

		end = start + (dir * remaining);

		g_csgo.m_engine_trace->TraceRay(Ray(start, end), MASK_SHOT | CONTENTS_HITBOX, (ITraceFilter*)&filter, &trace);

		if (trace.m_fraction == 1.f)
			return false;

		if (in->m_target)
			ClipTraceToPlayer(start, end + (dir * 40.f), MASK_SHOT | CONTENTS_HITBOX, &trace, in->m_target, -60.f);
		else
			game::UTIL_ClipTraceToPlayers(start, end + (dir * 40.f), MASK_SHOT | CONTENTS_HITBOX, (ITraceFilter*)&filter, &trace, -60.f);

		trace_len += trace.m_fraction * remaining;
		damage *= std::pow(weapon_info->flRangeModifier, trace_len / 500.f);

		if (in->m_target) {

			if (trace.m_entity && trace.m_entity == in->m_target && game::IsValidHitgroup(trace.m_hitgroup))
			{
				int group = (weapon->m_iItemDefinitionIndex() == ZEUS) ? HITGROUP_GENERIC : trace.m_hitgroup;

				player_damage = scale(in->m_target, trace, weapon_info, damage);

				out->m_pen = pen != 4;
				out->m_hitgroup = group;
				out->m_damage = player_damage;
				out->m_target = in->m_target;

				if (pen == 4)
					return player_damage >= in->m_damage;

				return player_damage >= in->m_damage_pen;
			}
		}
		else
		{
			out->m_pen = pen != 4;

			if (trace.m_entity && trace.m_entity->IsPlayer() && game::IsValidHitgroup(trace.m_hitgroup))
			{
				int group = (weapon->m_iItemDefinitionIndex() == ZEUS) ? HITGROUP_GENERIC : trace.m_hitgroup;

				player_damage = scale(trace.m_entity->as< Player* >(), trace, weapon_info, damage);

				out->m_hitgroup = group;
				out->m_damage = player_damage;
				out->m_target = trace.m_entity->as< Player* >();

				if (pen == 4)
					return player_damage >= in->m_damage;

				return player_damage >= in->m_damage_pen;
			}

			out->m_damage = damage;
		}

		if (!in->m_can_pen)
			return false;

		enter_surface = g_csgo.m_phys_props->GetSurfaceData(trace.m_surface.m_surface_props);

		if ((trace_len > 3000.f && penetration) || enter_surface->m_game.m_penetration_modifier < 0.1f)
			return false;

		nodraw = (trace.m_surface.m_flags & SURF_NODRAW);
		grate = (trace.m_contents & CONTENTS_GRATE);

		enter_material = enter_surface->m_game.m_material;

		if (!pen && !nodraw && !grate && enter_material != CHAR_TEX_GRATE && enter_material != CHAR_TEX_GLASS)
			return false;

		if (penetration <= 0.0f || pen <= 0)
			return false;

		if (!TraceToExit(trace.m_endpos, dir, pen_end, &trace, &exit_trace))
		{
			if (!(g_csgo.m_engine_trace->GetPointContents(pen_end, MASK_SHOT_HULL) & MASK_SHOT_HULL))
				return false;
		}

		exit_surface = g_csgo.m_phys_props->GetSurfaceData(exit_trace.m_surface.m_surface_props);
		exit_material = exit_surface->m_game.m_material;

		if (enter_material == CHAR_TEX_GRATE || enter_material == CHAR_TEX_GLASS)
		{
			total_pen_mod = 3.0f;
			damage_mod = 0.05f;
		}
		else if (nodraw || grate)
		{
			total_pen_mod = 1.0f;
			damage_mod = 0.16f;
		}
		else
		{
			total_pen_mod = (enter_surface->m_game.m_penetration_modifier + exit_surface->m_game.m_penetration_modifier) * 0.5f;
			damage_mod = 0.16f;
		}

		if (enter_material == exit_material)
		{
			if (exit_material == CHAR_TEX_WOOD || exit_material == CHAR_TEX_CARDBOARD)
				total_pen_mod = 3.0f;
			else if (exit_material == CHAR_TEX_PLASTIC)
				total_pen_mod = 2.0f;
		}

		trace_len = (exit_trace.m_endpos - trace.m_endpos).length_sqr();
		modifier = fmaxf(1.f / total_pen_mod, 0.f);

		damage_lost = fmaxf(
			((modifier * trace_len) / 24.f)
			+ ((damage * damage_mod)
				+ (fmaxf(3.75 / penetration, 0.f) * 3.f * modifier)), 0.f);

		damage -= std::max(0.f, damage_lost);

		if (damage < 1.0f)
			return false;

		start = exit_trace.m_endpos;
		--pen;
	}

	return false;
}