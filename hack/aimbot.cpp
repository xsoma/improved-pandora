#include "../includes.h"

Aimbot g_aimbot{ };;


void AimPlayer::OnNetUpdate(Player* player) {
	bool reset = (!g_cfg[XOR("rage_aimbot_enabled")].get<bool>() || player->m_lifeState() == LIFE_DEAD || !player->enemy(g_cl.m_local));

	if (reset) {
		m_missed_shots = 0;
		m_last_resolve = 0;

		return;
	}

	// update player ptr.
	m_player = player;
}

void AimPlayer::OnRoundStart(Player* player) {
	m_player = player;
	m_shots = 0;
	m_missed_shots = 0;
	m_last_resolve = 0;
	m_delta = 0;

	m_hitboxes.clear();
}

int synchronize_weapons()
{
	auto local = (Player*)g_csgo.m_entlist->GetClientEntity(g_csgo.m_engine->GetLocalPlayer());
	auto weapon = local->GetActiveWeapon()->m_iItemDefinitionIndex();
	auto cur_weapon = g_cfg["rage_aimbot_weapon_group"].get<int>();

	switch (weapon)
	{
	case SCAR20:
		return 0;
		break;
	case G3SG1:
		return 0;
		break;
	case SSG08:
		return 1;
		break;
	case AWP:
		return 2;
		break;
	case REVOLVER:
		return 3;
		break;
	case DEAGLE:
		return 3;
		break;
	case USPS:
		return 4;
		break;
	case GLOCK:
		return 4;
		break;
	case ELITE:
		return 4;
		break;
	default:
		break;
	}

	cur_weapon = weapon;

	return cur_weapon;
}

void AimPlayer::SetupHitboxes(LagComp::LagRecord_t* record, bool history) {
	if (!record)
		return;

	// reset hitboxes.
	m_hitboxes.clear();

	if (g_cl.m_weapon_id == ZEUS)
	{
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		return;
	}

	// prefer, always.
	if (g_cfg[XOR("rage_aimbot_baim_prefer_always")].get<bool>())
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });

	// prefer, lethal.
	if (g_cfg[XOR("rage_aimbot_baim_prefer_lethal")].get<bool>())
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL });

	// prefer, lethal x2.
	if (g_cfg[XOR("rage_aimbot_baim_prefer_lethalx2")].get<bool>())
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL2 });

	// prefer, in air.
	if (g_cfg[XOR("rage_aimbot_baim_prefer_air")].get<bool>() && !(record->m_fFlags & FL_ONGROUND))
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });

	bool only{ false };

	// only, always.
	if (g_cfg[XOR("rage_aimbot_baim_always")].get<bool>() || g_aimbot.m_force_body) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only, health.
	if (g_cfg[XOR("rage_aimbot_baim_always_health")].get<bool>() && record->m_pEntity->m_iHealth() <= (int)g_cfg[XOR("rage_aimbot_baim_always_health_amount")].get<int>()) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only, in air.
	if (g_cfg[XOR("rage_aimbot_baim_always_air")].get<bool>() && !(record->m_fFlags & FL_ONGROUND)) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only baim conditions have been met.
	// do not insert more hitboxes.
	if (only)
		return;

	bool ignore_limbs = record->m_vecVelocity.length_2d() > 71.f && g_cfg[XOR("rage_aimbot_ignore_limbs")].get<bool>();
	auto weapon = synchronize_weapons();
	switch (weapon)
	{
	case 0: // auto
	{
		if (g_cfg[XOR("rage_aimbot_auto_hitbox_head")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_auto_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_auto_hitbox_neck")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_NECK, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_auto_hitbox_chest")].get<bool>())
		{
			if (g_cfg[XOR("rage_aimbot_auto_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL, true });
			m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false });
		}
		if (g_cfg[XOR("rage_aimbot_auto_hitbox_stomach")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_auto_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true });
				m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_auto_hitbox_arms")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_auto_hitbox_legs")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_auto_hitbox_feet")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL, false });
		}
	} break;
	case 1: // scout
	{
		if (g_cfg[XOR("rage_aimbot_scout_hitbox_head")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_scout_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_scout_hitbox_neck")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_NECK, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_scout_hitbox_chest")].get<bool>())
		{
			if (g_cfg[XOR("rage_aimbot_scout_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL, true });
			m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false });
		}
		if (g_cfg[XOR("rage_aimbot_scout_hitbox_stomach")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_scout_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true });
				m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_PELVIS,HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_scout_hitbox_arms")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_scout_hitbox_legs")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_scout_hitbox_feet")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL, false });
		}
	} break;
	case 2: // awp
	{
		if (g_cfg[XOR("rage_aimbot_awp_hitbox_head")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_awp_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_awp_hitbox_neck")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_NECK, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_awp_hitbox_chest")].get<bool>())
		{
			if (g_cfg[XOR("rage_aimbot_awp_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL, true });
			m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false });
		}
		if (g_cfg[XOR("rage_aimbot_awp_hitbox_stomach")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_awp_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true });
				m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_awp_hitbox_arms")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_awp_hitbox_legs")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_awp_hitbox_feet")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL, false });
		}
	} break;
	case 3: // heavy pistols
	{
		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_head")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_heavy_pistols_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_neck")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_NECK, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_chest")].get<bool>())
		{
			if (g_cfg[XOR("rage_aimbot_heavy_pistols_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL, true });
			m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false });
		}
		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_stomach")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_heavy_pistols_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true });
				m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_arms")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_legs")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_heavy_pistols_hitbox_feet")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL, false });
		}
	} break;
	case 4: // pistols
	{
		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_head")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_pistols_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_neck")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_NECK, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_chest")].get<bool>())
		{
			if (g_cfg[XOR("rage_aimbot_pistols_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL, true });
			m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false });
		}
		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_stomach")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_pistols_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true });
				m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_arms")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_legs")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_pistols_hitbox_feet")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL, false });
		}
	} break;
	case 5: // other
	{
		if (g_cfg[XOR("rage_aimbot_other_hitbox_head")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_other_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_other_hitbox_neck")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_NECK, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_other_hitbox_chest")].get<bool>())
		{
			if (g_cfg[XOR("rage_aimbot_other_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL, true });
			m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false });
		}
		if (g_cfg[XOR("rage_aimbot_other_hitbox_stomach")].get<bool>())
		{

			if (g_cfg[XOR("rage_aimbot_other_prefer_safepoint")].get<bool>())
			{
				m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true });
				m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER_SAFEPOINT, true });
			}

			m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_other_hitbox_arms")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_other_hitbox_legs")].get<bool>())
		{
			m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL, false });
		}

		if (g_cfg[XOR("rage_aimbot_other_hitbox_feet")].get<bool>() && !ignore_limbs)
		{
			m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL, false });
			m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL, false });
		}
	} break;
	}
}

void AimPlayer::ShotRecord(LagComp::LagRecord_t* record, Color col, float time)
{
	const model_t* model;
	studiohdr_t* hdr;
	mstudiohitboxset_t* set;
	mstudiobbox_t* bbox;
	vec3_t             mins, maxs, origin;
	ang_t			   angle;

	model = record->m_pEntity->GetModel();
	if (!model)
		return;

	hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return;

	set = hdr->GetHitboxSet(record->m_pEntity->m_nHitboxSet());
	if (!set)
		return;

	for (int i{ }; i < set->m_hitboxes; ++i)
	{
		bbox = set->GetHitbox(i);
		if (!bbox)
			continue;

		// bbox.
		if (bbox->m_radius <= 0.f)
		{
			// https://developer.valvesoftware.com/wiki/Rotation_Tutorial

			// convert rotation angle to a matrix.
			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms(record->m_pMatrix[bbox->m_bone], rot_matrix, matrix);

			// extract the compound rotation as an angle.
			ang_t bbox_angle;
			math::MatrixAngles(bbox_angle);

			// extract hitbox origin.
			vec3_t origin = matrix.GetOrigin();

			// draw box.
			if (bbox->m_radius != -1)
				g_csgo.m_debug_overlay->AddBoxOverlay(origin, bbox->m_mins, bbox->m_maxs, bbox_angle, col.r(), col.g(), col.b(), 200, time);
		}

		// capsule.
		else
		{
			// NOTE; the angle for capsules is always 0.f, 0.f, 0.f.

			// create a rotation matrix.
			matrix3x4_t matrix;
			g_csgo.AngleMatrix(bbox->m_angle, matrix);

			// apply the rotation matrix to the entity output space (world).
			math::ConcatTransforms(record->m_pMatrix[bbox->m_bone], matrix, matrix);

			// get world positions from new matrix.
			math::VectorTransform(bbox->m_mins, matrix, mins);
			math::VectorTransform(bbox->m_maxs, matrix, maxs);

			if (bbox->m_radius != -1)
				g_csgo.m_debug_overlay->AddCapsuleOverlay(mins, maxs, bbox->m_radius, col.r(), col.g(), col.b(), 200, time, 0, 0);
		}
	}
}

void Aimbot::init() {
	// clear old targets.
	m_targets.clear();

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = ang_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;
	m_hitbox = -1;

	m_best_dist = std::numeric_limits< float >::max();
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max();
	m_best_height = std::numeric_limits< float >::max();
	g_aimbot.m_current_matrix = nullptr;

	if (!g_tickbase.m_shift_data.m_did_shift_before && !g_tickbase.m_shift_data.m_should_be_ready)
		m_shoot_next_tick = false;

	m_current_sphere.clear();
}

void Aimbot::StripAttack() {
	if (g_cl.m_weapon_id == REVOLVER)
		return;
	else
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK;
}

void Aimbot::think() {
	// do all startup routines.
	init();

	// sanity.
	if (!g_cl.m_weapon)
		return;

	// no grenades or bomb.
	if (g_cl.m_weapon->IsGrenade() || g_cl.m_weapon_type == WEAPONTYPE_EQUIPMENT)
		return;

	// we have no aimbot enabled.
	if (!g_cfg[XOR("rage_aimbot_enabled")].get<bool>())
		return;

	if (!g_cl.m_weapon_fire)
		StripAttack();

	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;

	// one tick before being able to shoot.
	if (revolver && g_cl.m_revolver_cock > 0 && g_cl.m_revolver_cock == g_cl.m_revolver_query) {
		//*g_cl.m_packet = false;
		return;
	}

	// no point in aimbotting if we cannot fire this tick.
	if (!g_cl.m_weapon_fire)
		return;

	// setup bones for all valid targets.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		if (!player || player == g_cl.m_local)
			continue;

		if (!IsValidTarget(player))
			continue;

		AimPlayer* data = &m_players[i - 1];
		if (!data)
			continue;

		m_targets.push_back(data);
	}

	// run knifebot.
	if (g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS) {

		if (g_cfg[XOR("misc_knifebot")].get<bool>())
			knife();

		return;
	}

	// scan available targets... if we even have any.
	find();

	// finally set data when shooting.
	apply();
}

void Aimbot::find() {
	struct BestTarget_t { Player* player; AimPlayer* target; vec3_t pos; float damage; float min_damage; LagComp::LagRecord_t* record; int hitbox; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	float		 tmp_min_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.target = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.record = nullptr;
	best.hitbox = -1;

	if (m_targets.empty())
		return;

	// iterate all targets.
	for (const auto& t : m_targets) {
		if (!t->m_player)
			continue;

		const auto ideal = g_lagcompensation.GetLatestRecord(t->m_player);
		if (!ideal.has_value() || ideal.value()->m_bDormant || (ideal.value()->m_pEntity && ideal.value()->m_pEntity->m_bGunGameImmunity()))
			continue;

		t->SetupHitboxes(ideal.value(), false);
		if (t->m_hitboxes.empty())
			continue;

		// try to select best record as target.
		if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, ideal.value(), tmp_min_damage)) {
			if (SelectTarget(ideal.value(), tmp_pos, tmp_damage)) {
				// if we made it so far, set shit. 
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal.value();
				best.min_damage = tmp_min_damage;
				best.target = t;
			}
		}

		const auto last = g_lagcompensation.GetOldestRecord(t->m_player);
		if (!last.has_value() || last.value() == ideal.value() || last.value()->m_bDormant || (last.value()->m_pEntity && last.value()->m_pEntity->m_bGunGameImmunity()))
			continue;

		t->SetupHitboxes(last.value(), true);
		if (t->m_hitboxes.empty())
			continue;

		// rip something went wrong..
		if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, last.value(), tmp_min_damage)) {
			if (SelectTarget(last.value(), tmp_pos, tmp_damage)) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = last.value();
				best.min_damage = tmp_min_damage;
				best.target = t;
			}
		}

		// we found a target we can shoot at and deal damage? fuck yeah. (THIS IS TEMPORARY TILL WE REPLACE THE TARGET SELECTION)
		if (best.damage > 0.f && best.player && best.record)
			break;
	}

	// verify our target and set needed data.
	if (best.player && best.record && best.target) {
		// calculate aim angle.
		math::VectorAngles(best.pos - g_cl.m_shoot_pos, m_angle);

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_record = best.record;
		m_hitbox = best.hitbox;

		m_current_matrix = best.record->m_pMatrix_Resolved;

		if (!m_target || m_target->dormant() || m_record->m_bDormant || !m_current_matrix || !m_damage || !(m_damage >= best.min_damage || (m_damage <= best.min_damage && m_damage >= m_target->m_iHealth())))
			return;

		bool on;

		switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
		{
		case 0: // auto
			on = g_cfg[XOR("rage_aimbot_auto_hitchance")].get<bool>();
			break;
		case 1: // scout
			on = g_cfg[XOR("rage_aimbot_scout_hitchance")].get<bool>();
			break;
		case 2: // awp
			on = g_cfg[XOR("rage_aimbot_awp_hitchance")].get<bool>();
			break;
		case 3: // heavy pistols
			on = g_cfg[XOR("rage_aimbot_heavy_pistols_hitchance")].get<bool>();
			break;
		case 4: // pistols
			on = g_cfg[XOR("rage_aimbot_pistols_hitchance")].get<bool>();
			break;
		case 5: // other
			on = g_cfg[XOR("rage_aimbot_other_hitchance")].get<bool>();
			break;
		}

		bool hit = (!g_cl.m_ground && g_cl.m_weapon_id == SSG08 && g_cl.m_weapon && g_cl.m_weapon->GetInaccuracy() < 0.009f) || (on && CheckHitchance(m_target, m_angle, m_record, best.hitbox));

		// set autostop shit.
		if (g_cfg[XOR("rage_aimbot_auto_stop")].get<bool>()) 
		{
			m_stop = !(g_cl.m_buttons & IN_JUMP) && on && !hit;
			m_slow_motion_slowwalk = false;
			m_slow_motion_fakewalk = false;

			g_movement.AutoStop();
		}
		
		// if we can scope.
		bool can_scope = g_cl.m_weapon && g_cl.m_weapon->m_zoomLevel() == 0 && g_cl.m_weapon->IsZoomable(true);

		if (can_scope)
		{
			// always.
			if (g_cfg[XOR("rage_aimbot_auto_scope")].get<int>() == 1) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}

			// hitchance fail.
			else if (g_cfg[XOR("rage_aimbot_auto_scope")].get<int>() == 2 && on && !hit) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}
		}

		if (hit || !on) {

			// these conditions are so cancer
			if (!g_tickbase.m_shift_data.m_should_attempt_shift || ((!g_cfg[XOR("rage_exploit_charged")].get<bool>() || g_cl.m_goal_shift == 14 || g_cl.m_goal_shift == 16 || g_tickbase.m_shift_data.m_should_disable) && g_tickbase.m_shift_data.m_should_attempt_shift) || (g_cfg[XOR("rage_exploit_charged")].get<bool>() && g_cl.m_goal_shift == 7 && g_tickbase.m_shift_data.m_should_attempt_shift && !(g_tickbase.m_shift_data.m_prepare_recharge || g_tickbase.m_shift_data.m_did_shift_before && !g_tickbase.m_shift_data.m_should_be_ready))) {
				if (g_cfg[XOR("rage_aimbot_autofire")].get<bool>()) {
					// right click attack.
					if (g_cfg[XOR("cheat_mode")].get<int>() == 1 && g_cl.m_weapon_id == REVOLVER)
						g_cl.m_cmd->m_buttons |= IN_ATTACK2;

					// left click attack.
					else
						g_cl.m_cmd->m_buttons |= IN_ATTACK;

					//m_old_target = best.target;
				}
			}
		}
	}
}

static std::vector<std::tuple<float, float, float>> precomputed_seeds = {};

__forceinline void Aimbot::build_seed_table()
{
	if (!precomputed_seeds.empty())
		return;

	for (auto i = 0; i < 255; i++) {
		math::random_seed(i + 1);

		const auto pi_seed = math::random_float(0.f, math::twopi);

		precomputed_seeds.emplace_back(math::random_float(0.f, 1.f),
			sin(pi_seed), cos(pi_seed));
	}
}

bool Aimbot::hitchance(int& final_hitchance, ang_t& angle, Player* player, LagComp::LagRecord_t* record, int hitbox)
{
	// generate look-up-table to enhance performance.
	build_seed_table();

	const auto info = g_cl.m_weapon_info;

	// crash or info
	if (!info)
	{
		final_hitchance = 0;
		return true;
	}

	// get cfg hitchance
	float hitchance_cfg;

	switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
	{
	case 0: // auto
		hitchance_cfg = g_cfg[XOR("rage_aimbot_auto_hitchance_amount")].get<float>();
		break;
	case 1: // scout
		hitchance_cfg = g_cfg[XOR("rage_aimbot_scout_hitchance_amount")].get<float>();
		break;
	case 2: // awp
		hitchance_cfg = g_cfg[XOR("rage_aimbot_awp_hitchance_amount")].get<float>();
		break;
	case 3: // heavy pistols
		hitchance_cfg = g_cfg[XOR("rage_aimbot_heavy_pistols_hitchance_amount")].get<float>();
		break;
	case 4: // pistols
		hitchance_cfg = g_cfg[XOR("rage_aimbot_pistols_hitchance_amount")].get<float>();
		break;
	case 5: // other
		hitchance_cfg = g_cfg[XOR("rage_aimbot_other_hitchance_amount")].get<float>();
		break;
	}

	static auto nospread = g_csgo.m_cvar->FindVar(HASH("weapon_accuracy_nospread"));

	if (nospread)
	{
		final_hitchance = INT_MAX;
		return true;
	}

	// setup calculation parameters.
	const auto round_acc = [](const float accuracy) { return roundf(accuracy * 1000.f) / 1000.f; };
	const auto sniper = g_cl.m_weapon_id == AWP || g_cl.m_weapon_id == G3SG1
		|| g_cl.m_weapon_id == SCAR20 || g_cl.m_weapon_id == SSG08;
	const auto crouched = g_cl.m_local->m_fFlags() & FL_DUCKING;

	// calculate inaccuracy.
	const auto weapon_inaccuracy = g_cl.m_weapon->GetInaccuracy();

	// no need for hitchance, if we can't increase it anyway.
	if (crouched)
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyCrouchAlt : info->flInaccuracyCrouch))
		{
			final_hitchance = INT_MIN;
			return true;
		}
	}
	else
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyStandAlt : info->flInaccuracyStand))
		{
			final_hitchance = INT_MAX;
			return true;
		}
	}

	// calculate start and angle.
	static auto weapon_recoil_scale = g_csgo.m_cvar->FindVar(HASH("weapon_recoil_scale"));
	auto forward = vec3_t(0,0,0);
	auto right = vec3_t(0, 0, 0);
	auto up = vec3_t(0, 0, 0);

	math::AngleVectors(angle, &forward, &right, &up);

	// vector normilize
	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);

	// keep track of all traces that hit the enemy.
	auto current = 0;

	// setup calculation parameters.
	vec3_t total_spread, spread_angle, end;
	float inaccuracy, spread_x, spread_y;
	std::tuple<float, float, float>* seed;

	// use look-up-table to find average hit probability.
	for (auto i = 0u; i < 255; i++)  // NOLINT(modernize-loop-convert)
	{
		// get seed.
		seed = &precomputed_seeds[i];

		// calculate spread.
		inaccuracy = std::get<0>(*seed) * weapon_inaccuracy;
		spread_x = std::get<2>(*seed) * inaccuracy;
		spread_y = std::get<1>(*seed) * inaccuracy;
		total_spread = (forward + right * spread_x + up * spread_y);
		total_spread.normalized();

		// calculate angle with spread applied.
		math::VectorAngles(total_spread, angle, &spread_angle);

		// calculate end point of trace.
		math::AngleVectors(angle, &spread_angle, &end);
		end.normalized();
		end = g_cl.m_shoot_pos + end * info->flRange;

		// did we hit the hitbox?
		if (CanHit(g_cl.m_shoot_pos, end, record, hitbox))
			current++;

		// abort if hitchance is already sufficent.
		if ((static_cast<float>(current) / 175.f) * 100.f >= hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current) / 175.f) * 100.f;
			return true;
		}

		// abort if we can no longer reach hitchance.
		if ((static_cast<float>(current + 175 - i) / 175.f) * 100.f < hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current + 175 - i) / 175.f) * 100.f;
			return false;
		}
	}

	final_hitchance = (static_cast<float>(current) / 175.f) * 100.f;
	return (static_cast<float>(current) / 175.f) * 100.f >= hitchance_cfg;
}

bool Aimbot::CheckHitchance(Player* player, const ang_t& angle, LagComp::LagRecord_t* record, int hitbox)
{
	if (!g_cl.m_weapon)
		return false;

	build_seed_table();

	vec3_t     start{ g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
	float      inaccuracy, spread;
	float hitchance;

	switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
	{
	case 0: // auto
		hitchance = g_cfg[XOR("rage_aimbot_auto_hitchance_amount")].get<float>();
		break;
	case 1: // scout
		hitchance = g_cfg[XOR("rage_aimbot_scout_hitchance_amount")].get<float>();
		break;
	case 2: // awp
		hitchance = g_cfg[XOR("rage_aimbot_awp_hitchance_amount")].get<float>();
		break;
	case 3: // heavy pistols
		hitchance = g_cfg[XOR("rage_aimbot_heavy_pistols_hitchance_amount")].get<float>();
		break;
	case 4: // pistols
		hitchance = g_cfg[XOR("rage_aimbot_pistols_hitchance_amount")].get<float>();
		break;
	case 5: // other
		hitchance = g_cfg[XOR("rage_aimbot_other_hitchance_amount")].get<float>();
		break;
	}

	size_t     total_hits{ }, needed_hits{ (size_t)std::ceil((hitchance * 255) / 100.f) };

	// get needed directional vectors.
	math::AngleVectors(angle, &fwd, &right, &up);

	// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
	inaccuracy = g_cl.m_weapon->GetInaccuracy();
	spread = g_cl.m_weapon->GetSpread();

	g_cl.m_weapon->UpdateAccuracyPenalty();


	// iterate all possible seeds.
	for (int i{ }; i <= 255; ++i) 
	{
		// get spread.
		wep_spread = g_cl.m_weapon->CalculateSpread(i, inaccuracy, spread);

		// get spread direction.
		dir = (fwd + (right * wep_spread.x) + (up * wep_spread.y)).normalized();

		// get end of trace.
		end = start + (dir * g_cl.m_weapon_info->flRange);

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (CanHit(start, end, record, hitbox))
			++total_hits;

		// we made it.
		if (total_hits >= needed_hits)
			return true;

		// we cant make it anymore.
		if ((255 - i + total_hits) < needed_hits)
			return false;
	}

	return (static_cast<float>(total_hits) / 175.f) * 100.f >= hitchance;
}

bool AimPlayer::SetupHitboxPoints(LagComp::LagRecord_t* record, BoneArray* bones, int index, std::vector< vec3_t >& points) {
	// reset points.
	points.clear();

	const model_t* model = record->m_pEntity->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(record->m_pEntity->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(index);
	if (!bbox)
		return false;

	// get hitbox scales.
	// remove bbox->m_radius
	float auto_scale = g_cfg[XOR("rage_aimbot_auto_multipoint_scale")].get<float>();
	float scout_scale = g_cfg[XOR("rage_aimbot_scout_multipoint_scale")].get<float>();
	float awp_scale = g_cfg[XOR("rage_aimbot_awp_multipoint_scale")].get<float>();
	float heavy_pistols_scale = g_cfg[XOR("rage_aimbot_heavy_pistols_multipoint_scale")].get<float>();
	float pistols_scale = g_cfg[XOR("rage_aimbot_pistols_multipoint_scale")].get<float>();
	float other_scale = g_cfg[XOR("rage_aimbot_other_multipoint_scale")].get<float>();

	// big inair fix.
	if (!(record->m_fFlags) & FL_ONGROUND)
	{
		switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
		{
		case 0: // auto
			auto_scale = 0.7f;
			break;
		case 1: // scout
			scout_scale = 0.7f;
			break;
		case 2: // awp
			awp_scale = 0.7f;
			break;
		case 3: // heavy pistols
			heavy_pistols_scale = 0.7f;
			break;
		case 4: // pistols
			pistols_scale = 0.7f;
			break;
		case 5: // other
			other_scale = 0.7f;
			break;
		}
	}

	// these indexes represent boxes.
	if (bbox->m_radius <= 0.f) 
	{
		// references: 
		//      https://developer.valvesoftware.com/wiki/Rotation_Tutorial
		//      CBaseAnimating::GetHitboxBonePosition
		//      CBaseAnimating::DrawServerHitboxes

		// convert rotation angle to a matrix.
		matrix3x4_t rot_matrix;
		g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

		// apply the rotation to the entity input space (local).
		matrix3x4_t matrix;
		math::ConcatTransforms(bones[bbox->m_bone], rot_matrix, matrix);

		// extract origin from matrix.
		vec3_t origin = matrix.GetOrigin();

		// compute raw center point.
		vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

		// the feet hiboxes have a side, heel and the toe.
		if (index == HITBOX_R_FOOT || index == HITBOX_L_FOOT) {
			float d1 = (bbox->m_mins.z - center.z) * 0.875f;

			// invert.
			if (index == HITBOX_L_FOOT)
				d1 *= -1.f;

			// side is more optimal then center.
			points.push_back({ center.x, center.y, center.z + d1 });

			switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
			{
			case 0: // auto
			{
				if (g_cfg[XOR("rage_aimbot_auto_multipoint_feet")].get<bool>())
				{
					// get point offset relative to center point
					// and factor in hitbox scale.
					float d2 = (bbox->m_mins.x - center.x) * auto_scale;
					float d3 = (bbox->m_maxs.x - center.x) * auto_scale;

					// heel.
					points.push_back({ center.x + d2, center.y, center.z });

					// toe.
					points.push_back({ center.x + d3, center.y, center.z });
				}
			} break;
			case 1: // scout
			{
				if (g_cfg[XOR("rage_aimbot_scout_multipoint_feet")].get<bool>())
				{
					// get point offset relative to center point
					// and factor in hitbox scale.
					float d2 = (bbox->m_mins.x - center.x) * scout_scale;
					float d3 = (bbox->m_maxs.x - center.x) * scout_scale;

					// heel.
					points.push_back({ center.x + d2, center.y, center.z });

					// toe.
					points.push_back({ center.x + d3, center.y, center.z });
				}
			} break;
			case 2: // awp
			{
				if (g_cfg[XOR("rage_aimbot_awp_multipoint_feet")].get<bool>())
				{
					// get point offset relative to center point
					// and factor in hitbox scale.
					float d2 = (bbox->m_mins.x - center.x) * awp_scale;
					float d3 = (bbox->m_maxs.x - center.x) * awp_scale;

					// heel.
					points.push_back({ center.x + d2, center.y, center.z });

					// toe.
					points.push_back({ center.x + d3, center.y, center.z });
				}
			} break;
			case 3: // heavy pistols
			{
				if (g_cfg[XOR("rage_aimbot_heavy_pistols_multipoint_feet")].get<bool>())
				{
					// get point offset relative to center point
					// and factor in hitbox scale.
					float d2 = (bbox->m_mins.x - center.x) * heavy_pistols_scale;
					float d3 = (bbox->m_maxs.x - center.x) * heavy_pistols_scale;

					// heel.
					points.push_back({ center.x + d2, center.y, center.z });

					// toe.
					points.push_back({ center.x + d3, center.y, center.z });
				}
			} break;
			case 4: // pistols
			{
				if (g_cfg[XOR("rage_aimbot_pistols_multipoint_feet")].get<bool>())
				{
					// get point offset relative to center point
					// and factor in hitbox scale.
					float d2 = (bbox->m_mins.x - center.x) * pistols_scale;
					float d3 = (bbox->m_maxs.x - center.x) * pistols_scale;

					// heel.
					points.push_back({ center.x + d2, center.y, center.z });

					// toe.
					points.push_back({ center.x + d3, center.y, center.z });
				}
			} break;
			case 5: // other
			{
				if (g_cfg[XOR("rage_aimbot_other_multipoint_feet")].get<bool>())
				{
					// get point offset relative to center point
					// and factor in hitbox scale.
					float d2 = (bbox->m_mins.x - center.x) * other_scale;
					float d3 = (bbox->m_maxs.x - center.x) * other_scale;

					// heel.
					points.push_back({ center.x + d2, center.y, center.z });

					// toe.
					points.push_back({ center.x + d3, center.y, center.z });
				}
			} break;
			}
		}

		// nothing to do here we are done.
		if (points.empty())
			return false;

		// rotate our bbox points by their correct angle
		// and convert our points to world space.
		for (auto& p : points) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p = { p.dot(matrix[0]), p.dot(matrix[1]), p.dot(matrix[2]) };

			// transform point to world space.
			p += origin;
		}
	}

	// these hitboxes are capsules.
	else
	{
		float r, br;

		// factor in the pointscale.
		switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
		{
		case 0: // auto
			r = bbox->m_radius * auto_scale;
			break;
		case 1: // scout
			r = bbox->m_radius * scout_scale;
			break;
		case 2: // awp
			r = bbox->m_radius * awp_scale;
			break;
		case 3: // heavy pistols
			r = bbox->m_radius * heavy_pistols_scale;
			break;
		case 4: // pistols
			r = bbox->m_radius * pistols_scale;
			break;
		case 5: // other
			r = bbox->m_radius * other_scale;
			break;
		}

		// compute raw center point.
		vec3_t center = (bbox->m_mins + bbox->m_maxs) * 0.5f;

		// head has 5 points.
		if (index == HITBOX_HEAD) {
			// add center.
			points.push_back(center);

			switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
			{
			case 0: // auto
			{
				if (g_cfg[XOR("rage_aimbot_auto_multipoint_head")].get<bool>())
				{
					// rotation matrix 45 degrees.
					// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
					// std::cos( deg_to_rad( 45.f ) )
					constexpr float rotation = 0.70710678f;

					// top/back 45 deg.
					// this is the best spot to shoot at.
					points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

					vec3_t right{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r };

					// right.
					points.push_back(right);

					vec3_t left{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r };

					// left.
					points.push_back(left);

					// back.
					points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

					// get animstate ptr.
					CCSGOPlayerAnimState* state = record->m_pEntity->m_PlayerAnimState();

					// add this point only under really specific circumstances.
					// if we are standing still and have the lowest possible pitch pose.
					//if (state && record->m_vecVelocity.length() <= 0.1f && record->m_angEyeAngles.x <= state->m_flMinPitch)
					//{

					//	// bottom point.
					//	points.push_back({ bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z });
					//}
				}
			} break;
			case 1: // scout
			{
				if (g_cfg[XOR("rage_aimbot_scout_multipoint_head")].get<bool>())
				{
					// rotation matrix 45 degrees.
					// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
					// std::cos( deg_to_rad( 45.f ) )
					constexpr float rotation = 0.70710678f;

					// top/back 45 deg.
					// this is the best spot to shoot at.
					points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

					vec3_t right{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r };

					// right.
					points.push_back(right);

					vec3_t left{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r };

					// left.
					points.push_back(left);

					// back.
					points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

					// get animstate ptr.
					CCSGOPlayerAnimState* state = record->m_pEntity->m_PlayerAnimState();

					// add this point only under really specific circumstances.
					//// if we are standing still and have the lowest possible pitch pose.
					//if (state && record->m_vecVelocity.length() <= 0.1f && record->m_angEyeAngles.x <= state->m_flMinPitch)
					//{

					//	// bottom point.
					//	points.push_back({ bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z });
					//}
				}
			} break;
			case 2: // awp
			{
				if (g_cfg[XOR("rage_aimbot_awp_multipoint_head")].get<bool>())
				{
					// rotation matrix 45 degrees.
					// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
					// std::cos( deg_to_rad( 45.f ) )
					constexpr float rotation = 0.70710678f;

					// top/back 45 deg.
					// this is the best spot to shoot at.
					points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

					vec3_t right{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r };

					// right.
					points.push_back(right);

					vec3_t left{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r };

					// left.
					points.push_back(left);

					// back.
					points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

					// get animstate ptr.
					CCSGOPlayerAnimState* state = record->m_pEntity->m_PlayerAnimState();

					// add this point only under really specific circumstances.
					// if we are standing still and have the lowest possible pitch pose.
					//if (state && record->m_vecVelocity.length() <= 0.1f && record->m_angEyeAngles.x <= state->m_flMinPitch)
					//{

					//	// bottom point.
					//	points.push_back({ bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z });
					//}
				}
			} break;
			case 3: // heavy pistols
			{
				if (g_cfg[XOR("rage_aimbot_heavy_pistols_multipoint_head")].get<bool>())
				{
					// rotation matrix 45 degrees.
					// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
					// std::cos( deg_to_rad( 45.f ) )
					constexpr float rotation = 0.70710678f;

					// top/back 45 deg.
					// this is the best spot to shoot at.
					points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

					vec3_t right{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r };

					// right.
					points.push_back(right);

					vec3_t left{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r };

					// left.
					points.push_back(left);

					// back.
					points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

					// get animstate ptr.
					CCSGOPlayerAnimState* state = record->m_pEntity->m_PlayerAnimState();

					// add this point only under really specific circumstances.
					// if we are standing still and have the lowest possible pitch pose.
					//if (state && record->m_vecVelocity.length() <= 0.1f && record->m_angEyeAngles.x <= state->m_flMinPitch)
					//{

					//	// bottom point.
					//	points.push_back({ bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z });
					//}
				}
			} break;
			case 4: // pistols
			{
				if (g_cfg[XOR("rage_aimbot_pistols_multipoint_head")].get<bool>())
				{
					// rotation matrix 45 degrees.
					// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
					// std::cos( deg_to_rad( 45.f ) )
					constexpr float rotation = 0.70710678f;

					// top/back 45 deg.
					// this is the best spot to shoot at.
					points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

					vec3_t right{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r };

					// right.
					points.push_back(right);

					vec3_t left{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r };

					// left.
					points.push_back(left);

					// back.
					points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

					// get animstate ptr.
					CCSGOPlayerAnimState* state = record->m_pEntity->m_PlayerAnimState();

					// add this point only under really specific circumstances.
					// if we are standing still and have the lowest possible pitch pose.
					//if (state && record->m_vecVelocity.length() <= 0.1f && record->m_angEyeAngles.x <= state->m_flMinPitch)
					//{

					//	// bottom point.
					//	points.push_back({ bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z });
					//}
				}
			} break;
			case 5: // other
			{
				if (g_cfg[XOR("rage_aimbot_other_multipoint_head")].get<bool>())
				{
					// rotation matrix 45 degrees.
					// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
					// std::cos( deg_to_rad( 45.f ) )
					constexpr float rotation = 0.70710678f;

					// top/back 45 deg.
					// this is the best spot to shoot at.
					points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

					vec3_t right{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r };

					// right.
					points.push_back(right);

					vec3_t left{ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r };

					// left.
					points.push_back(left);

					// back.
					points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

					// get animstate ptr.
					CCSGOPlayerAnimState* state = record->m_pEntity->m_PlayerAnimState();

					// add this point only under really specific circumstances.
					// if we are standing still and have the lowest possible pitch pose.
					//if (state && record->m_vecVelocity.length() <= 0.1f && record->m_angEyeAngles.x <= state->m_flMinPitch)
					//{

					//	// bottom point.
					//	points.push_back({ bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z });
					//}
				}
			} break;
			}
		}

		// body has 5 points.
		else if (index == HITBOX_BODY) {
			// center.
			points.push_back(center);

			// back.
			switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
			{
			case 0: // auto
			{
				if (g_cfg[XOR("rage_aimbot_auto_multipoint_stomach")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y, center.z });
			} break;
			case 1: // scout
			{
				if (g_cfg[XOR("rage_aimbot_scout_multipoint_stomach")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y, center.z });
			} break;
			case 2: // awp
			{
				if (g_cfg[XOR("rage_aimbot_awp_multipoint_stomach")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y, center.z });
			} break;
			case 3: // heavy pistols
			{
				if (g_cfg[XOR("rage_aimbot_heavy_pistols_multipoint_stomach")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y, center.z });
			} break;
			case 4: // pistols
			{
				if (g_cfg[XOR("rage_aimbot_pistols_multipoint_stomach")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y, center.z });
			} break;
			case 5: // other
			{
				if (g_cfg[XOR("rage_aimbot_other_multipoint_stomach")].get<bool>())
				points.push_back({ center.x, bbox->m_maxs.y, center.z });
			} break;
			}
		}

		else if (index == HITBOX_PELVIS || index == HITBOX_UPPER_CHEST) 
			points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
		

		// other stomach/chest hitboxes have 2 points.
		else if (index == HITBOX_THORAX || index == HITBOX_CHEST) {
			// add center.
			points.push_back(center);

			// add extra point on back.
			switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
			{
			case 0: // auto
			{
				if (g_cfg[XOR("rage_aimbot_auto_multipoint_chest")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
			} break;
			case 1: // scout
			{
				if (g_cfg[XOR("rage_aimbot_scout_multipoint_chest")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
			} break;
			case 2: // awp
			{
				if (g_cfg[XOR("rage_aimbot_awp_multipoint_chest")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
			} break;
			case 3: // heavy pistols
			{
				if (g_cfg[XOR("rage_aimbot_heavy_pistols_multipoint_chest")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
			} break;
			case 4: // pistols
			{
				if (g_cfg[XOR("rage_aimbot_pistols_multipoint_chest")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
			} break;
			case 5: // other
			{
				if (g_cfg[XOR("rage_aimbot_other_multipoint_chest")].get<bool>())
					points.push_back({ center.x, bbox->m_maxs.y - r, center.z });
			} break;
			}
		}

		else if (index == HITBOX_R_CALF || index == HITBOX_L_CALF) {
			// add center.
			points.push_back(center);

			// half bottom.
			switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
			{
			case 0: // auto
			{
				if (g_cfg[XOR("rage_aimbot_auto_multipoint_legs")].get<bool>())
					points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
			} break;
			case 1: // scout
			{
				if (g_cfg[XOR("rage_aimbot_scout_multipoint_legs")].get<bool>())
					points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
			} break;
			case 2: // awp
			{
				if (g_cfg[XOR("rage_aimbot_awp_multipoint_legs")].get<bool>())
					points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
			} break;
			case 3: // heavy pistols
			{
				if (g_cfg[XOR("rage_aimbot_heavy_pistols_multipoint_legs")].get<bool>())
					points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
			} break;
			case 4: // pistols
			{
				if (g_cfg[XOR("rage_aimbot_pistols_multipoint_legs")].get<bool>())
					points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
			} break;
			case 5: // other
			{
				if (g_cfg[XOR("rage_aimbot_other_multipoint_legs")].get<bool>())
					points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
			} break;
			}
		}

		else if (index == HITBOX_R_THIGH || index == HITBOX_L_THIGH) {
			// add center.
			points.push_back(center);
		}

		// arms get only one point.
		else if (index == HITBOX_R_UPPER_ARM || index == HITBOX_L_UPPER_ARM) {
			// elbow.
			points.push_back({ bbox->m_maxs.x + bbox->m_radius, center.y, center.z });
		}

		// nothing left to do here.
		if (points.empty())
			return false;

		// transform capsule points.
		for (auto& p : points)
			math::VectorTransform(p, bones[bbox->m_bone], p);
	}

	return true;
}

bool AimPlayer::GetBestAimPosition(vec3_t& aim, float& damage, int& hitbox, LagComp::LagRecord_t* record, float& tmp_min_damage) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;

	// get player hp.
	int hp = std::min(100, record->m_pEntity->m_iHealth());

	m_matrix = record->m_pMatrix_Resolved;

	if (!m_matrix)
		return false;

	if (g_cl.m_weapon_id == ZEUS) {
		dmg = pendmg = hp;
		pen = true;
	}

	else
	{
		switch (synchronize_weapons()/*g_cfg["rage_aimbot_weapon_group"].get<int>()*/)
		{
		case 0: // auto
		{
			dmg = g_cfg[XOR("rage_aimbot_auto_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage)			
				dmg = g_cfg[XOR("rage_aimbot_auto_minimal_damage_override")].get<float>();
			
			pendmg = g_cfg[XOR("rage_aimbot_auto_penetration_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage && g_cfg[XOR("rage_aimbot_auto_minimal_damage_override_pen")].get<bool>())			
				pendmg = g_cfg[XOR("rage_aimbot_auto_minimal_damage_override")].get<float>();
			
			pen = g_cfg[XOR("rage_aimbot_auto_penetration")].get<bool>();
		} break;
		case 1: // scout
		{
			dmg = g_cfg[XOR("rage_aimbot_scout_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage)			
				dmg = g_cfg[XOR("rage_aimbot_scout_minimal_damage_override")].get<float>();
			

			pendmg = g_cfg[XOR("rage_aimbot_scout_penetration_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage && g_cfg[XOR("rage_aimbot_scout_minimal_damage_override_pen")].get<bool>())			
				pendmg = g_cfg[XOR("rage_aimbot_scout_minimal_damage_override")].get<float>();
			

			pen = g_cfg[XOR("rage_aimbot_scout_penetration")].get<bool>();
		} break;
		case 2: // awp
		{
			dmg = g_cfg[XOR("rage_aimbot_awp_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage)			
				dmg = g_cfg[XOR("rage_aimbot_awp_minimal_damage_override")].get<float>();
			
			pendmg = g_cfg[XOR("rage_aimbot_awp_penetration_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage && g_cfg[XOR("rage_aimbot_awp_minimal_damage_override_pen")].get<bool>())		
				pendmg = g_cfg[XOR("rage_aimbot_awp_minimal_damage_override")].get<float>();
	
			pen = g_cfg[XOR("rage_aimbot_awp_penetration")].get<bool>();
		} break;
		case 3: // heavy pistols
		{
			dmg = g_cfg[XOR("rage_aimbot_heavy_pistols_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage)			
				dmg = g_cfg[XOR("rage_aimbot_heavy_pistols_minimal_damage_override")].get<float>();
			
			pendmg = g_cfg[XOR("rage_aimbot_heavy_pistols_penetration_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage && g_cfg[XOR("rage_aimbot_heavy_pistols_minimal_damage_override_pen")].get<bool>())			
				pendmg = g_cfg[XOR("rage_aimbot_heavy_pistols_minimal_damage_override")].get<float>();
			
			pen = g_cfg[XOR("rage_aimbot_heavy_pistols_penetration")].get<bool>();
		} break;
		case 4: // pistols
		{
			dmg = g_cfg[XOR("rage_aimbot_pistols_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage)		
				dmg = g_cfg[XOR("rage_aimbot_pistols_minimal_damage_override")].get<float>();
			
			pendmg = g_cfg[XOR("rage_aimbot_pistols_penetration_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage && g_cfg[XOR("rage_aimbot_pistols_minimal_damage_override_pen")].get<bool>())			
				pendmg = g_cfg[XOR("rage_aimbot_pistols_minimal_damage_override")].get<float>();
			
			pen = g_cfg[XOR("rage_aimbot_pistols_penetration")].get<bool>();
		} break;
		case 5: // other
		{
			dmg = g_cfg[XOR("rage_aimbot_other_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage)			
				dmg = g_cfg[XOR("rage_aimbot_other_minimal_damage_override")].get<float>();
			
			pendmg = g_cfg[XOR("rage_aimbot_other_penetration_minimal_damage")].get<float>();

			if (g_aimbot.m_override_damage && g_cfg[XOR("rage_aimbot_other_minimal_damage_override_pen")].get<bool>())			
				pendmg = g_cfg[XOR("rage_aimbot_other_minimal_damage_override")].get<float>();
			
			pen = g_cfg[XOR("rage_aimbot_other_penetration")].get<bool>();
		} break;
		}
	}

	// backup player
	const auto backup_origin = record->m_pEntity->m_vecOrigin();
	const auto backup_abs_origin = record->m_pEntity->GetAbsOrigin();
	const auto backup_abs_angles = record->m_pEntity->GetAbsAngles();
	const auto backup_obb_mins = record->m_pEntity->m_vecMins();
	const auto backup_obb_maxs = record->m_pEntity->m_vecMaxs();
	const auto backup_cache = record->m_pEntity->m_BoneCache2();

	auto restore = [&]() -> void {
		record->m_pEntity->m_vecOrigin() = backup_origin;
		record->m_pEntity->SetAbsOrigin(backup_abs_origin);
		record->m_pEntity->SetAbsAngles(backup_abs_angles);
		record->m_pEntity->m_vecMins() = backup_obb_mins;
		record->m_pEntity->m_vecMaxs() = backup_obb_maxs;
		record->m_pEntity->m_BoneCache2() = backup_cache;
	};

	// iterate hitboxes.
	for (const auto& it : m_hitboxes) {
		done = false;

		// setup points on hitbox.
		if (!SetupHitboxPoints(record, m_matrix, it.m_index, points)) {
			continue;
		}

		// iterate points on hitbox.
		for (const auto& point : points) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = pendmg;
			in.m_can_pen = pen;
			in.m_target = record->m_pEntity;
			in.m_from = g_cl.m_local;
			in.m_pos = point;

			// ignore mindmg.
			//if(/*(it.m_mode == HitscanMode::PREFER && it.m_safepoint) ||*/ it.m_mode == HitscanMode::LETHAL || it.m_mode == HitscanMode::LETHAL2 )
				//in.m_damage = in.m_damage_pen = 1.f;

			penetration::PenetrationOutput_t out;

			// tests for intersection with unresolved matrix, if it returns true, the point should (!) be a safe point
			bool is_safepoint = g_aimbot.CanHit(g_cl.m_shoot_pos, point, record, it.m_index, true, record->m_pMatrix);

			// we only want safe pointable (nice word) hitboxes when forcing..
			if (!is_safepoint && g_aimbot.m_force_safepoint)
				continue;

			//	if (is_safepoint && it.m_safepoint)
					//g_csgo.m_debug_overlay->AddLineOverlay(g_cl.m_shoot_pos, point, 255, 255, 255, false, 0.1f);

				// setup trace data
			record->m_pEntity->m_vecOrigin() = record->m_vecOrigin;
			record->m_pEntity->SetAbsOrigin(record->m_vecOrigin);
			record->m_pEntity->SetAbsAngles(record->m_angAbsAngles);
			record->m_pEntity->m_vecMins() = record->m_vecMins;
			record->m_pEntity->m_vecMaxs() = record->m_vecMaxs;
			record->m_pEntity->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(m_matrix);

			// we can hit p!
			if (penetration::run(&in, &out)) {
				tmp_min_damage = dmg;

				// restore trace data
				restore();

				// nope we did not hit head..
				if (it.m_index == HITBOX_HEAD && out.m_hitgroup != HITGROUP_HEAD)
					continue;

				// prefered hitbox, just stop now.
				if (it.m_mode == HitscanMode::PREFER && !it.m_safepoint)
					done = true;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				else if (it.m_mode == HitscanMode::LETHAL && out.m_damage >= record->m_pEntity->m_iHealth())
					done = true;

				// 2 shots will be sufficient to kill.
				else if (it.m_mode == HitscanMode::LETHAL2 && (out.m_damage * 2.f) >= record->m_pEntity->m_iHealth())
					done = true;

				// always prefer safe points if we want to.
				else if (it.m_mode == HitscanMode::PREFER_SAFEPOINT && it.m_safepoint && is_safepoint)
					done = true;

				// this hitbox has normal selection, it needs to have more damage.
				else if (it.m_mode == HitscanMode::NORMAL) {
					// we did more damage.
					if (out.m_damage > scan.m_damage) {
						// save new best data.
						scan.m_damage = out.m_damage;
						scan.m_pos = point;

						scan.m_hitbox = it.m_index;

						scan.m_safepoint = it.m_safepoint;

						// if the first point is lethal
						// screw the other ones.
						if (point == points.front() && out.m_damage >= record->m_pEntity->m_iHealth())
							break;
					}
				}

				// we found a preferred / lethal hitbox.
				if (done) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					scan.m_hitbox = it.m_index;
					scan.m_safepoint = it.m_mode == HitscanMode::PREFER && it.m_safepoint;
					scan.m_mode = it.m_mode;

					break;
				}
			}
			else {
				// restore trace data
				restore();
			}
		}

		// ghetto break out of outer loop.
		if (done)
			break;
	}

	// we found something that we can damage.
	// set out vars.
	if (scan.m_damage > 0.f) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		hitbox = scan.m_hitbox;

		g_aimbot.m_current_matrix = m_matrix;

		return true;
	}

	return false;
}

bool Aimbot::SelectTarget(LagComp::LagRecord_t* record, const vec3_t& aim, float damage) {
	float dist, fov, height;
	int   hp;

	// fov check.
	if (g_cfg[XOR("rage_aimbot_limit_fov")].get<bool>()) {
		// if out of fov, retn false.
		if (math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, aim) > g_cfg[XOR("rage_aimbot_limit_fov_amount")].get<float>())
			return false;
	}

	switch (g_cfg[XOR("rage_aimbot_target_selection")].get<int>()) {

		// distance.
	case 0:
		dist = g_cl.m_shoot_pos.dist_to(record->m_vecOrigin);

		if (dist < m_best_dist) {
			m_best_dist = dist;
			return true;
		}

		break;

		// crosshair.
	case 1:
		fov = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, aim);

		if (fov < m_best_fov) {
			m_best_fov = fov;
			return true;
		}

		break;

		// damage.
	case 2:
		if (damage > m_best_damage) {
			m_best_damage = damage;
			return true;
		}

		break;

		// lowest hp.
	case 3:
		// fix for retarded servers?
		hp = std::min(100, record->m_pEntity->m_iHealth());

		if (hp < m_best_hp) {
			m_best_hp = hp;
			return true;
		}

		break;

		// least lag.
	case 4:
		if (record->m_iChoked < m_best_lag) {
			m_best_lag = record->m_iChoked;
			return true;
		}

		break;

		// height.
	case 5:
		height = record->m_vecOrigin.z - g_cl.m_local->m_vecOrigin().z;

		if (height < m_best_height) {
			m_best_height = height;
			return true;
		}

		break;

	default:
		return false;
	}

	return false;
}

float GetLerpTime()
{
	auto cl_updaterate = g_csgo.m_cvar->FindVar(HASH("cl_updaterate"));
	auto sv_minupdaterate = g_csgo.m_cvar->FindVar(HASH("sv_minupdaterate"));
	auto sv_maxupdaterate = g_csgo.m_cvar->FindVar(HASH("sv_maxupdaterate"));

	auto cl_interp = g_csgo.m_cvar->FindVar(HASH("cl_interp"));
	auto sv_client_min_interp_ratio = g_csgo.m_cvar->FindVar(HASH("sv_client_min_interp_ratio"));
	auto sv_client_max_interp_ratio = g_csgo.m_cvar->FindVar(HASH("sv_client_max_interp_ratio"));

	auto updateRate = cl_updaterate->GetInt();
	auto interpRatio = cl_interp->GetFloat();
	auto minInterpRatio = sv_client_min_interp_ratio->GetFloat();
	auto maxInterpRatio = sv_client_max_interp_ratio->GetFloat();
	auto minUpdateRate = sv_minupdaterate->GetInt();
	auto maxUpdateRate = sv_maxupdaterate->GetInt();

	auto clampedUpdateRate = std::clamp(updateRate, minUpdateRate, maxUpdateRate);
	auto clampedInterpRatio = std::clamp(interpRatio, minInterpRatio, maxInterpRatio);

	auto lerp = clampedInterpRatio / clampedUpdateRate;

	if (lerp <= cl_interp->GetFloat())
		lerp = cl_interp->GetFloat();

	return lerp;
}

void Aimbot::apply() {
	bool attack, attack2;

	// attack states.
	attack = (g_cl.m_cmd->m_buttons & IN_ATTACK);
	attack2 = (g_cl.m_weapon_id == REVOLVER && g_cl.m_cmd->m_buttons & IN_ATTACK2);

	// if not silent aim, apply the viewangles.
	if (m_target) {
		if (!g_cfg[XOR("rage_aimbot_silent_aim")].get<bool>())
			g_csgo.m_engine->SetViewAngles(m_angle);
	}

	// ensure we're attacking.
	if (attack || attack2) {
		// choke every shot.
		//*g_cl.m_packet = false;

		if (m_shoot_next_tick)
			m_shoot_next_tick = false;

		if (m_target) {

			bool broken_lc = (m_target->m_vecOrigin() - m_record->m_vecOrigin).length_sqr() > 4096.f;

			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if (m_record && !broken_lc) {
				g_cl.m_cmd->m_tick = game::TIME_TO_TICKS(m_record->m_flSimulationTime + GetLerpTime()/*g_cl.m_lerp */);
			}

			// set angles to target.
			g_cl.m_cmd->m_view_angles = m_angle;
		}

		// store fired shot.
		if (g_cl.IsFiring(game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase()))) 		
			g_shots.OnShotFire(m_target ? m_target : nullptr, m_target ? m_damage : -1.f, 1, m_target ? m_record : nullptr, m_hitbox);	

		if (!m_shoot_next_tick && g_cl.m_goal_shift == 14 || g_cl.m_goal_shift == 16 && g_tickbase.m_shift_data.m_should_attempt_shift && !(g_tickbase.m_shift_data.m_prepare_recharge || g_tickbase.m_shift_data.m_did_shift_before && !g_tickbase.m_shift_data.m_should_be_ready)) {
			m_shoot_next_tick = true;
		}
	}
}

bool Aimbot::CanHit(vec3_t start, vec3_t end, LagComp::LagRecord_t* record, int box, bool in_shot, BoneArray* bones)
{
	if (!record || !record->m_pEntity)
		return false;

	// backup player
	const auto backup_origin = record->m_pEntity->m_vecOrigin();
	const auto backup_abs_origin = record->m_pEntity->GetAbsOrigin();
	const auto backup_abs_angles = record->m_pEntity->GetAbsAngles();
	const auto backup_obb_mins = record->m_pEntity->m_vecMins();
	const auto backup_obb_maxs = record->m_pEntity->m_vecMaxs();
	const auto backup_cache = record->m_pEntity->m_BoneCache2();

	// always try to use our aimbot matrix first.
	auto matrix = m_current_matrix;

	// this is basically for using a custom matrix.
	if (in_shot)
		matrix = bones;

	if (!matrix)
		return false;

	const model_t* model = record->m_pEntity->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(record->m_pEntity->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(box);
	if (!bbox)
		return false;

	vec3_t min, max;
	const auto IsCapsule = bbox->m_radius != -1.f;

	if (IsCapsule) {
		math::VectorTransform(bbox->m_mins, matrix[bbox->m_bone], min);
		math::VectorTransform(bbox->m_maxs, matrix[bbox->m_bone], max);
		const auto dist = math::SegmentToSegment(start, end, min, max);

		if (dist < bbox->m_radius) {
			return true;
		}
	}
	else {
		CGameTrace tr;

		// setup trace data
		record->m_pEntity->m_vecOrigin() = record->m_vecOrigin;
		record->m_pEntity->SetAbsOrigin(record->m_vecOrigin);
		record->m_pEntity->SetAbsAngles(record->m_angAbsAngles);
		record->m_pEntity->m_vecMins() = record->m_vecMins;
		record->m_pEntity->m_vecMaxs() = record->m_vecMaxs;
		record->m_pEntity->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(matrix);

		// setup ray and trace.
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, record->m_pEntity, &tr);

		record->m_pEntity->m_vecOrigin() = backup_origin;
		record->m_pEntity->SetAbsOrigin(backup_abs_origin);
		record->m_pEntity->SetAbsAngles(backup_abs_angles);
		record->m_pEntity->m_vecMins() = backup_obb_mins;
		record->m_pEntity->m_vecMaxs() = backup_obb_maxs;
		record->m_pEntity->m_BoneCache2() = backup_cache;

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (tr.m_entity == record->m_pEntity && game::IsValidHitgroup(tr.m_hitgroup))
			return true;
	}

	return false;
}