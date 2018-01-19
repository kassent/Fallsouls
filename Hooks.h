#pragma once

class GFxMovieView;
class GFxValue;

namespace FallSouls
{
	void LoadSettings();

	void InstallHooks();

	bool ScaleformCallback(GFxMovieView * view, GFxValue * value);
}