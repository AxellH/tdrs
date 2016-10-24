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
		std::cout << "DL: Running discovery service listener ..." << std::endl;
		zyre::node_t _zyreListenerNode;
		std::cout << "DL: Adding node for discovery service listener ..." << std::endl;
		_zyreListenerNode = zyre::node_t("nodeOne");
		std::cout << "DL: Setting header to discovery service listener ..." << std::endl;
		_zyreListenerNode.set_header("X-HELLO", "World");
		// _zyreListenerNode.set_verbose();
		std::cout << "DL: Starting node for discovery service listener ..." << std::endl;
		_zyreListenerNode.start();
		std::cout << "DL: Joining group as discovery service listener ..." << std::endl;
		_zyreListenerNode.join("TDRS");

		std::cout << "DL: Listening for discovery service events ..." << std::endl;
		while(_params->run == true) {
			zyre::event_t zyreEvent = _zyreListenerNode.event();
			std::cout << "DL: Got discovery service event:" << std::endl;
			std::cout << "DL: type: " << zyreEvent.type() << std::endl;
			std::cout << "DL: sender: " << zyreEvent.sender() << std::endl;
			std::cout << "DL: name: " << zyreEvent.name() << std::endl;
			std::cout << "DL: address: " << zyreEvent.address() << std::endl;
			std::cout << "DL: group: " << zyreEvent.group() << std::endl;
			// zyreEvent.print();
		}

		std::cout << "DL: Leaving group ..." << std::endl;
		_zyreListenerNode.leave("TDRS");

		std::cout << "DL: Stopping node ..." << std::endl;
		_zyreListenerNode.stop();

		delete _params;
		std::cout << "DL: Goodbye!" << std::endl;
	}
}
