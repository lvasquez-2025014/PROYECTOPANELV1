#include <imgui.h>
#include <src/Fonts/Fonts.hpp>

// El ImGui de ext/ esta modificado y referencia las fuentes como
// variables globales. Estas definiciones conectan esos simbolos
// globales con las variables de FWork::Fonts.

ImFont* FontAwesomeRegular;
ImFont* FontAwesomeSolid;
ImFont* FontAwesomeSolid14;
ImFont* FontAwesomeBrands;
ImFont* FontAwesomeSolidBig;

ImFont* InterBlack;
ImFont* InterBold;
ImFont* InterBold12;
ImFont* InterExtraBold;
ImFont* InterExtraLight;
ImFont* InterLight;
ImFont* InterMedium;
ImFont* InterRegular;
ImFont* InterSemiBold;
ImFont* InterThin;

namespace FWork {
	namespace Fonts {
		void BindImGuiGlobals() {
			::FontAwesomeRegular = FontAwesomeRegular;
			::FontAwesomeSolid = FontAwesomeSolid;
			::FontAwesomeSolid14 = FontAwesomeSolid18;
			::FontAwesomeBrands = FontAwesomeRegular;
			::FontAwesomeSolidBig = FontAwesomeSolidBig;

			::InterBlack = InterBlack;
			::InterBold = InterBold;
			::InterBold12 = InterBold12;
			::InterExtraBold = InterExtraBold;
			::InterExtraLight = InterExtraLight;
			::InterLight = InterLight;
			::InterMedium = InterMedium;
			::InterRegular = InterRegular;
			::InterSemiBold = InterSemiBold;
			::InterThin = InterThin;
		}
	}
}
