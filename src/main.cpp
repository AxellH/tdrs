#include "tdrs.hpp"

// Initializes Hub.
tdrs::Hub hub(1);

/**
 * @brief      Handles signals sent to the main process.
 *
 * @param[in]  signalNumber  The signal number
 */
void signalHandler(int signalNumber) {
	std::cout << std::endl << "Main: Caught signal " << signalNumber << std::endl;
	switch(signalNumber) {
		case 2:
			hub.shutdown();
			break;
	}
	return;
}

/**
 * @brief      Program entrypoint.
 *
 * @return     0
 */
int main(void)
{
	std::cout << "Main: Starting ..." << std::endl;

	struct sigaction mainSignalHandler;
	mainSignalHandler.sa_handler = signalHandler;
	sigemptyset(&mainSignalHandler.sa_mask);
	mainSignalHandler.sa_flags = 0;
	sigaction(SIGINT, &mainSignalHandler, NULL);

	hub.run();

	std::cout << "Main: Quit." << std::endl;
	return(0);
}
