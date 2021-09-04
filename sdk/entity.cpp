#include "../includes.h"

/*float Player::get_max_desync_delta()
{
	if (!this)
		return 0.0f;

	auto animstate = m_PlayerAnimState();

	if (!animstate)
		return 0.0f;

	auto speedfactor = math::clamp(animstate->m_flFeetSpeedForwardsOrSideways, 0.0f, 1.0f);
	auto avg_speedfactor = (animstate->m_flStopToFullRunningFraction * -0.30000001f - 0.19999999f) * speedfactor + 1.f;

	if (animstate->m_flDuckAmount > 0.0f)
		avg_speedfactor += ((animstate->m_flDuckAmount * speedfactor) * (0.5f - avg_speedfactor));

	return animstate->m_flMaxYaw * avg_speedfactor;
}*/

void Player::ModifyEyePosition( CCSGOPlayerAnimState* state, vec3_t* pos ) {
	if( !this )
		return;

	// The local player sets up their third-person bones to locate the position of their head,
	// then this position is used to softly bound the vertical camera position for the client.
	if( !state->m_bInHitGroundAnimation && state->m_fDuckAmount == 0 )
		return;

	int nHeadBone = 8;
	if( nHeadBone != -1 ) {
		vec3_t vecHeadPos(
			state->m_pBaseEntity->m_BoneCache( ).m_pCachedBones[ 8 ][ 0 ][ 3 ],
			state->m_pBaseEntity->m_BoneCache( ).m_pCachedBones[ 8 ][ 1 ][ 3 ],
			state->m_pBaseEntity->m_BoneCache( ).m_pCachedBones[ 8 ][ 2 ][ 3 ] );

		vecHeadPos.z += 1.7f;

		// Only correct the eye if the camera is ABOVE the head. If the camera is below the head, that's unlikely
		// to be advantageous for the local player.
		if( vecHeadPos.z < pos->z ) {
			constexpr float FIRSTPERSON_TO_THIRDPERSON_VERTICAL_TOLERANCE_MIN = 4.0f;
			constexpr float FIRSTPERSON_TO_THIRDPERSON_VERTICAL_TOLERANCE_MAX = 10.0f;

			float flLerp = math::SimpleSplineRemapValClamped( abs( pos->z - vecHeadPos.z ),
				FIRSTPERSON_TO_THIRDPERSON_VERTICAL_TOLERANCE_MIN,
				FIRSTPERSON_TO_THIRDPERSON_VERTICAL_TOLERANCE_MAX,
				0.0f, 1.0f );

			pos->z = math::Lerp( flLerp, pos->z, vecHeadPos.z );
		}
	}
}