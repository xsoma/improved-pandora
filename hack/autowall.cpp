/*#include "autowall.h"

bool autowall::is_breakable_entity(Entity* e)
{
	if (!e || !e->index())
		return false;

	static auto is_breakable = g_csgo.IsBreakableEntity;

	auto take_damage = *(uintptr_t*)((uintptr_t)is_breakable + 0x26);
	auto backup = *(uint8_t*)((uintptr_t)e + take_damage);

	auto client_class = e->GetClientClass();
	auto network_name = client_class->m_pNetworkName;

	if (!strcmp(network_name, "CBreakableSurface"))
		*(uint8_t*)((uintptr_t)e + take_damage) = DAMAGE_YES;
	else if (!strcmp(network_name, "CBaseDoor") || !strcmp(network_name, "CDynamicProp"))
		*(uint8_t*)((uintptr_t)e + take_damage) = DAMAGE_NO;

	auto result = is_breakable(e);

	*(uint8_t*)((uintptr_t)e + take_damage) = backup;
	return result;
}

void autowall::scale_damage(Player* e, CGameTrace& enterTrace, WeaponInfo* weaponData, float& currentDamage)
{
	if (!e->IsPlayer())
		return;

	auto is_armored = [&]()->bool
	{
		auto has_helmet = e->m_bHasHelmet();
		auto armor_value = e->m_ArmorValue();

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
				return has_helmet || e->m_bHasHeavyArmor();
			default:
				return e->m_bHasHeavyArmor();
			}
		}

		return false;
	};

	static auto mp_damage_scale_ct_head = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_ct_head"));
	static auto mp_damage_scale_t_head = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_t_head"));

	static auto mp_damage_scale_ct_body = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_ct_body"));
	static auto mp_damage_scale_t_body = g_csgo.m_cvar->FindVar(HASH("mp_damage_scale_t_body"));

	auto head_scale = e->m_iTeamNum() == 3 ? mp_damage_scale_ct_head->GetFloat() : mp_damage_scale_t_head->GetFloat();
	auto body_scale = e->m_iTeamNum() == 3 ? mp_damage_scale_ct_body->GetFloat() : mp_damage_scale_t_body->GetFloat();

	auto armor_heavy = e->m_bHasHeavyArmor();
	auto armor_value = (float)e->m_ArmorValue();

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
}

bool autowall::trace_to_exit(CGameTrace& enterTrace, CGameTrace& exitTrace, vec3_t startPosition, const vec3_t& direction)
{
	auto enter_point_contents = 0;
	auto point_contents = 0;

	auto is_window = 0;
	auto flag = 0;

	auto fDistance = 0.0f;
	vec3_t start, end;

	do
	{
		fDistance += 4.0f;

		end = startPosition + direction * fDistance;
		start = end - direction * 4.0f;

		if (!enter_point_contents)
		{
			enter_point_contents = g_csgo.m_engine_trace->GetPointContents(end, 0x4600400B);
			point_contents = enter_point_contents;
		}
		else
			point_contents = g_csgo.m_engine_trace->GetPointContents(end, 0x4600400B);

		if (point_contents & MASK_SHOT_HULL && (!(point_contents & CONTENTS_HITBOX) || enter_point_contents == point_contents))
			continue;

		static auto trace_filter_simple = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F0 83 EC 7C 56 52")).rel32(0x3D);

		uint32_t filter_[4] =
		{
			(uint32_t)(trace_filter_simple),
			(uint32_t)g_cl.m_local,
			0,
			0
		};

		g_csgo.m_engine_trace->TraceRay(Ray(end, start), MASK_SHOT_HULL | CONTENTS_HITBOX, (CTraceFilter*)filter_, &exitTrace);

		if (exitTrace.m_startsolid && exitTrace.m_surface.m_flags & SURF_HITBOX)
		{
			CTraceFilter filter;
			filter.skip_entity = exitTrace.m_entity;

			filter_[1] = (uint32_t)exitTrace.m_entity;

			g_csgo.m_engine_trace->TraceRay(Ray(end, startPosition), MASK_SHOT_HULL, (CTraceFilter*)filter_, &exitTrace);

			if (exitTrace.hit() && !exitTrace.m_startsolid)
				return true;

			continue;
		}

		auto name = (int*)enterTrace.m_surface.m_name;

		if (name)
		{
			if (*name == 1936744813 && name[1] == 1601397551 && name[2] == 1768318575 && name[3] == 1731159395 && name[4] == 1936941420 && name[5] == 1651668271 && name[6] == 1734307425 && name[7] == 1936941420)
				is_window = 1;
			else
			{
				is_window = 0;

				if (*name != 1936744813)
					goto LABEL_34;
			}

			if (name[1] == 1600480303 && name[2] == 1701536108 && name[3] == 1634494255 && name[4] == 1731162995 && name[5] == 1936941420)
			{
				flag = 1;

			LABEL_35:
				if (is_window || flag)
				{
					exitTrace = enterTrace;
					exitTrace.m_endpos = end + direction;
					return true;
				}

				goto LABEL_37;
			}
		LABEL_34:
			flag = 0;
			goto LABEL_35;
		}

	LABEL_37:
		if (!exitTrace.hit() || exitTrace.m_startsolid)
		{
			if (enterTrace.m_entity && enterTrace.m_entity->index() && is_breakable_entity(enterTrace.m_entity))
			{
				exitTrace = enterTrace;
				exitTrace.m_endpos = startPosition + direction;
				return true;
			}

			continue;
		}

		if (exitTrace.m_surface.m_flags & SURF_NODRAW)
		{
			if (is_breakable_entity(exitTrace.m_entity) && is_breakable_entity(enterTrace.m_entity))
				return true;

			if (!(enterTrace.m_surface.m_flags & SURF_NODRAW))
				continue;
		}

		if (exitTrace.m_plane.m_normal.dot(direction) <= 1.0)
			return true;

	} while (fDistance <= 90.0f);

	return false;
}

bool autowall::handle_bullet_penetration(WeaponInfo* weaponData, CGameTrace& enterTrace, vec3_t& eyePosition, const vec3_t& direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, bool draw_impact)
{
	if (weaponData->flPenetration <= 0.0f)
		return false;

	if (possibleHitsRemaining <= 0)
		return false;

	auto contents_grate = enterTrace.m_contents & CONTENTS_GRATE;
	auto surf_nodraw = enterTrace.m_surface.m_flags & SURF_NODRAW;

	auto enterSurfaceData = g_csgo.m_phys_props->GetSurfaceData(enterTrace.m_surface.m_surface_props);
	auto enter_material = enterSurfaceData->m_game.m_material;

	auto is_solid_surf = enterTrace.m_contents >> 3 & CONTENTS_SOLID;
	auto is_light_surf = enterTrace.m_surface.m_flags >> 7 & SURF_LIGHT;

	CGameTrace exit_trace;

	if (!trace_to_exit(enterTrace, exit_trace, enterTrace.m_endpos, direction) && !(g_csgo.m_engine_trace->GetPointContents(enterTrace.m_endpos, MASK_SHOT_HULL) & MASK_SHOT_HULL))
		return false;

	auto enter_penetration_modifier = enterSurfaceData->m_game.m_penetration_modifier;
	auto exit_surface_data = g_csgo.m_phys_props->GetSurfaceData(exit_trace.m_surface.m_surface_props);

	if (!exit_surface_data)
		return false;

	auto exit_material = exit_surface_data->m_game.m_material;
	auto exit_penetration_modifier = exit_surface_data->m_game.m_penetration_modifier;

	auto combined_damage_modifier = 0.16f;
	auto combined_penetration_modifier = (enter_penetration_modifier + exit_penetration_modifier) * 0.5f;

	if (enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE)
	{
		combined_penetration_modifier = 3.0f;
		combined_damage_modifier = 0.05f;
	}
	else if (contents_grate || surf_nodraw)
		combined_penetration_modifier = 1.0f;
	else if (enter_material == CHAR_TEX_FLESH && ((Player*)enterTrace.m_entity)->m_iTeamNum() == g_cl.m_local->m_iTeamNum() && !ff_damage_reduction_bullets)
	{
		if (!ff_damage_bullet_penetration)
			return false;

		combined_penetration_modifier = ff_damage_bullet_penetration;
		combined_damage_modifier = 0.16f;
	}

	if (enter_material == exit_material)
	{
		if (exit_material == CHAR_TEX_WOOD || exit_material == CHAR_TEX_CARDBOARD)
			combined_penetration_modifier = 3.0f;
		else if (exit_material == CHAR_TEX_PLASTIC)
			combined_penetration_modifier = 2.0f;
	}

	auto penetration_modifier = std::fmaxf(0.0f, 1.0f / combined_penetration_modifier);
	auto penetration_distance = (exit_trace.m_endpos - enterTrace.m_endpos).length();

	penetration_distance = penetration_distance * penetration_distance * penetration_modifier * 0.041666668f;

	auto damage_modifier = std::max(0.0f, 3.0f / weaponData->flPenetration * 1.25f) * penetration_modifier * 3.0f + currentDamage * combined_damage_modifier + penetration_distance;
	auto damage_lost = std::max(0.0f, damage_modifier);

	if (damage_lost > currentDamage)
		return false;

	currentDamage -= damage_lost;

	if (currentDamage < 1.0f)
		return false;

	eyePosition = exit_trace.m_endpos;
	--possibleHitsRemaining;

	return true;
}

bool autowall::fire_bullet(Weapon* pWeapon, vec3_t& direction, bool& visible, float& currentDamage, int& hitbox, Entity* e, float length, const vec3_t& pos)
{
	if (!pWeapon)
		return false;

	auto weaponData = pWeapon->GetWpnData();

	if (!weaponData)
		return false;

	CGameTrace enterTrace;
	CTraceFilter filter;

	filter.skip_entity = g_cl.m_local;
	currentDamage = weaponData->iDamage;

	auto eyePosition = pos;
	auto currentDistance = 0.0f;
	auto maxRange = weaponData->flRange;
	auto penetrationDistance = 3000.0f;
	auto penetrationPower = weaponData->flPenetration;
	auto possibleHitsRemaining = 4;

	while (currentDamage >= 1.0f)
	{
		maxRange -= currentDistance;
		auto end = eyePosition + direction * maxRange;

		CTraceFilter filter;
		filter.skip_entity = g_cl.m_local;

		g_csgo.m_engine_trace->TraceRay(Ray(eyePosition, end), MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &enterTrace);
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(eyePosition, end + direction * 40.0f), MASK_SHOT_HULL | CONTENTS_HITBOX, e, &enterTrace);

		auto enterSurfaceData = g_csgo.m_phys_props->GetSurfaceData(enterTrace.m_surface.m_surface_props);
		auto enterSurfPenetrationModifier = enterSurfaceData->m_game.m_penetration_modifier;
		auto enterMaterial = enterSurfaceData->m_game.m_material;

		if (enterTrace.m_fraction == 1.0f)
			break;

		currentDistance += enterTrace.m_fraction * maxRange;
		currentDamage *= pow(weaponData->flRangeModifier, currentDistance / 500.0f);

		if (currentDistance > penetrationDistance && weaponData->flPenetration || enterSurfPenetrationModifier < 0.1f)
			break;

		auto canDoDamage = enterTrace.m_hitgroup != HITGROUP_GEAR && enterTrace.m_hitgroup != HITGROUP_GENERIC;
		auto isPlayer = ((Player*)enterTrace.m_entity)->IsPlayer();
		auto isEnemy = ((Player*)enterTrace.m_entity)->m_iTeamNum() != g_cl.m_local->m_iTeamNum();

		if (canDoDamage && isPlayer && isEnemy)
		{
			scale_damage((Player*)enterTrace.m_entity, enterTrace, weaponData, currentDamage);
			hitbox = enterTrace.m_hitbox;
			return true;
		}

		if (!possibleHitsRemaining)
			break;

		static auto damageReductionBullets = g_csgo.m_cvar->FindVar(HASH("ff_damage_reduction_bullets"));
		static auto damageBulletPenetration = g_csgo.m_cvar->FindVar(HASH("ff_damage_bullet_penetration"));

		if (!handle_bullet_penetration(weaponData, enterTrace, eyePosition, direction, possibleHitsRemaining, currentDamage, penetrationPower, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat(), !e))
			break;

		visible = false;
	}

	return false;
}

autowall::returninfo_t autowall::wall_penetration(const vec3_t& eye_pos, vec3_t& point, Entity* e)
{
	//g_ctx.globals.autowalling = true;
	auto tmp = point - eye_pos;

	auto angles = ZERO;
	math::VectorAngles(tmp, angles);

	auto direction = ZERO_;
	math::AngleVectors(angles, &direction);

	direction.normalize();

	auto visible = true;
	auto damage = -1.0f;
	auto hitbox = -1;

	Weapon* weapon;
	//auto weapon = g_cl.m_local->m_hActiveWeapon().Get();

	if (fire_bullet(weapon, direction, visible, damage, hitbox, e, 0.0f, eye_pos))
	{
		//g_ctx.globals.autowalling = false;
		return returninfo_t(visible, (int)damage, hitbox);
	}
	else
	{
		//g_ctx.globals.autowalling = false;
		return returninfo_t(false, -1, -1);
	}
}*/