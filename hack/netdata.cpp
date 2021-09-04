#include "../includes.h"

NetData g_netdata{};;

void NetData::store_netvars()
{
    stored_data_t* data;

    if (!g_cl.m_processing)
    {
        reset_netvars();
        return;
    }

    data = &m_data[g_csgo.m_cl->iCommandAck % MULTIPLAYER_BACKUP];

    data->m_nTickBase = g_cl.m_local->m_nTickBase();
    data->m_aimPunchAngle = g_cl.m_local->m_aimPunchAngle();
    data->m_aimPunchAngleVel = g_cl.m_local->m_aimPunchAngleVel();
    data->m_viewPunchAngle = g_cl.m_local->m_viewPunchAngle();
    data->m_vecViewOffset = g_cl.m_local->m_vecViewOffset();
    data->m_flVelocityModifier = g_cl.m_local->m_flVelocityModifier(); // later remove this...
}

void NetData::restore_netvars()
{
    stored_data_t* data;
    ang_t aim_punch_angle_delta, aim_punch_angle_vel_delta, view_punch_angle_delta;
    vec3_t view_offset_delta;
    float velocity_modifier_delta;

    if (!g_cl.m_processing)
    {
        reset_netvars();
        return;
    }

    data = &m_data[(g_csgo.m_cl->iCommandAck - 1) % MULTIPLAYER_BACKUP];

    if (data->m_nTickBase != g_cl.m_local->m_nTickBase())
        return;

    aim_punch_angle_delta = g_cl.m_local->m_aimPunchAngle() - data->m_aimPunchAngle;
    aim_punch_angle_vel_delta = g_cl.m_local->m_aimPunchAngleVel() - data->m_aimPunchAngleVel;
    view_punch_angle_delta = g_cl.m_local->m_viewPunchAngle() - data->m_viewPunchAngle;
    view_offset_delta = g_cl.m_local->m_vecViewOffset() - data->m_vecViewOffset;
    velocity_modifier_delta = g_cl.m_local->m_flVelocityModifier() - data->m_flVelocityModifier; // later remove this...

    if (fabs(aim_punch_angle_delta.x) < 0.03125f && fabs(aim_punch_angle_delta.y) < 0.03125f && fabs(aim_punch_angle_delta.z) < 0.03125f)
        g_cl.m_local->m_aimPunchAngle() = data->m_aimPunchAngle;

    if (fabs(aim_punch_angle_vel_delta.x) < 0.03125f && fabs(aim_punch_angle_vel_delta.y) < 0.03125f && fabs(aim_punch_angle_vel_delta.z) < 0.03125f)
        g_cl.m_local->m_aimPunchAngleVel() = data->m_aimPunchAngleVel;

    if (fabs(view_punch_angle_delta.x) < 0.03125f && fabs(view_punch_angle_delta.y) < 0.03125f && fabs(view_punch_angle_delta.z) < 0.03125f)
        g_cl.m_local->m_viewPunchAngle() = data->m_viewPunchAngle;

    if (fabs(view_offset_delta.x) < 0.03125f && fabs(view_offset_delta.y) < 0.03125f && fabs(view_offset_delta.z) < 0.03125f)
        g_cl.m_local->m_vecViewOffset() = data->m_vecViewOffset;

    if (fabs(velocity_modifier_delta) < 0.03125f) // later remove this...
        g_cl.m_local->m_flVelocityModifier() = data->m_flVelocityModifier; 
}

void NetData::reset_netvars()
{
	m_data.fill(stored_data_t());
}