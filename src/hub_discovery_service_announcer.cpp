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
	HubDiscoveryServiceAnnouncer::HubDiscoveryServiceAnnouncer(_discoveryServiceAnnouncerParams *params) {
		_params = params;
		_runLoop = true;
	}

	/**
	 * @brief      Runs the discovery service announcer.
	 *
	 * @return     NULL
	 */
	void HubDiscoveryServiceAnnouncer::run() {
		std::cout << "DA: Running discovery service announcer ..." << std::endl;
		zyre::node_t _zyreAnnouncerNode;

		std::cout << "DA: Adding node for discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode = zyre::node_t("nodeOne");
		std::cout << "DA: Setting header of discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode.set_header("X-HELLO", "World");
		// _zyreAnnouncerNode.set_verbose();
		std::cout << "DA: Starting node for discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode.start();
		std::cout << "DA: Joining group as discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode.join("TDRS");

		std::cout << "DA: Looping discovery service announcer ..." << std::endl;
		while(_params->run == true) {
			zclock_sleep(1000);

			zmsg_t *msg = zmsg_new();
			zframe_t *shoutFrame = zframe_new("HELLO", 5);
		    zmsg_append(msg, &shoutFrame);

			_zyreAnnouncerNode.shout("TDRS", msg);
			std::cout << "DA: Service announced." << std::endl;

			// zmsg_destroy(&msg);
		}

		std::cout << "DA: Leaving group ..." << std::endl;
		_zyreAnnouncerNode.leave("TDRS");

		std::cout << "DA: Stopping node ..." << std::endl;
		_zyreAnnouncerNode.stop();

		delete _params;
		std::cout << "DA: Goodbye!" << std::endl;
	}
}
