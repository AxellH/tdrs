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
		_optionDiscovery = false;
		_optionDiscoveryPort = 5670;
		_optionDiscoveryInterval = 1000;
		_optionDiscoveryGroup = "TDRS";
		_optionDiscoveryKey = "TDRS";
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
	 * @brief      Method for running discovery service threads.
	 */
	void Hub::_runDisoveryServiceThreads() {
		std::cout << "Hub: Launching discovery service threads ..." << std::endl;

		std::cout << "Hub: Launching discovery listener thread ..." << std::endl;
		_discoveryServiceListenerThreadInstance.params = new _discoveryServiceListenerParams;
		_discoveryServiceListenerThreadInstance.params->receiver = _rewriteReceiver(&_optionReceiverListen);
		_discoveryServiceListenerThreadInstance.params->publisher = _optionPublisherListen;
		_discoveryServiceListenerThreadInstance.params->interface = _optionDiscoveryInterface;
		_discoveryServiceListenerThreadInstance.params->port = _optionDiscoveryPort;
		_discoveryServiceListenerThreadInstance.params->interval = _optionDiscoveryInterval;
		_discoveryServiceListenerThreadInstance.params->group = _optionDiscoveryGroup;
		_discoveryServiceListenerThreadInstance.params->key = _optionDiscoveryKey;
		_discoveryServiceListenerThreadInstance.params->run = true;

		pthread_attr_init(&_discoveryServiceListenerThreadInstance.thattr);
		pthread_attr_setdetachstate(&_discoveryServiceListenerThreadInstance.thattr, PTHREAD_CREATE_DETACHED);
		pthread_create(&_discoveryServiceListenerThreadInstance.thread, &_discoveryServiceListenerThreadInstance.thattr, &Hub::_discoveryServiceListener, (void *)_discoveryServiceListenerThreadInstance.params);

		std::cout << "Hub: Discovery service threads launched." << std::endl;
	}

	/**
	 * @brief      Method for shutting down all running discovery service threads.
	 */
	void Hub::_shutdownDisoveryServiceThreads() {
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

		pthread_cleanup_push(&Hub::_chainClientCleanup, chainClientParams);

		hubChainClient.run();

		pthread_cleanup_pop(0);
		return NULL;
	}

	/**
	 * @brief      The chain client cleanup; static method for cleaning up the thread.
	 *
	 * @param      chainClientParams  The chain client parameters (struct)
	 *
	 * @return     NULL
	 */
	void Hub::_chainClientCleanup(void *chainClientParams) {
		_chainClientParams *params = static_cast<_chainClientParams*>(chainClientParams);

		std::cout << "Chain[" << params->link << "]: Thread cancelled! Cleaning up ..." << std::endl;
		params->subscriberSocket->close();
		params->senderSocket->close();
		delete params;
		std::cout << "Chain[cleaned]: Thread cleaned up." << std::endl;
	}

	/**
	 * @brief      Method for running one chain client thread.
	 *
	 * @param[in]  id    The identifier
	 * @param[in]  link  The link
	 */
	void Hub::_runChainClientThread(std::string id, std::string link) {
		bool foundId = false;
		BOOST_FOREACH(_chainClientThread client, _chainClientThreads) {
			if(client.params->id == id) {
				foundId = true;
				break;
			}
		}

		if(foundId) {
			std::cout << "Hub: Not launching chain client thread for link " << link << " as was launched already." << std::endl;
			return;
		}

		std::cout << "Hub: Launching chain client thread for link " << link << " ..." << std::endl;

		_chainClientThread client;
		client.params = new _chainClientParams;

		client.params->shmsgvecmtx = &_sharedMessageVectorMutex;
		client.params->shmsgvec = &_sharedMessageVector;
		client.params->id = id;
		client.params->link = link;

		client.params->receiver = _rewriteReceiver(&_optionReceiverListen);

		client.params->run = true;

		pthread_attr_init(&client.thattr);
		pthread_attr_setdetachstate(&client.thattr, PTHREAD_CREATE_DETACHED);
		pthread_create(&client.thread, &client.thattr, &Hub::_chainClient, (void *)client.params);

		_chainClientThreads.push_back(client);

		std::cout << "Hub: Launched chain client thread for link " << link << "." << std::endl;
		return;
	}

	/**
	 * @brief      Method for running all required chain client threads.
	 */
	void Hub::_runChainClientThreads() {
		std::string link;
		std::string idPrefix = "manual-";
		int idNumber = 0;

		BOOST_FOREACH(link, _optionChainLinks) {
			idNumber++;

			std::string id = idPrefix + std::to_string(idNumber);
			_runChainClientThread(id, link);
		}
	}

	/**
	 * @brief      Method for shutting down one running chain client thread.
	 */
	bool Hub::_shutdownChainClientThread(std::string id) {
		bool wasShutDown = false;

		BOOST_FOREACH(_chainClientThread client, _chainClientThreads) {
			if(client.params->id == id) {
				std::cout << "Hub: Shutting down chain client thread for link " << client.params->link << " ..." << std::endl;

				client.params->run = false;

				pthread_cancel(client.thread);
				wasShutDown = true;
				break;
			}
		}

		return wasShutDown;
	}

	/**
	 * @brief      Method for shutting down all running chain client threads.
	 */
	void Hub::_shutdownChainClientThreads() {
		BOOST_FOREACH(_chainClientThread client, _chainClientThreads) {
			std::cout << "Hub: Shutting down chain client thread for link " << client.params->link << " ..." << std::endl;

			client.params->run = false;

			pthread_cancel(client.thread);
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

	/**
	 * @brief      Method for rewriting a receiver address if necessarry.
	 *
	 * @param      receiver  The receiver address
	 *
	 * @return     The rewritten address
	 */
	std::string Hub::_rewriteReceiver(std::string *receiver) {
		std::regex receiverReplaceRegex("(\\*|0\\.0\\.0\\.0)");
		return std::regex_replace(_optionReceiverListen, receiverReplaceRegex, "127.0.0.1");
	}

	/**
	 * @brief      Static method for parsing a peer message into
	 * peerMessage type.
	 *
	 * @param[in]  message  The message
	 *
	 * @return     The peerMessage
	 */
	peerMessage *Hub::_parsePeerMessage(const std::string &message) {
		// PEER:<event>:<id>:<pub proto>:<pub addr>:<pub port>:<sub proto>:<sub addr>:<sub port>
		std::regex messageSearchRegex("PEER:([a-zA-Z]+):([a-zA-Z0-9]+):([a-zA-Z\\*]+):([0-9\\.\\*]+):([0-9\\*]+):([a-zA-Z\\*]+):([0-9\\.\\*]+):([0-9\\*]+)");
		std::smatch match;

		if(std::regex_search(message.begin(), message.end(), match, messageSearchRegex)) {
			peerMessage *pm = new peerMessage;

			pm->event = match[1];
			pm->id = match[2];
			pm->publisher = match[3].str() + "://" + match[4].str() + (match[5].str() != "" ? (":" + match[5].str()) : "");
			pm->receiver = match[6].str() + "://" + match[7].str() + (match[8].str() != "" ? (":" + match[8].str()) : "");

			return pm;
		}

		return NULL;
	}

	/**
	 * @brief      Static method for parsing a ZeroMQ address string into
	 * zeroAddress type.
	 *
	 * @param[in]  address  The address
	 *
	 * @return     The zeroAddress
	 */
	zeroAddress *Hub::parseZeroAddress(const std::string &address) {
		std::regex addressSearchRegex("(.+):\\/\\/([0-9\\.\\*]+):?([0-9]*)");
		std::smatch match;

		if(std::regex_search(address.begin(), address.end(), match, addressSearchRegex)) {
			zeroAddress *za = new zeroAddress;

			za->protocol = match[1];
			za->address = match[2];
			za->port = match[3];

			return za;
		}

		return NULL;
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
				("discovery", "enable auto discovery of chain links")
				("discovery-interval", bpo::value<size_t>(), "set the auto discovery interval (ms), default 1000")
				("discovery-interface", bpo::value<std::string>(), "set the network interface to be used for auto discovery, e.g. eth0")
				("discovery-port", bpo::value<int>(), "set the UDP port to be used for auto discovery, default 5670")
				// ("discovery-group", bpo::value<std::string>(), "set the auto discovery group name, default 'TDRS'")
				("discovery-key", bpo::value<std::string>(), "set the auto discovery key, default 'TDRS'")
			;

			bpo::variables_map variablesMap;
			bpo::store(bpo::parse_command_line(argc, argv, optionsDescription), variablesMap);
			bpo::notify(variablesMap);

			if(variablesMap.count("help")) {
				std::cout << optionsDescription << std::endl;
				return false;
			}

			if(variablesMap.count("receiver-listen")) {
				_optionReceiverListen = variablesMap["receiver-listen"].as<std::string>();
				std::cout << "Hub: Listener for receiver was set to " << _optionReceiverListen << std::endl;
			} else {
				std::cout << "Hub: Listener for receiver (--receiver-listen) was not set!" << std::endl << std::endl;
				std::cout << optionsDescription << std::endl;
				return false;
			}

			if(variablesMap.count("publisher-listen")) {
				_optionPublisherListen = variablesMap["publisher-listen"].as<std::string>();
				std::cout << "Hub: Listener for publisher was set to " << _optionPublisherListen << std::endl;
			} else {
				std::cout << "Hub: Listener for publisher (--publisher-listen) was not set!" << std::endl << std::endl;
				std::cout << optionsDescription << std::endl;
				return false;
			}

			if(variablesMap.count("discovery")) {
				if(variablesMap.count("chain-link")) {
					std::cout << "Hub: Error, cannot manually add chain links while --discovery is enabled. Use either --discovery or --chain-link." << std::endl;
					return false;
				}

				_optionDiscovery = true;
				std::cout << "Hub: Auto discovery was enabled." << std::endl;
			}

			if(variablesMap.count("discovery-interval")) {
				_optionDiscoveryInterval = variablesMap["discovery-interval"].as<size_t>();
				std::cout << "Hub: Auto discovery interval was set to " << _optionDiscoveryInterval << std::endl;
			}

			if(variablesMap.count("discovery-interface")) {
				_optionDiscoveryInterface = variablesMap["discovery-interface"].as<std::string>();
				std::cout << "Hub: Auto discovery interface was set to " << _optionDiscoveryInterface << std::endl;
			}

			if(variablesMap.count("discovery-port")) {
				_optionDiscoveryPort = variablesMap["discovery-port"].as<int>();
				std::cout << "Hub: Auto discovery port was set to " << _optionDiscoveryPort << std::endl;
			}

			// if(variablesMap.count("discovery-group")) {
			// 	_optionDiscoveryGroup = variablesMap["discovery-group"].as<std::string>();
			// 	std::cout << "Hub: Auto discovery group was set to " << _optionDiscoveryGroup << std::endl;
			// }

			if(variablesMap.count("discovery-key")) {
				_optionDiscoveryKey = variablesMap["discovery-key"].as<std::string>();
				std::cout << "Hub: Auto discovery key was set to " << _optionDiscoveryKey << std::endl;
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

		if(_optionDiscovery == true) {
			// Run the discovery service threads
			_runDisoveryServiceThreads();
		} else {
			// Run chain client threads
			_runChainClientThreads();
		}

		std::cout << "Hub: Launching run-loop ..." << std::endl;

		// Run loop
		bool propagateMessage = true;
		while(_runLoop == true) {
			propagateMessage = true;
			zmq::message_t zmqReceiverMessageIncoming;
			std::string zmqReceiverMessageOutgoingString;

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

			if(zmqReceiverMessageIncomingString.substr(0, 5) == "PEER:") {
				std::cout << "Hub: Message is peer announcement. Processing ..." << std::endl;

				peerMessage *discoveredPeer = Hub::_parsePeerMessage(zmqReceiverMessageIncomingString);

				if(discoveredPeer != NULL) {
					if(discoveredPeer->event == "ENTER") {
						std::cout << "Hub: Running new chain client thread for announced peer ..." << std::endl;
						_runChainClientThread(discoveredPeer->id, discoveredPeer->publisher);
					} else if(discoveredPeer->event == "EXIT") {
						std::cout << "Hub: Exiting chain client thread for peer ..." << std::endl;
						if(!_shutdownChainClientThread(discoveredPeer->id)) {
							std::cout << "Hub: Chain client thread was not available. Not propagating peer announcement!" << std::endl;
							propagateMessage = false;
							zmqReceiverMessageOutgoingString = "NOK NOT AVAILABLE";
						}
					}
					delete discoveredPeer;
				}
			}

			if(propagateMessage) {
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

				try {
					_zmqHubSocket.send(zmqIpcMessageOutgoing);
					zmqReceiverMessageOutgoingString = "OOK " + hashedMessage;
					std::cout << "Hub: Forwarding successful." << std::endl;
				} catch(...) {
					zmqReceiverMessageOutgoingString = "NOK " + hashedMessage;
					std::cout << "Hub: Forwarding failed!" << std::endl;
				}
			}

			std::cout << "Hub: Preparing response to initiator ..." << std::endl;
			zmq::message_t zmqReceiverMessageOutgoing(zmqReceiverMessageOutgoingString.size());
			memcpy(zmqReceiverMessageOutgoing.data(), zmqReceiverMessageOutgoingString.c_str(), zmqReceiverMessageOutgoingString.size());

			std::cout << "Hub: Sending response to initiator ..." << std::endl;
			_zmqReceiverSocket.send(zmqReceiverMessageOutgoing);
			std::cout << "Hub: Response sent to initiator." << std::endl;
		}

		std::cout << std::endl;

		if(_optionDiscovery == true) {
			// Shutdown the discovery service threads
			_shutdownDisoveryServiceThreads();
		}

		// Shutdown chain client threads, from auto discovery or manual setup
		_shutdownChainClientThreads();

		// Unbind the receiver
		_unbindReceiver();
		// Unbind the publisher
		_unbindPublisher();

		std::cout << "Hub: Hasta la vista." << std::endl;
	}
}
