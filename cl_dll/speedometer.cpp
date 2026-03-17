#include "hud.h"
#include "cl_util.h"
#include <math.h>

extern "C" {
	extern float g_flJumpSpeed;
	extern int g_iJumpOccurred;
}

extern int g_iUser1;
extern vec3_t v_simvel;

static float RGBtoHue( int r, int g, int b )
{
	float fr = r / 255.0f, fg = g / 255.0f, fb = b / 255.0f;
	float maxc = Q_max( fr, Q_max( fg, fb ) );
	float minc = Q_min( fr, Q_min( fg, fb ) );
	float delta = maxc - minc;
	if( delta == 0 ) return 0;
	float hue;
	if( maxc == fr )      hue = 60.0f * fmod( ( fg - fb ) / delta, 6.0f );
	else if( maxc == fg ) hue = 60.0f * ( ( fb - fr ) / delta + 2.0f );
	else                  hue = 60.0f * ( ( fr - fg ) / delta + 4.0f );
	if( hue < 0 ) hue += 360.0f;
	return hue;
}

static void HueToRGB( float hue, int &r, int &g, int &b )
{
	float h = fmod( hue, 360.0f );
	if( h < 0 ) h += 360.0f;
	int sector = (int)( h / 60.0f );
	float f = h / 60.0f - sector;
	int q = (int)( ( 1.0f - f ) * 255 );
	int t = (int)( f * 255 );
	switch( sector ) {
	case 0: r = 255; g = t;   b = 0;   break;
	case 1: r = q;   g = 255; b = 0;   break;
	case 2: r = 0;   g = 255; b = t;   break;
	case 3: r = 0;   g = q;   b = 255; break;
	case 4: r = t;   g = 0;   b = 255; break;
	default:r = 255; g = 0;   b = q;   break;
	}
}

static float HueDist( float a, float b )
{
	float d = fabs( a - b );
	return d > 180.0f ? 360.0f - d : d;
}

static int CountDigits( int n )
{
	if( n <= 0 ) return 1;
	int d = 0;
	while( n > 0 ) { d++; n /= 10; }
	return d;
}

static void DrawSpeedNumber( int x, int y, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	int digits[16];
	int nDigits = 0;

	if( iNumber <= 0 )
	{
		digits[0] = 0;
		nDigits = 1;
	}
	else
	{
		int n = iNumber;
		while( n > 0 )
		{
			digits[nDigits++] = n % 10;
			n /= 10;
		}
	}

	for( int i = nDigits - 1; i >= 0; i-- )
	{
		int k = digits[i];
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x += iWidth;
	}
}

int CHudSpeedometer::Init( void )
{
	m_pCvarSpeedometer = CVAR_CREATE( "hud_speedometer", "0", FCVAR_ARCHIVE );
	m_pCvarJumpSpeed = CVAR_CREATE( "hud_speedometer_jumpspeed", "0", FCVAR_ARCHIVE );

	m_flJumpSpeed = 0.0f;
	m_flPrevJumpSpeed = 0.0f;
	m_flJumpSpeedFadeRemaining = 0.0f;
	m_iJumpSpeedColor = 0;

	m_iLastBaseColor = 0;

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
		m_flJumpSpeedFadeRemaining = 1.0f;

		if( m_flJumpSpeed > m_flPrevJumpSpeed )
			m_iJumpSpeedColor = 1; // green
		else if( m_flJumpSpeed < m_flPrevJumpSpeed )
			m_iJumpSpeedColor = 2; // red
		else
			m_iJumpSpeedColor = 0; // neutral
	}

#ifdef HIDEHUD_NOHEV
	int baseColor = ( gHUD.m_iHideHUDDisplay & HIDEHUD_NOHEV ) ? RGB_REDISH : RGB_YELLOWISH;
#else
	int baseColor = RGB_YELLOWISH;
#endif

	if( baseColor != m_iLastBaseColor )
	{
		m_iLastBaseColor = baseColor;

		int hr, hg, hb;
		UnpackRGB( hr, hg, hb, baseColor );
		float baseHue = RGBtoHue( hr, hg, hb );

		float goodHue = 120.0f;
		if( HueDist( goodHue, baseHue ) < 35.0f )
			goodHue = fmod( baseHue + 120.0f, 360.0f );
		HueToRGB( goodHue, m_iFlashGoodR, m_iFlashGoodG, m_iFlashGoodB );

		float badHue = 0.0f;
		if( HueDist( badHue, baseHue ) < 35.0f )
			badHue = fmod( baseHue - 120.0f + 360.0f, 360.0f );
		HueToRGB( badHue, m_iFlashBadR, m_iFlashBadG, m_iFlashBadB );
	}

	int r, g, b;
	UnpackRGB( r, g, b, baseColor );

	// Draw current speed centered at bottom using HUD number sprites
	int iSpeed = (int)( speed + 0.5f );
	int iDigitWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	int iNumDigits = CountDigits( iSpeed );
	int iTotalWidth = iNumDigits * iDigitWidth;
	int x = ( ScreenWidth - iTotalWidth ) / 2;
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	DrawSpeedNumber( x, y + gHUD.m_iHudNumbersYOffset, iSpeed, r, g, b );

	// Draw jump speed if enabled
	if( m_pCvarJumpSpeed->value )
	{
		int jr, jg, jb;
		UnpackRGB( jr, jg, jb, baseColor );

		if( m_flJumpSpeedFadeRemaining > 0.0f )
			m_flJumpSpeedFadeRemaining -= gHUD.m_flTimeDelta;

		if( m_iJumpSpeedColor != 0 && m_flJumpSpeedFadeRemaining > 0.0f )
		{
			float factor = 1.0f - m_flJumpSpeedFadeRemaining;
			int fr, fg, fb;

			if( m_iJumpSpeedColor == 1 )
			{
				fr = m_iFlashGoodR; fg = m_iFlashGoodG; fb = m_iFlashGoodB;
			}
			else
			{
				fr = m_iFlashBadR; fg = m_iFlashBadG; fb = m_iFlashBadB;
			}

			jr = fr + (int)( ( jr - fr ) * factor );
			jg = fg + (int)( ( jg - fg ) * factor );
			jb = fb + (int)( ( jb - fb ) * factor );
		}

		int iJumpSpeed = (int)( m_flJumpSpeed + 0.5f );
		int iJsDigits = CountDigits( iJumpSpeed );
		int iJsWidth = iJsDigits * iDigitWidth;
		int jx = ( ScreenWidth - iJsWidth ) / 2;
		int jy = y - gHUD.m_iFontHeight;

		DrawSpeedNumber( jx, jy + gHUD.m_iHudNumbersYOffset, iJumpSpeed, jr, jg, jb );
	}

	return 1;
}
