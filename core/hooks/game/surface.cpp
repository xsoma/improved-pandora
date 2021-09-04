#include "../../../includes.h"

void Hooks::LockCursor()
{
	if (GUI::ctx->open)
	{
		g_csgo.m_surface->UnlockCursor();

		static bool disabled = false;

		if (!disabled)
		{
			if (GUI::ctx->typing)
			{
				g_csgo.m_input_system->EnableInput(false);
				disabled = true;
			}
		}

		if (!GUI::ctx->typing && disabled)
		{
			g_csgo.m_input_system->EnableInput(true);
			disabled = false;
		}
	}
	else
	{
		g_hooks.m_surface.GetOldMethod< LockCursor_t >(ISurface::LOCKCURSOR)(this);

		g_csgo.m_input_system->EnableInput(true);
	}
}

void Hooks::PlaySound(const char* name)
{
	g_hooks.m_surface.GetOldMethod< PlaySound_t >(ISurface::PLAYSOUND)(this, name);
}

void Hooks::OnScreenSizeChanged(int oldwidth, int oldheight)
{
	g_hooks.m_surface.GetOldMethod< OnScreenSizeChanged_t >(ISurface::ONSCREENSIZECHANGED)(this, oldwidth, oldheight);

	render::init();

	g_visuals.ModulateWorld();
}