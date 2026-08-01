#include "Application/Application.h"
#include <string_view>

int main(const int ArgumentCount, char** const Arguments)
{
	bool bSmokeTest = false;
	ProjectTemplate::EWindowPlatform WindowPlatform = ProjectTemplate::EWindowPlatform::Default;
	for (int Index = 1; Index < ArgumentCount; Index++)
	{
		const std::string_view Argument = Arguments[Index];
		if (Argument == "--smoke-test")
		{
			bSmokeTest = true;
		}
		else if (Argument == "--platform=x11")
		{
			WindowPlatform = ProjectTemplate::EWindowPlatform::X11;
		}
		else if (Argument == "--platform=wayland")
		{
			WindowPlatform = ProjectTemplate::EWindowPlatform::Wayland;
		}
	}

	return ProjectTemplate::RunApplication(bSmokeTest, WindowPlatform);
}
