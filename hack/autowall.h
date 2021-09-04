/*#pragma once
#include "..\includes.h"

class WeaponInfo;
class Weapon;

struct fire_bullet_data
{
	vec3_t src;
	CGameTrace enter_trace;
	vec3_t direction;
	CTraceFilter filter;
	float trace_length;
	float trace_length_remaining;
	float current_damage;
	int penetrate_count;
};

class autowall : public singleton <autowall>
{
public:
	struct returninfo_t
	{
		bool valid = false;

		bool visible = false;
		int damage = -1;
		int hitbox = -1;

		returninfo_t()
		{
			valid = false;

			visible = false;
			damage = -1;
			hitbox = -1;
		}

		returninfo_t(bool visible, int damage, int hitbox)
		{
			valid = true;

			this->visible = visible;
			this->damage = damage;
			this->hitbox = hitbox;
		}
	};

	struct FireBulletData
	{
		vec3_t src;
		CGameTrace enter_trace;
		vec3_t direction;
		CTraceFilter filter;
		WeaponInfo* wpn_data;
		float trace_length;
		float trace_length_remaining;
		float length_to_end;
		float current_damage;
		int penetrate_count;
	};

	bool is_breakable_entity(Entity* e);
	void scale_damage(Player* e, CGameTrace& enterTrace, WeaponInfo* weaponData, float& currentDamage);
	bool trace_to_exit(CGameTrace& enterTrace, CGameTrace& exitTrace, vec3_t startPosition, const vec3_t& direction);
	bool handle_bullet_penetration(WeaponInfo* weaponData, CGameTrace& enterTrace, vec3_t& eyePosition, const vec3_t& direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, bool draw_impact = false);
	returninfo_t wall_penetration(const vec3_t& eye_pos, vec3_t& point, Entity* e);
	bool fire_bullet(Weapon* pWeapon, vec3_t& direction, bool& visible, float& currentDamage, int& hitbox, Entity* e = nullptr, float length = 0.f, const vec3_t& pos = { 0.f,0.f,0.f });
};*/