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

		zeroAddress *publisherAddress = Hub::parseZeroAddress(_params->publisher);
		zeroAddress *receiverAddress = Hub::parseZeroAddress(_params->receiver);
		zmq::context_t zmqContext(1);

		std::cout << "DL: Connecting to receiver at " << _params->receiver << " ..." << std::endl;
		int _zmqSenderSocketLinger = 0;
		zmq::socket_t _zmqSenderSocket(zmqContext, ZMQ_REQ);
		_zmqSenderSocket.setsockopt(ZMQ_LINGER, &_zmqSenderSocketLinger, sizeof(_zmqSenderSocketLinger));
		_zmqSenderSocket.connect(_params->receiver);

		zyre::node_t _zyreListenerNode;
		std::cout << "DL: Adding node for discovery service listener ..." << std::endl;
		_zyreListenerNode = zyre::node_t(zsys_hostname());

		if(!_params->interface.empty()) {
			_zyreListenerNode.set_interface(_params->interface);
		}
		_zyreListenerNode.set_port(_params->port);
		_zyreListenerNode.set_interval(_params->interval);

		std::cout << "DL: Setting header to discovery service listener ..." << std::endl;
		_zyreListenerNode.set_header("X-PUB-PTCL", publisherAddress->protocol);
		_zyreListenerNode.set_header("X-PUB-ADDR", publisherAddress->address);
		_zyreListenerNode.set_header("X-PUB-PORT", publisherAddress->port);
		_zyreListenerNode.set_header("X-REC-PTCL", receiverAddress->protocol);
		_zyreListenerNode.set_header("X-REC-ADDR", receiverAddress->address);
		_zyreListenerNode.set_header("X-REC-PORT", receiverAddress->port);
		_zyreListenerNode.set_header("X-KEY", Hub::hashString(&_params->key));
		// _zyreListenerNode.set_verbose();
		std::cout << "DL: Starting node for discovery service listener ..." << std::endl;
		_zyreListenerNode.start();
		std::cout << "DL: Joining group as discovery service listener ..." << std::endl;
		_zyreListenerNode.join(_params->group);

		std::cout << "DL: Listening for discovery service events ..." << std::endl;
		while(_params->run == true) {
			zyre::event_t zyreEvent = _zyreListenerNode.event();

			std::cout << "DL: Got discovery service event ..." << std::endl;
			std::string eventType                    = zyreEvent.type();
			std::string eventSenderId                = zyreEvent.sender();
			std::string eventSenderName              = zyreEvent.name();
			std::string eventSenderAddressZyre       = zyreEvent.address();
			zeroAddress *eventSenderZyreAddress 	 = Hub::parseZeroAddress(eventSenderAddressZyre);
			std::string eventSenderPublisherProtocol = zyreEvent.header_value("X-PUB-PTCL");
			std::string eventSenderPublisherAddress  = zyreEvent.header_value("X-PUB-ADDR");
			std::string eventSenderPublisherPort     = zyreEvent.header_value("X-PUB-PORT");
			std::string eventSenderReceiverProtocol  = zyreEvent.header_value("X-REC-PTCL");
			std::string eventSenderReceiverAddress   = zyreEvent.header_value("X-REC-ADDR");
			std::string eventSenderReceiverPort      = zyreEvent.header_value("X-REC-PORT");
			std::string eventSenderKey               = zyreEvent.header_value("X-KEY");
			std::string eventGroup                   = zyreEvent.group();

			zyreEvent.print();

			std::string message;
			if(eventType == "ENTER") {
				if(Hub::hashString(&_params->key) != eventSenderKey) {
					std::cout << "DL: Ignoring discovery service event, as key does not fit." << std::endl;
					continue;
				}

				message = "PEER:ENTER:" + eventSenderId + \
										":" + eventSenderPublisherProtocol + \
										":" + eventSenderZyreAddress->address + \
										":" + eventSenderPublisherPort + \
										":" + eventSenderReceiverProtocol + \
										":" + eventSenderZyreAddress->address + \
										":" + eventSenderReceiverPort;
			} else if(eventType == "EXIT") {
				message = "PEER:EXIT:" + eventSenderId + \
										":*" \
										":*" \
										":*" \
										":*" \
										":*" \
										":*" ;
			} else {
				std::cout << "DL: Ignoring unhandled discovery service event." << std::endl;
				continue;
			}


			zmq::message_t zmqSenderMessageOutgoing(message.size());
			memcpy(zmqSenderMessageOutgoing.data(), message.c_str(), message.size());

			_zmqSenderSocket.send(zmqSenderMessageOutgoing);

			zmq::message_t zmqSenderMessageIncoming;
			try {
				_zmqSenderSocket.recv(&zmqSenderMessageIncoming);
			} catch(...) {
				continue;
			}

			std::string zmqSenderMessageIncomingString(
				static_cast<const char*>(zmqSenderMessageIncoming.data()),
				zmqSenderMessageIncoming.size()
			);

			if(zmqSenderMessageIncomingString.substr(0, 3) == "OOK") {
				std::cout << "DL: Receiver responded with success." << std::endl;
			} else {
				std::cout << "DL: Receiver responded with failure!" << std::endl;
			}
		}

		std::cout << "DL: Leaving group ..." << std::endl;
		_zyreListenerNode.leave(_params->group);

		std::cout << "DL: Stopping node ..." << std::endl;
		_zyreListenerNode.stop();

		delete _params;
		std::cout << "DL: Goodbye!" << std::endl;
	}
}
