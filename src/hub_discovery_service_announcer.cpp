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
		std::cout << "Running discovery service announcer ..." << std::endl;
		zyre::node_t _zyreAnnouncerNode;

		std::cout << "Adding node for discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode = zyre::node_t("nodeOne");
		std::cout << "Setting header of discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode.set_header("X-HELLO", "World");
		// _zyreAnnouncerNode.set_verbose();
		std::cout << "Starting node for discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode.start();
		std::cout << "Joining group as discovery service announcer ..." << std::endl;
		_zyreAnnouncerNode.join("TDRS");

		while(_params->run == true) {
			zclock_sleep(1000);

			std::cout << "Looping discovery service announcer ..." << std::endl;

			zmsg_t *msg = zmsg_new();
			zframe_t *shoutFrame = zframe_new("HELLO", 6);
		    zmsg_append(msg, &shoutFrame);

			_zyreAnnouncerNode.shout("TDRS", msg);

			zmsg_destroy(&msg);
		}

		_zyreAnnouncerNode.leave("TDRS");
		_zyreAnnouncerNode.stop();

		delete _params;
	}
}
