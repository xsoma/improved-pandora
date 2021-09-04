#include "../includes.h"

Client g_cl{ };

std::string GetComputerName()
{
	char buffer[MAX_COMPUTERNAME_LENGTH + 1] = " ";
	unsigned long size = MAX_COMPUTERNAME_LENGTH + 1;
	::GetComputerNameA(buffer, &size);
	return std::string(buffer);
}

ulong_t __stdcall Client::init(void* arg)
{
	g_cl.m_user = GetComputerName();

	if (!g_csgo.init())
		return 0;

	g_notify.add(tfm::format(XOR("welcome %s"), g_cl.m_user));

	return 1;
}

void Client::DrawHUD()
{
	if (!g_cfg[XOR("misc_watermark")].get<bool>())
		return;

	const auto margin = 10;
	const auto padding = 4;

	const auto col_background = Color(0, 0, 0, 240);
	const auto col_accent = Color(90, 120, 240);
	const auto col_text = Color(255, 255, 255);

	auto t = std::time(nullptr);
	std::ostringstream time;
	time << std::put_time(std::localtime(&t), XOR("%H:%M:%S"));

	std::string logo = "pandora.space";
	const std::string user = GetComputerName();

	auto delay = std::max(0, (int)std::round(g_cl.m_latency * 1000.f));
	auto tick_rate = (int)(1.f / g_csgo.m_globals->m_interval);

	auto text = tfm::format(XOR("%s | %s | %s"), logo, user, time.str().data());

	if (g_csgo.m_engine->IsInGame())
		text = tfm::format(XOR("%s | %s | delay: %ims | %itick | %s"), logo, user, delay, tick_rate, time.str().data());

	int w, h;
	g_csgo.m_engine->GetScreenSize(w, h);

	auto text_size = render::Verdana.size(text);

	auto text_pos = vec2_t(w - margin - padding - text_size.m_width,
		margin + padding);
	auto bg_size = vec2_t(padding + text_size.m_width + padding,
		padding + text_size.m_height + padding);
	auto bg_pos = vec2_t(w - margin - padding - text_size.m_width - padding,
		margin);

	render::rect_filled(bg_pos.x, bg_pos.y, bg_size.x, bg_size.y, col_background);
	render::rect_filled(bg_pos.x, bg_pos.y, bg_size.x, 2, col_accent);
	render::Verdana.string(text_pos.x, text_pos.y, col_text, text, render::ALIGN_LEFT);
}

void Client::KillFeed() 
{
	static bool clear_notices = false;
	static bool old_setting = g_cfg[XOR("misc_preserve_killfeed")].get<bool>();
	static void(__thiscall * clear_death_notices_addr)(uintptr_t);
	static uintptr_t* death_notices_addr;

	if (!g_csgo.m_engine->IsInGame() || !g_cl.m_local)
		return;

	if (!g_cl.m_local) {

		// invalidate the clear death notice address if local goes invalid.
		if (clear_death_notices_addr) {
			clear_death_notices_addr = nullptr;
		}

		// invalidate the death notice address if local goes invalid.
		if (death_notices_addr) {
			death_notices_addr = nullptr;
		}

		// that's it, exit out.
		return;
	}

	// only do the following if local is alive.
	if (g_cl.m_local->alive()) {
		// get the death notice addr.
		if (!death_notices_addr) {
			death_notices_addr = game::FindHudElement<uintptr_t>(XOR("CCSGO_HudDeathNotice"));
		}

		// get the clear death notice addr.
		if (!clear_death_notices_addr) {
			clear_death_notices_addr = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 EC 0C 53 56 8B 71 58")).as< void(__thiscall*)(uintptr_t) >();
		}

		// only do the following if both addresses were found and are valid.
		if (clear_death_notices_addr && death_notices_addr) {
			// grab the local death notice time.
			float* local_death_notice_time = (float*)((uintptr_t)death_notices_addr + 0x50);
			static float backup_local_death_notice_time = *local_death_notice_time;

			// extend killfeed time.
			if (g_cl.m_round_flags == 1) {
				if (local_death_notice_time && g_cfg[XOR("misc_preserve_killfeed")].get<bool>()) {
					*local_death_notice_time = std::numeric_limits<float>::max();
				}
			}

			// if we disable the option, clear the death notices.
			if (old_setting != g_cfg[XOR("misc_preserve_killfeed")].get<bool>()) {
				if (!g_cfg[XOR("misc_preserve_killfeed")].get<bool>()) {
					if (local_death_notice_time) {
						// reset back to the regular death notice time.
						*local_death_notice_time = backup_local_death_notice_time;
					}

					clear_notices = true;
				}
				old_setting = g_cfg[XOR("misc_preserve_killfeed")].get<bool>();
			}

			// if we have the option enabled and we need to clear the death notices.
			if (g_cfg[XOR("misc_preserve_killfeed")].get<bool>()) {
				if (g_cl.m_round_flags == 0 && death_notices_addr - 0x14) {
					clear_notices = true;
				}
			}

			// clear the death notices.
			if (clear_notices) {
				clear_death_notices_addr(((uintptr_t)death_notices_addr - 0x14));
				clear_notices = false;
			}
		}
	}
	else {
		// invalidate clear death notice address.
		if (clear_death_notices_addr) {
			clear_death_notices_addr = nullptr;
		}

		// invalidate death notice address.
		if (death_notices_addr) {
			death_notices_addr = nullptr;
		}
	}
}

void Client::ClanTag()
{
	auto SetClanTag = [&](std::string tag) -> void
	{
		using SetClanTag_t = int(__fastcall*)(const char*, const char*);
		static auto SetClanTagFn = pattern::find(g_csgo.m_engine_dll, XOR("53 56 57 8B DA 8B F9 FF 15")).as<SetClanTag_t>();

		SetClanTagFn(tag.c_str(), XOR("pandora.space"));
	};

	std::string szClanTag = XOR("pandora.space ");
	std::string szSuffix = XOR(" $");
	static int iPrevFrame = 0;
	static bool bReset = false;
	int iCurFrame = ((int)(g_inputpred.m_stored_variables.m_curtime * 2.f)) % (szClanTag.size() * 2);

	if (g_cfg[XOR("misc_clantag")].get<bool>())
	{
		if (iPrevFrame != iCurFrame)
		{
			for (int i = 0; i < iCurFrame; i++)
				std::rotate(szClanTag.begin(), szClanTag.begin() + 1, szClanTag.end());

			szClanTag = tfm::format(XOR("%s%s"), szClanTag, szSuffix);

			SetClanTag(szClanTag.data());

			iPrevFrame = iCurFrame;
		}

		bReset = true;
	}
	else
	{
		if (bReset)
		{
			SetClanTag(XOR(""));
			bReset = false;
		}
	}
}

void Client::UnlockHiddenConvars()
{
	if (!g_csgo.m_cvar)
		return;

	auto p = **reinterpret_cast<ConVar***>(g_csgo.m_cvar + 0x34);
	for (auto c = p->m_next; c != nullptr; c = c->m_next)
	{
		c->m_flags &= ~(1 << 1);
		c->m_flags &= ~(1 << 4);
	}
}

void Client::MouseDelta()
{
	if (!m_local || !m_cmd)
		return;

	static ang_t delta_viewangles{ };
	ang_t delta = m_cmd->m_view_angles - delta_viewangles;
	delta.clamp();

	static ConVar* sensitivity = g_csgo.m_cvar->FindVar(HASH("sensitivity"));

	if (!sensitivity)
		return;

	if (delta.x != 0.f)
	{
		static ConVar* m_pitch = g_csgo.m_cvar->FindVar(HASH("m_pitch"));

		if (!m_pitch)
			return;

		int final_dy = static_cast<int>((delta.x / m_pitch->GetFloat()) / sensitivity->GetFloat());
		if (final_dy <= 32767)
		{
			if (final_dy >= -32768)
			{
				if (final_dy >= 1 || final_dy < 0)
				{
					if (final_dy <= -1 || final_dy > 0)
						final_dy = final_dy;
					else
						final_dy = -1;
				}
				else
					final_dy = 1;
			}
			else
				final_dy = 32768;
		}
		else
			final_dy = 32767;

		m_cmd->m_mousedy = static_cast<short>(final_dy);
	}

	if (delta.y != 0.f)
	{
		static ConVar* m_yaw = g_csgo.m_cvar->FindVar(HASH("m_yaw"));

		if (!m_yaw)
			return;

		int final_dx = static_cast<int>((delta.y / m_yaw->GetFloat()) / sensitivity->GetFloat());
		if (final_dx <= 32767)
		{
			if (final_dx >= -32768)
			{
				if (final_dx >= 1 || final_dx < 0)
				{
					if (final_dx <= -1 || final_dx > 0)
						final_dx = final_dx;
					else
						final_dx = -1;
				}
				else
					final_dx = 1;
			}
			else
				final_dx = 32768;
		}
		else
			final_dx = 32767;

		m_cmd->m_mousedx = static_cast<short>(final_dx);
	}

	delta_viewangles = m_cmd->m_view_angles;
}

void Client::OnPaint()
{
	if (g_csgo.m_engine->IsInGame())
	{
		g_visuals.think();
		g_grenades.paint();
	}

	g_notify.think();

//	Keybinds();
	DrawHUD();
	KillFeed();
}

void Client::OnMapload()
{
	g_netvars.SetupClassData();

	m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());

	g_visuals.ModulateWorld();

	g_skins.load();

	m_sequences.clear();
}

void Client::StartMove( CUserCmd* cmd ) {
	// save some usercmd stuff.
	m_cmd = cmd;
	m_tick = cmd->m_tick;
	m_view_angles = cmd->m_view_angles;
	m_buttons = cmd->m_buttons;
	m_pressing_move = ( m_buttons & ( IN_LEFT ) || m_buttons & ( IN_FORWARD ) || m_buttons & ( IN_BACK ) ||
		m_buttons & ( IN_RIGHT ) || m_buttons & ( IN_MOVELEFT ) || m_buttons & ( IN_MOVERIGHT ) ||
		m_buttons & ( IN_JUMP ) );

	// store max choke
	// TODO; 6 -> m_bIsValveDS
	m_max_lag = 14;
	m_lag = g_csgo.m_cl->iChokedCommands;
	m_lerp = game::GetClientInterpAmount( );
	m_latency = g_csgo.m_net->GetLatency( INetChannel::FLOW_OUTGOING );
	math::clamp( m_latency, 0.f, 1.f );
	m_latency_ticks = game::TIME_TO_TICKS( m_latency );
	m_server_tick = g_csgo.m_cl->iServerCount;
	m_arrival_tick = m_server_tick + m_latency_ticks;

	// get local ptr.
	m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// processing indicates that the localplayer is valid and alive.
	m_processing = m_local && g_cl.m_local->alive();
	if( !m_processing )
		return;

	// store some stuff about the local player.
	m_flags = m_local->m_fFlags( );

	// do tickbase shift related code.
	g_tickbase.PreMovement( );

	// make sure prediction has ran on all usercommands.
	// because prediction runs on frames, when we have low fps it might not predict all usercommands.
	// also fix the tick being inaccurate.
	g_inputpred.Misc.m_bOverrideModifier = true;
	g_inputpred.update( );

	// ...
	m_shot = false;
}


void Client::DoMove( ) {
	penetration::PenetrationOutput_t tmp_pen_data{ };

	// backup strafe angles (we need them for input prediction)
	m_strafe_angles = m_cmd->m_view_angles;

	// run movement code before input prediction.
	g_movement.JumpRelated( );
	g_movement.Strafe( );
	g_movement.AutoStop( );
	g_movement.FakeWalk( );
	//g_movement.AutoPeek( );
	//g_hvh.FakeDuck( );

	// make sure prediction has ran on all usercommands.
	// because prediction runs on frames, when we have low fps it might not predict all usercommands.
	// also fix the tick being inaccurate.
	g_inputpred.predict( );

	// restore original angles after input prediction
	//m_cmd->m_view_angles = m_view_angles;

	// convert viewangles to directional forward vector.
	math::AngleVectors( m_view_angles, &m_forward_dir );

	// store stuff after input pred.
	m_shoot_pos = m_local->Weapon_ShootPosition( );

	// reset shit.
	m_weapon = nullptr;
	m_weapon_info = nullptr;
	m_weapon_id = -1;
	m_weapon_type = WEAPONTYPE_UNKNOWN;
	m_player_fire = m_weapon_fire = false;

	// store weapon stuff.
	m_weapon = m_local->GetActiveWeapon( );

	if( m_weapon ) {
		m_weapon_info = m_weapon->GetWpnData( );
		m_weapon_id = m_weapon->m_iItemDefinitionIndex( );
		m_weapon_type = m_weapon_info->WeaponType;

		// ensure weapon spread values / etc are up to date.
		if( !m_weapon->IsGrenade( ) )
			m_weapon->UpdateAccuracyPenalty( );

		// run autowall once for penetration crosshair if we have an appropriate weapon.
		if( m_weapon_type != WEAPONTYPE_KNIFE && m_weapon_type != WEAPONTYPE_EQUIPMENT && !m_weapon->IsGrenade( ) ) {
			penetration::PenetrationInput_t in;
			in.m_from = m_local;
			in.m_target = nullptr;
			in.m_pos = m_shoot_pos + ( m_forward_dir * m_weapon_info->flRange );
			in.m_damage = 1.f;
			in.m_damage_pen = 1.f;
			in.m_can_pen = true;

			// run autowall.
			penetration::run( &in, &tmp_pen_data );
		}

		// set pen data for penetration crosshair.
		m_pen_data = tmp_pen_data;

		// can the player fire.
		m_player_fire = game::TICKS_TO_TIME(m_local->m_nTickBase()) >= m_local->m_flNextAttack( ) && !g_csgo.m_gamerules->m_bFreezePeriod( ) && !( g_cl.m_flags & FL_FROZEN );

		if( g_cfg[ XOR( "rage_aimbot_enabled" ) ].get<bool>( ) ) {
			UpdateRevolverCock( );
		}

		m_weapon_fire = CanFireWeapon(game::TICKS_TO_TIME(m_local->m_nTickBase()));
	}

	// last tick defuse.
	// todo - dex;  figure out the range for CPlantedC4::Use?
	//              add indicator if valid (on ground, still have time, not being defused already, etc).
	//              move this? not sure where we should put it.
	if( g_input.GetKeyState( g_cfg[ XOR( "misc_last_tick_defuse_key" ) ].get<int>( ) ) && g_visuals.m_c4_planted ) {
		float defuse = ( m_local->m_bHasDefuser( ) ) ? 5.f : 10.f;
		float remaining = g_visuals.m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;
		float dt = remaining - defuse - ( g_cl.m_latency / 2.f );

		m_cmd->m_buttons &= ~IN_USE;
		if( dt <= game::TICKS_TO_TIME( 2 ) )
			m_cmd->m_buttons |= IN_USE;
	}

	// grenade prediction.
	g_grenades.think( );

	// run fakelag.
	g_hvh.SendPacket( );

	// run aimbot.
	g_aimbot.think( );

	// run antiaims.
	g_hvh.AntiAim( );
}

void Client::EndMove( CUserCmd* cmd ) {
	// if matchmaking mode, anti untrust clamp.
	if( g_cfg[ XOR( "cheat_mode" ) ].get<int>( ) == 0 )
		m_cmd->m_view_angles.SanitizeAngle( );

	// this packet will be sent.
	if( *m_packet ) {
		g_hvh.m_step_switch = ( bool )g_csgo.RandomInt( 0, 1 );

		// we are sending a packet, so this will be reset soon.
		// store the old value.
		m_old_lag = m_lag;

		// get radar angles.
		m_radar = cmd->m_view_angles;
		m_radar.normalize( );

		// get current origin.
		vec3_t cur = m_local->m_vecOrigin( );

		// get prevoius origin.
		vec3_t prev = m_net_pos.empty( ) ? cur : m_net_pos.front( ).m_pos;

		// check if we broke lagcomp.
		m_lagcomp = ( cur - prev ).length_sqr( ) > 4096.f;

		// save sent origin and time.
		m_net_pos.emplace_front( g_csgo.m_globals->m_curtime, cur );
	}

	// store some values for next tick.
	m_old_packet = *m_packet;
	m_old_shot = m_shot;

	// finish tickbase shift related code.
	g_tickbase.PostMovement( );
}

void Client::OnTick( CUserCmd* cmd ) {
	// TODO; add this to the menu.
	if( g_cfg[ XOR( "misc_reveal_ranks" ) ].get<bool>( ) && cmd->m_buttons & IN_SCORE ) {
		static CCSUsrMsg_ServerRankRevealAll msg{ };
		g_csgo.ServerRankRevealAll( &msg );
	}

	MouseDelta( );

	// store some data and update prediction.
	StartMove( cmd );

	// not much more to do here.
	if( !m_processing )
		return;

	// run all movement related code.
	DoMove( );

	// store command number at the time we fired.
	if (IsFiring(g_csgo.m_globals->m_curtime)) {
		g_cl.m_shot_command_number = cmd->m_command_number;
		g_cl.m_shot_tickbase = g_cl.m_local->m_nTickBase();

		if (!g_hvh.m_fake_duck) {
			*g_cl.m_packet = true;
		}

		if (!m_shot)
			m_shot = true;
	}

	// store stome additonal stuff for next tick
	// sanetize our usercommand if needed and fix our movement.
	EndMove( cmd );

	// restore curtime/frametime
	// and prediction seed/player.
	g_inputpred.restore( );
	g_inputpred.Misc.m_bOverrideModifier = false;

	// fix our movement.
	g_movement.FixMove( cmd, m_strafe_angles );
}

void Client::UpdateAnimations( ) {
	if( g_cl.m_lag > 0 || !g_cl.m_processing )
		return;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState( );
	if( !state )
		return;

	// update time.
	m_anim_frame = g_csgo.m_globals->m_curtime - m_anim_time;
	m_anim_time = g_csgo.m_globals->m_curtime;

	// CCSGOPlayerAnimState::Update, bypass already animated checks.
	if( state->m_iLastClientSideAnimationUpdateFramecount == g_csgo.m_globals->m_frame )
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;

	// save updated data.
	m_speed = state->m_velocity;
	m_ground = state->m_bOnGround;
}

void Client::print( const std::string text, ... ) {
	va_list     list;
	int         size;
	std::string buf;

	if( text.empty( ) )
		return;

	va_start( list, text );

	// count needed size.
	size = std::vsnprintf( 0, 0, text.c_str( ), list );

	// allocate.
	buf.resize( size );

	// print to buffer.
	std::vsnprintf( buf.data( ), size + 1, text.c_str( ), list );

	va_end( list );

	Color clr = g_cfg[ XOR( "menu_color" ) ].get_color( );

	// print to console.
	g_csgo.m_cvar->ConsoleColorPrintf( clr, XOR( "[ pandora.space ] " ) );
	g_csgo.m_cvar->ConsoleColorPrintf( colors::white, buf.append( XOR( "\n" ) ).c_str( ) );
}

bool Client::CanFireWeapon( float curtime ) {
	// the player cant fire.
	if( !m_player_fire || !m_weapon)
		return false;

	if( m_weapon->IsGrenade( ) )
		return false;

	// if we have no bullets, we cant shoot.
	if( m_weapon_type != WEAPONTYPE_KNIFE && m_weapon->m_iClip1( ) < 1 )
		return false;

	// do we have any burst shots to handle?
	if( ( m_weapon_id == GLOCK || m_weapon_id == FAMAS ) && m_weapon->m_iBurstShotsRemaining( ) > 0 ) {
		// new burst shot is coming out.
		if( curtime >= m_weapon->m_fNextBurstShot( ) )
			return true;
	}

	// r8 revolver.
	if( m_weapon_id == REVOLVER ) {
		// mouse1.
		if( m_revolver_fire ) {
			return true;
		}
		else {
			return false;
		}
	}

	// yeez we have a normal gun.
	if( curtime >= m_weapon->m_flNextPrimaryAttack( ) )
		return true;

	return false;
}

bool Client::IsFiring( float curtime ) {
	const auto weapon = g_cl.m_weapon;
	if( !weapon )
		return false;

	const auto IsZeus = m_weapon_id == Weapons_t::ZEUS;
	const auto IsKnife = !IsZeus && m_weapon_type == WEAPONTYPE_KNIFE;

	if( weapon->IsGrenade( ) )
		return !weapon->m_bPinPulled( ) && weapon->m_fThrowTime( ) > 0.f && weapon->m_fThrowTime( ) < curtime;
	else if( IsKnife )
		return ( g_cl.m_cmd->m_buttons & ( IN_ATTACK ) || g_cl.m_cmd->m_buttons & ( IN_ATTACK2 ) ) && CanFireWeapon( curtime );
	else
		return g_cl.m_cmd->m_buttons & ( IN_ATTACK ) && CanFireWeapon( curtime );
}

void Client::UpdateRevolverCock( ) {
	if (m_weapon_id != REVOLVER || !m_weapon)
		return;

	static auto last_checked = 0;
	static auto last_spawn_time = 0.f;
	static auto tick_cocked = 0;
	static auto tick_strip = 0;

	const auto max_ticks = game::TIME_TO_TICKS(.25f) - 1;
	const auto tick_base = game::TIME_TO_TICKS(g_csgo.m_globals->m_curtime);

	if (m_local->m_flSpawnTime() != last_spawn_time) {
		last_spawn_time = m_local->m_flSpawnTime();
		tick_cocked = tick_base;
		tick_strip = tick_base - max_ticks - 1;
	}

	if (m_weapon->m_flNextPrimaryAttack() > g_csgo.m_globals->m_curtime) {
		m_cmd->m_buttons &= ~IN_ATTACK;
		m_revolver_fire = false;
		return;
	}

	if (last_checked == tick_base)
		return;

	last_checked = tick_base;
	m_revolver_fire = false;

	if (tick_base - tick_strip > 2 && tick_base - tick_strip < 14)
		m_revolver_fire = true;

	if (m_cmd->m_buttons & IN_ATTACK && m_revolver_fire)
		return;

	m_cmd->m_buttons |= IN_ATTACK;

	if (m_weapon->m_flNextSecondaryAttack() >= g_csgo.m_globals->m_curtime)
		m_cmd->m_buttons |= IN_ATTACK2;

	if (tick_base - tick_cocked > max_ticks * 2 + 1) {
		tick_cocked = tick_base;
		tick_strip = tick_base - max_ticks - 1;
	}

	const auto cock_limit = tick_base - tick_cocked >= max_ticks;
	const auto after_strip = tick_base - tick_strip <= max_ticks;

	if (cock_limit || after_strip) {
		tick_cocked = tick_base;
		m_cmd->m_buttons &= ~IN_ATTACK;

		if (cock_limit)
			tick_strip = tick_base;
	}
}

void Client::UpdateIncomingSequences( ) {
	if( !g_csgo.m_net )
		return;

	if( m_sequences.empty( ) || g_csgo.m_net->m_in_seq > m_sequences.front( ).m_seq ) {
		// store new stuff.
		m_sequences.emplace_front( g_csgo.m_globals->m_realtime, g_csgo.m_net->m_in_rel_state, g_csgo.m_net->m_in_seq );
	}

	// do not save too many of these.
	while( m_sequences.size( ) > 2048 )
		m_sequences.pop_back( );
}

void Client::BuildSeedTable()
{
	for (auto i = 0; i <= 255; i++) {
		g_csgo.RandomSeed(i + 1);

		const auto pi_seed = g_csgo.RandomFloat(0.f, 6.283186f);

		g_csgo.m_computed_seeds.emplace_back(g_csgo.RandomFloat(0.f, 1.f),
			pi_seed);
	}
}
