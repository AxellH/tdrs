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
	 * @brief      The chain client; static method instantiated as an own thread.
	 *
	 * @param      chainClientParams  The chain client parameters (struct)
	 *
	 * @return     NULL
	 */
	void *Hub::_chainClient(void *chainClientParams) {
		_chainClientParams *params = static_cast<_chainClientParams*>(chainClientParams);
		std::vector<std::string> &sharedMessageVector = *params->shmsgvec;

		std::cout << "Chain[" << params->link << "]: Starting ..." << std::endl;
		zmq::context_t zmqContext(1);

		int zmqSenderSocketLinger = 0;
		std::cout << "Chain[" << params->link << "]: Connecting to receiver at " << params->receiver << " ..." << std::endl;
		zmq::socket_t zmqSenderSocket(zmqContext, ZMQ_REQ);
		zmqSenderSocket.setsockopt(ZMQ_LINGER, &zmqSenderSocketLinger, sizeof(zmqSenderSocketLinger));
		zmqSenderSocket.connect(params->receiver);
		std::cout << "Chain[" << params->link << "]: Connected to receiver." << std::endl;

		int zmqSubscriberSocketLinger = 0;
		std::cout << "Chain[" << params->link << "]: Subscribing to link publisher at " << params->link << " ..." << std::endl;
		zmq::socket_t zmqSubscriberSocket(zmqContext, ZMQ_SUB);
		zmqSubscriberSocket.setsockopt(ZMQ_LINGER, &zmqSubscriberSocketLinger, sizeof(zmqSubscriberSocketLinger));
		zmqSubscriberSocket.setsockopt(ZMQ_IDENTITY, "hub", 3);
		zmqSubscriberSocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
		zmqSubscriberSocket.connect(params->link);
		std::cout << "Chain[" << params->link << "]: Subscribed to link publisher." << std::endl;

		while(params->run == true) {
			std::cout << "Chain[" << params->link << "]: Loop started ..." << std::endl;
			zmq::message_t zmqSubscriberMessageIncoming;

			try {
				zmqSubscriberSocket.recv(&zmqSubscriberMessageIncoming);
			} catch(...) {
				std::cout << "Chain[" << params->link << "]: Message receiver failed. Looping." << std::endl;
				continue;
			}

			std::string zmqSubscriberMessageIncomingString(
				static_cast<const char*>(zmqSubscriberMessageIncoming.data()),
				zmqSubscriberMessageIncoming.size()
			);

			std::cout << "Chain[" << params->link << "]: Received message: " << zmqSubscriberMessageIncomingString << std::endl;

			std::string hashedMessage = Hub::_hashString(&zmqSubscriberMessageIncomingString);
			std::cout << "Chain[" << params->link << "]: Hashed message: " << hashedMessage << std::endl;

			std::cout << "Chain[" << params->link << "]: Checking hashed message in shared message vector ..." << std::endl;
			bool processMessage = true;
			pthread_mutex_lock(params->shmsgvecmtx);
			if(std::find(sharedMessageVector.begin(), sharedMessageVector.end(), hashedMessage) != sharedMessageVector.end()) {
				sharedMessageVector.erase(std::remove(sharedMessageVector.begin(), sharedMessageVector.end(), hashedMessage), sharedMessageVector.end());
				processMessage = false;
			}
			pthread_mutex_unlock(params->shmsgvecmtx);
			std::cout << "Chain[" << params->link << "]: Checked hashed message in shared message vector." << std::endl;

			if(processMessage) {
				std::cout << "Chain[" << params->link << "]: Forwarding message to receiver ..." << std::endl;
				try {
					zmqSenderSocket.send(zmqSubscriberMessageIncoming);
				} catch(...) {
					std::cout << "Chain[" << params->link << "]: Forwarding failed!" << std::endl;
					continue;
				}

				zmq::message_t zmqSenderMessageIncoming;
				try {
					zmqSenderSocket.recv(&zmqSenderMessageIncoming);
				} catch(...) {
					continue;
				}

				std::string zmqSenderMessageIncomingString(
					static_cast<const char*>(zmqSenderMessageIncoming.data()),
					zmqSenderMessageIncoming.size()
				);

				if(zmqSenderMessageIncomingString == "OK") {
					std::cout << "Chain[" << params->link << "]: Forwarding successful." << std::endl;
				} else {
					std::cout << "Chain[" << params->link << "]: Forwarding failed!" << std::endl;
				}
			} else {
				std::cout << "Chain[" << params->link << "]: Not forwarding message to receiver as it was processed before." << std::endl;
			}
		}

		std::cout << std::endl << "Chain[" << params->link << "]: Unsubscribing from link publisher at " << params->link << " ..." << std::endl;
		zmqSubscriberSocket.close();
		std::cout << std::endl << "Chain[" << params->link << "]: Unsubscribed from link publisher." << std::endl;

		std::cout << std::endl << "Chain[" << params->link << "]: Disconnecting from from receiver at " << params->receiver << " ..." << std::endl;
		zmqSenderSocket.close();
		std::cout << std::endl << "Chain[" << params->link << "]: Disconnected from from receiver ..." << std::endl;

		std::cout << std::endl << "Chain[" << params->link << "]: Goodbye!" << std::endl;
		std::cout.flush();
		delete params;
		return NULL;
	}

	/**
	 * @brief      Method for running all required chain client threads.
	 */
	void Hub::_runChainClientThreads() {
		std::string link;
		BOOST_FOREACH(link, _optionChainLinks) {
			std::cout << "Hub: Launching chain client thread for link " << link << " ..." << std::endl;

			_chainClientThread client;
			client.params = new _chainClientParams;

			client.params->shmsgvecmtx = &_sharedMessageVectorMutex;
			client.params->shmsgvec = &_sharedMessageVector;
			client.params->link = link;

			std::regex receiverReplaceRegex("(\\*|0\\.0\\.0\\.0)");
			client.params->receiver = std::regex_replace(_optionReceiverListen, receiverReplaceRegex, "127.0.0.1");

			client.params->run = true;

			pthread_create(&client.thread, NULL, &Hub::_chainClient, (void *)client.params);
			_chainClientThreads.push_back(client);
		}
	}

	/**
	 * @brief      Method for shutting down all running chain client threads.
	 */
	void Hub::_shutdownChainClientThreads() {
		BOOST_FOREACH(_chainClientThread client, _chainClientThreads) {
			std::cout << "Hub: Shutting down chain client thread for link " << client.params->link << " ..." << std::endl;

			client.params->run = false;

			pthread_kill(client.thread, SIGINT);
		}
	}

	/**
	 * @brief      Static method for hashing a string using SHA1.
	 *
	 * @param      source  The source string
	 *
	 * @return     The hash.
	 */
	std::string Hub::_hashString(std::string *source) {
		CryptoPP::SHA1 sha1;
		std::string hashed = "";

		CryptoPP::StringSource(*source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hashed))));

		return hashed;
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
				("chain-link", bpo::value<std::vector<std::string> >(&_optionChainLinks)->multitoken(), "add a chain link, specify one per link")
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
				std::cout << "Hub: Listener for receiver (--receiver-listen) was not set!" << std::endl << std::endl;
				std::cout << optionsDescription << std::endl;
				return false;
			}

			if (variablesMap.count("publisher-listen")) {
				_optionPublisherListen = variablesMap["publisher-listen"].as<std::string>();
				std::cout << "Hub: Listener for publisher was set to " << _optionPublisherListen << std::endl;
			} else {
				std::cout << "Hub: Listener for publisher (--publisher-listen) was not set!" << std::endl << std::endl;
				std::cout << optionsDescription << std::endl;
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

		// Run chain client threads
		_runChainClientThreads();

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

			std::string hashedMessage = Hub::_hashString(&zmqReceiverMessageIncomingString);
			std::cout << "Hub: Hashed message: " << hashedMessage << std::endl;

			std::cout << "Hub: Adding hashed message to shared message vector ..." << std::endl;
			pthread_mutex_lock(&_sharedMessageVectorMutex);
			_sharedMessageVector.push_back(hashedMessage);
			pthread_mutex_unlock(&_sharedMessageVectorMutex);
			std::cout << "Hub: Added hashed message to shared message vector." << std::endl;

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

		std::cout << std::endl;

		// Shutdown chain client threads
		_shutdownChainClientThreads();

		// Unbind the receiver
		_unbindReceiver();
		// Unbind the publisher
		_unbindPublisher();

		std::cout << "Hub: Hasta la vista." << std::endl;
	}
}
