#include "tdrs.hpp"

/**
 * tdrs namespace.
 */
namespace tdrs {
	/**
	 * @brief      Constructs the object.
	 *
	 * @param[in]  ctxn  The number of context IO threads
	 */
	HubDiscoveryServiceListener::HubDiscoveryServiceListener(_discoveryServiceListenerParams *params) {
		_params = params;
		_runLoop = true;
	}

	/**
	 * @brief      Runs the discovery service listener.
	 *
	 * @return     NULL
	 */
	void HubDiscoveryServiceListener::run() {
		std::cout << "Running discovery service listener ..." << std::endl;
		zyre::node_t _zyreListenerNode;
		std::cout << "Adding node for discovery service listener ..." << std::endl;
		_zyreListenerNode = zyre::node_t("nodeOne");
		std::cout << "Setting header to discovery service listener ..." << std::endl;
		_zyreListenerNode.set_header("X-HELLO", "World");
		// _zyreListenerNode.set_verbose();
		std::cout << "Starting node for discovery service listener ..." << std::endl;
		_zyreListenerNode.start();
		std::cout << "Joining group as discovery service listener ..." << std::endl;
		_zyreListenerNode.join("TDRS");

		while(_params->run == true) {
			std::cout << "Listening for discovery service events ..." << std::endl;
			zyre::event_t zyreEvent = _zyreListenerNode.event();
			std::cout << "Got discovery service event ..." << std::endl;
		}

		_zyreListenerNode.leave("TDRS");
		_zyreListenerNode.stop();

		delete _params;
	}
}
