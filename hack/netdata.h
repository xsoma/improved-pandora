#pragma once

class NetData
{
private:
	class stored_data_t
	{
	public:
		int m_nTickBase;
		ang_t m_aimPunchAngle;
		ang_t m_aimPunchAngleVel;
		ang_t m_viewPunchAngle;
		vec3_t m_vecViewOffset;
		float m_flVelocityModifier; // later remove this...
	public:
		__forceinline stored_data_t() : m_nTickBase{}, m_aimPunchAngle{}, m_aimPunchAngleVel{}, m_vecViewOffset{}, m_flVelocityModifier{}/*later remove this...*/{};
	};

	std::array<stored_data_t, MULTIPLAYER_BACKUP> m_data;
public:
	void store_netvars();
	void restore_netvars();
	void reset_netvars();
};

extern NetData g_netdata;