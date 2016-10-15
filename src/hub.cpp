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
	Hub::Hub(int ctxn) : _zmqContext(ctxn), _zmqHubSocket(_zmqContext, ZMQ_PUB), _zmqReceiverSocket(_zmqContext, ZMQ_REP) {
		_runLoop = true;
	}

	/**
	 * @brief      Binds the publisher.
	 */
	void Hub::_bindPublisher() {
		std::cout << "Hub: Binding publisher ..." << std::endl;
		int _zmqHubSocketLinger = 0;
		_zmqHubSocket.setsockopt(ZMQ_LINGER, &_zmqHubSocketLinger, sizeof(_zmqHubSocketLinger));
		_zmqHubSocket.bind(_optionPublisherListen);
		std::cout << "Hub: Bound publisher." << std::endl;
	}

	/**
	 * @brief      Unbinds (closes) the publisher.
	 */
	void Hub::_unbindPublisher() {
		std::cout << "Hub: Sending termination to subscribers ..." << std::endl;
		_zmqHubSocket.send("TERMINATE", 9, 0);
		std::cout << "Hub: Sent termination to subscribers." << std::endl;
		std::cout << "Hub: Unbinding publisher ..." << std::endl;
		_zmqHubSocket.close();
		std::cout << "Hub: Unbound publisher." << std::endl;
	}

	/**
	 * @brief      Binds the receiver.
	 */
	void Hub::_bindReceiver() {
		std::cout << "Hub: Binding receiver ..." << std::endl;
		int _zmqReceiverSocketLinger = 0;
		_zmqReceiverSocket.setsockopt(ZMQ_LINGER, &_zmqReceiverSocketLinger, sizeof(_zmqReceiverSocketLinger));
		_zmqReceiverSocket.bind(_optionReceiverListen);
		std::cout << "Hub: Bound receiver." << std::endl;
	}

	/**
	 * @brief      Unbinds (closes) the receiver.
	 */
	void Hub::_unbindReceiver() {
		std::cout << "Hub: Unbinding receiver ..." << std::endl;
		_zmqReceiverSocket.close();
		std::cout << "Hub: Unbound receiver." << std::endl;
	}

	/**
	 * @brief      Sets the Hub options.
	 *
	 * @param[in]  argc  The main argc
	 * @param      argv  The main argv
	 *
	 * @return     True on success, false on failure.
	 */
	bool Hub::options(int argc, char *argv[]) {
		try {
			bpo::options_description optionsDescription("Options:");
			optionsDescription.add_options()
				("help", "show this usage information")
				("receiver-listen", bpo::value<std::string>(), "set listener for receiver")
				("publisher-listen", bpo::value<std::string>(), "set listener for publisher")
			;

			bpo::variables_map variablesMap;
			bpo::store(bpo::parse_command_line(argc, argv, optionsDescription), variablesMap);
			bpo::notify(variablesMap);

			if (variablesMap.count("help")) {
				std::cout << optionsDescription << std::endl;
				return false;
			}

			if (variablesMap.count("receiver-listen")) {
				_optionReceiverListen = variablesMap["receiver-listen"].as<std::string>();
				std::cout << "Hub: Listener for receiver was set to " << _optionReceiverListen << std::endl;
			} else {
				std::cout << "Hub: Listener for receiver (--receiver-listen) was not set!" << std::endl;
				return false;
			}

			if (variablesMap.count("publisher-listen")) {
				_optionPublisherListen = variablesMap["publisher-listen"].as<std::string>();
				std::cout << "Hub: Listener for publisher was set to " << _optionPublisherListen << std::endl;
			} else {
				std::cout << "Hub: Listener for publisher (--publisher-listen) was not set!" << std::endl;
				return false;
			}
		} catch(...) {
			return false;
		}

		return true;
	}

	/**
	 * @brief      Requests an exit of the run-loop on its next iteration.
	 */
	void Hub::shutdown() {
		_runLoop = false;
	}

	/**
	 * @brief      Runs the Hub.
	 */
	void Hub::run() {
		// Bind the publisher
		_bindPublisher();
		// Bind the receiver
		_bindReceiver();

		// Run loop
		while(_runLoop == true) {
			zmq::message_t zmqReceiverMessageIncoming;

			try {
				_zmqReceiverSocket.recv(&zmqReceiverMessageIncoming);
			} catch(...) {
				continue;
			}

			std::string zmqReceiverMessageIncomingString(
				static_cast<const char*>(zmqReceiverMessageIncoming.data()),
				zmqReceiverMessageIncoming.size()
			);

			std::cout << "Hub: Received message: " << zmqReceiverMessageIncomingString << std::endl;

			std::cout << "Hub: Forwarding message to Hub subscribers ..." << std::endl;
			zmq::message_t zmqIpcMessageOutgoing(zmqReceiverMessageIncoming.size());
			memcpy(zmqIpcMessageOutgoing.data(), zmqReceiverMessageIncoming.data(), zmqReceiverMessageIncoming.size());

			std::string zmqReceiverMessageOutgoingString;
			try {
				_zmqHubSocket.send(zmqIpcMessageOutgoing);
				zmqReceiverMessageOutgoingString = "OK";
				std::cout << "Hub: Forwarding successful." << std::endl;
			} catch(...) {
				zmqReceiverMessageOutgoingString = "NOK";
				std::cout << "Hub: Forwarding failed!" << std::endl;
			}

			std::cout << "Hub: Preparing response to initiator ..." << std::endl;
			zmq::message_t zmqReceiverMessageOutgoing(zmqReceiverMessageOutgoingString.size());
			memcpy(zmqReceiverMessageOutgoing.data(), zmqReceiverMessageOutgoingString.c_str(), zmqReceiverMessageOutgoingString.size());

			std::cout << "Hub: Sending response to initiator ..." << std::endl;
			_zmqReceiverSocket.send(zmqReceiverMessageOutgoing);
			std::cout << "Hub: Response sent to initiator." << std::endl;
		}

		// Unbind the receiver
		_unbindReceiver();
		// Unbind the publisher
		_unbindPublisher();

		std::cout << "Hub: Hasta la vista." << std::endl;
	}
}
