#include "application.h"

#include <iostream>

int main(int argc, char **argv)
{
	MBRF::Application app;

	app.ParseCommandLineArguments(argc, argv);

	app.Run();

	return 0;
}