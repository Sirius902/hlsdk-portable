#include "hud.h"
#include "cl_util.h"
#include <math.h>

extern "C" {
	extern float g_flJumpSpeed;
	extern int g_iJumpOccurred;
}

extern int g_iUser1;
extern vec3_t v_simvel;

int CHudSpeedometer::Init( void )
{
	m_pCvarSpeedometer = CVAR_CREATE( "hud_speedometer", "0", FCVAR_ARCHIVE );
	m_pCvarJumpSpeed = CVAR_CREATE( "hud_speedometer_jumpspeed", "0", FCVAR_ARCHIVE );

	m_flJumpSpeed = 0.0f;
	m_flPrevJumpSpeed = 0.0f;
	m_flJumpSpeedFlashTime = 0.0f;
	m_iJumpSpeedColor = 0;

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );

	return 1;
}

int CHudSpeedometer::VidInit( void )
{
	return 1;
}

int CHudSpeedometer::Draw( float flTime )
{
	if( !m_pCvarSpeedometer->value )
		return 1;

	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	if( g_iUser1 )
		return 1;

	float speed = sqrt( v_simvel[0] * v_simvel[0] + v_simvel[1] * v_simvel[1] );

	// Jump speed tracking
	if( g_iJumpOccurred )
	{
		g_iJumpOccurred = 0;

		m_flPrevJumpSpeed = m_flJumpSpeed;

		m_flJumpSpeed = g_flJumpSpeed;
		m_flJumpSpeedFlashTime = flTime;

		if( m_flJumpSpeed > m_flPrevJumpSpeed )
			m_iJumpSpeedColor = 1; // green
		else if( m_flJumpSpeed < m_flPrevJumpSpeed )
			m_iJumpSpeedColor = 2; // red
		else
			m_iJumpSpeedColor = 0; // neutral
	}

	int r, g, b;
	UnpackRGB( r, g, b, RGB_YELLOWISH );

	// Draw current speed centered at bottom using HUD number sprites
	int iSpeed = (int)( speed + 0.5f );
	int iDigitWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	int iNumDigits = gHUD.GetNumWidth( iSpeed, DHN_DRAWZERO );
	int iTotalWidth = iNumDigits * iDigitWidth;
	int x = ( ScreenWidth - iTotalWidth ) / 2;
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	gHUD.DrawHudNumber( x, y + gHUD.m_iHudNumbersYOffset, DHN_DRAWZERO, iSpeed, r, g, b );

	// Draw jump speed if enabled
	if( m_pCvarJumpSpeed->value )
	{
		int jr, jg, jb;
		UnpackRGB( jr, jg, jb, RGB_YELLOWISH );

		float elapsed = flTime - m_flJumpSpeedFlashTime;
		float fadeDuration = 1.0f;

		if( m_iJumpSpeedColor != 0 && elapsed < fadeDuration )
		{
			float factor = elapsed / fadeDuration;
			int fr, fg, fb;

			if( m_iJumpSpeedColor == 1 )
			{
				fr = 0; fg = 255; fb = 0;
			}
			else
			{
				fr = 255; fg = 0; fb = 0;
			}

			jr = fr + (int)( ( jr - fr ) * factor );
			jg = fg + (int)( ( jg - fg ) * factor );
			jb = fb + (int)( ( jb - fb ) * factor );
		}

		int iJumpSpeed = (int)( m_flJumpSpeed + 0.5f );
		int iJsDigits = gHUD.GetNumWidth( iJumpSpeed, DHN_DRAWZERO );
		int iJsWidth = iJsDigits * iDigitWidth;
		int jx = ( ScreenWidth - iJsWidth ) / 2;
		int jy = y - gHUD.m_iFontHeight;

		gHUD.DrawHudNumber( jx, jy + gHUD.m_iHudNumbersYOffset, DHN_DRAWZERO, iJumpSpeed, jr, jg, jb );
	}

	return 1;
}
