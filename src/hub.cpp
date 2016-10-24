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
	 * @brief      The discovery service listener; static method instantiated as an own thread.
	 *
	 * @param      discoveryServiceParams  The discovery service parameters (struct)
	 *
	 * @return     NULL
	 */
	void *Hub::_discoveryServiceListener(void *discoveryServiceListenerParams) {
		_discoveryServiceListenerParams *params = static_cast<_discoveryServiceListenerParams*>(discoveryServiceListenerParams);
		tdrs::HubDiscoveryServiceListener hubDiscoveryServiceListener(params);

		hubDiscoveryServiceListener.run();
		return NULL;
	}

	/**
	 * @brief      The discovery service announcer; static method instantiated as an own thread.
	 *
	 * @param      discoveryServiceParams  The discovery service parameters (struct)
	 *
	 * @return     NULL
	 */
	void *Hub::_discoveryServiceAnnouncer(void *discoveryServiceAnnouncerParams) {
		_discoveryServiceAnnouncerParams *params = static_cast<_discoveryServiceAnnouncerParams*>(discoveryServiceAnnouncerParams);
		tdrs::HubDiscoveryServiceAnnouncer hubDiscoveryServiceAnnouncer(params);

		hubDiscoveryServiceAnnouncer.run();
		return NULL;
	}

	/**
	 * @brief      Method for running discovery service threads.
	 */
	void Hub::_runDisoveryServiceThreads() {
		std::cout << "Hub: Launching discovery service threads ..." << std::endl;

		std::cout << "Hub: Launching discovery listener thread ..." << std::endl;
		_discoveryServiceListenerThreadInstance.params = new _discoveryServiceListenerParams;
		_discoveryServiceListenerThreadInstance.params->receiver = _rewriteReceiver(&_optionReceiverListen);
		_discoveryServiceListenerThreadInstance.params->run = true;

		pthread_attr_init(&_discoveryServiceListenerThreadInstance.thattr);
		pthread_attr_setdetachstate(&_discoveryServiceListenerThreadInstance.thattr, PTHREAD_CREATE_DETACHED);
		pthread_create(&_discoveryServiceListenerThreadInstance.thread, &_discoveryServiceListenerThreadInstance.thattr, &Hub::_discoveryServiceListener, (void *)_discoveryServiceListenerThreadInstance.params);

		std::cout << "Hub: Launching discovery announcer thread ..." << std::endl;
		_discoveryServiceAnnouncerThreadInstance.params = new _discoveryServiceAnnouncerParams;
		_discoveryServiceAnnouncerThreadInstance.params->run = true;

		pthread_attr_init(&_discoveryServiceAnnouncerThreadInstance.thattr);
		pthread_attr_setdetachstate(&_discoveryServiceAnnouncerThreadInstance.thattr, PTHREAD_CREATE_DETACHED);
		pthread_create(&_discoveryServiceAnnouncerThreadInstance.thread, &_discoveryServiceAnnouncerThreadInstance.thattr, &Hub::_discoveryServiceAnnouncer, (void *)_discoveryServiceAnnouncerThreadInstance.params);

		std::cout << "Hub: Discovery service threads launched." << std::endl;
	}

	/**
	 * @brief      Method for shutting down all running discovery service threads.
	 */
	void Hub::_shutdownDisoveryServiceThreads() {
		std::cout << "Hub: Shutting down discovery announcer thread ..." << std::endl;
		_discoveryServiceAnnouncerThreadInstance.params->run = false;
		pthread_kill(_discoveryServiceAnnouncerThreadInstance.thread, SIGINT);

		std::cout << "Hub: Shutting down discovery listener thread ..." << std::endl;
		_discoveryServiceListenerThreadInstance.params->run = false;
		pthread_kill(_discoveryServiceListenerThreadInstance.thread, SIGINT);
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
		tdrs::HubChainClient hubChainClient(1, params);

		hubChainClient.run();
		return NULL;
	}

	/**
	 * @brief      Method for running one chain client thread.
	 *
	 * @param[in]  link  The link
	 */
	void Hub::_runChainClientThread(std::string link) {
		std::cout << "Hub: Launching chain client thread for link " << link << " ..." << std::endl;

		_chainClientThread client;
		client.params = new _chainClientParams;

		client.params->shmsgvecmtx = &_sharedMessageVectorMutex;
		client.params->shmsgvec = &_sharedMessageVector;
		client.params->link = link;

		client.params->receiver = _rewriteReceiver(&_optionReceiverListen);

		client.params->run = true;

		pthread_create(&client.thread, NULL, &Hub::_chainClient, (void *)client.params);
		_chainClientThreads.push_back(client);
	}

	/**
	 * @brief      Method for running all required chain client threads.
	 */
	void Hub::_runChainClientThreads() {
		std::string link;
		BOOST_FOREACH(link, _optionChainLinks) {
			_runChainClientThread(link);
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
	std::string Hub::hashString(std::string *source) {
		CryptoPP::SHA1 sha1;
		std::string hashed = "";

		CryptoPP::StringSource(*source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hashed))));

		return hashed;
	}

	std::string Hub::_rewriteReceiver(std::string *receiver) {
		std::regex receiverReplaceRegex("(\\*|0\\.0\\.0\\.0)");
		return std::regex_replace(_optionReceiverListen, receiverReplaceRegex, "127.0.0.1");
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

		// Run the discovery service threads
		_runDisoveryServiceThreads();

		// Run chain client threads
		_runChainClientThreads();

		std::cout << "Hub: Launching run-loop ..." << std::endl;
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

			std::string hashedMessage = Hub::hashString(&zmqReceiverMessageIncomingString);
			std::cout << "Hub: Hashed message: " << hashedMessage << std::endl;

			std::cout << "Hub: Adding hashed message to shared message vector ..." << std::endl;
			pthread_mutex_lock(&_sharedMessageVectorMutex);
			BOOST_FOREACH(_chainClientThread client, _chainClientThreads) {
				_sharedMessageEntry entry;
				entry.hash = hashedMessage;
				entry.link = client.params->link;

				_sharedMessageVector.push_back(entry);
				std::cout << "Hub: Hash for " << client.params->link << " added to shared message vector." << std::endl;
			}
			pthread_mutex_unlock(&_sharedMessageVectorMutex);
			std::cout << "Hub: Added hashed message to shared message vector." << std::endl;

			std::cout << "Hub: Forwarding message to Hub subscribers ..." << std::endl;
			zmq::message_t zmqIpcMessageOutgoing(zmqReceiverMessageIncoming.size());
			memcpy(zmqIpcMessageOutgoing.data(), zmqReceiverMessageIncoming.data(), zmqReceiverMessageIncoming.size());

			std::string zmqReceiverMessageOutgoingString;
			try {
				_zmqHubSocket.send(zmqIpcMessageOutgoing);
				zmqReceiverMessageOutgoingString = "OOK " + hashedMessage;
				std::cout << "Hub: Forwarding successful." << std::endl;
			} catch(...) {
				zmqReceiverMessageOutgoingString = "NOK " + hashedMessage;
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

		// Shutdown the discovery service threads
		_shutdownDisoveryServiceThreads();

		// Unbind the receiver
		_unbindReceiver();
		// Unbind the publisher
		_unbindPublisher();

		std::cout << "Hub: Hasta la vista." << std::endl;
	}
}
