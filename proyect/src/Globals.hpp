#pragma once

namespace FWork {

	struct EspConfig {
		int Width = 0;
		int Height = 0;
		bool Enabled = false;
	};

	struct Globals {
		EspConfig EspConfig;
	};

	inline Globals g_Globals;
}
