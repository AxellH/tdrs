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
	HubChainClient::HubChainClient(int ctxn, _chainClientParams *params) : _zmqContext(ctxn), _zmqSubscriberSocket(_zmqContext, ZMQ_SUB), _zmqSenderSocket(_zmqContext, ZMQ_REQ) {
		_params = params;
		_runLoop = true;
	}

	/**
	 * @brief      Runs the chain client.
	 *
	 * @return     NULL
	 */
	void HubChainClient::run() {
		std::vector<_sharedMessageEntry> &sharedMessageVector = *_params->shmsgvec;

		std::cout << "Chain[" << _params->link << "]: Starting ..." << std::endl;
		zmq::context_t zmqContext(1);

		std::cout << "Chain[" << _params->link << "]: Connecting to receiver at " << _params->receiver << " ..." << std::endl;
		int _zmqSenderSocketLinger = 0;
		zmq::socket_t _zmqSenderSocket(zmqContext, ZMQ_REQ);
		_zmqSenderSocket.setsockopt(ZMQ_LINGER, &_zmqSenderSocketLinger, sizeof(_zmqSenderSocketLinger));
		_zmqSenderSocket.connect(_params->receiver);
		_params->senderSocket = &_zmqSenderSocket;

		std::cout << "Chain[" << _params->link << "]: Connected to receiver." << std::endl;

		int _zmqSubscriberSocketLinger = 0;
		std::cout << "Chain[" << _params->link << "]: Subscribing to link publisher at " << _params->link << " ..." << std::endl;
		zmq::socket_t _zmqSubscriberSocket(zmqContext, ZMQ_SUB);
		_zmqSubscriberSocket.setsockopt(ZMQ_LINGER, &_zmqSubscriberSocketLinger, sizeof(_zmqSubscriberSocketLinger));
		_zmqSubscriberSocket.setsockopt(ZMQ_IDENTITY, "hub", 3);
		_zmqSubscriberSocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
		_zmqSubscriberSocket.connect(_params->link);
		_params->subscriberSocket = &_zmqSubscriberSocket;
		std::cout << "Chain[" << _params->link << "]: Subscribed to link publisher." << std::endl;

		while(_params->run == true) {
			std::cout << "Chain[" << _params->link << "]: Loop started ..." << std::endl;
			zmq::message_t zmqSubscriberMessageIncoming;

			try {
				_zmqSubscriberSocket.recv(&zmqSubscriberMessageIncoming);
			} catch(...) {
				std::cout << "Chain[" << _params->link << "]: Message receiver failed. Looping." << std::endl;
				continue;
			}

			std::string zmqSubscriberMessageIncomingString(
				static_cast<const char*>(zmqSubscriberMessageIncoming.data()),
				zmqSubscriberMessageIncoming.size()
			);

			std::cout << "Chain[" << _params->link << "]: Received message: " << zmqSubscriberMessageIncomingString << std::endl;

			std::string hashedMessage = Hub::hashString(&zmqSubscriberMessageIncomingString);
			std::cout << "Chain[" << _params->link << "]: Hashed message: " << hashedMessage << std::endl;

			std::cout << "Chain[" << _params->link << "]: Checking hashed message in shared message vector ..." << std::endl;
			bool processMessage = true;
			pthread_mutex_lock(_params->shmsgvecmtx);
			if(std::find_if(
					sharedMessageVector.begin(),
					sharedMessageVector.end(),
					(boost::bind(&_sharedMessageEntry::hash, _1) == hashedMessage
						&& boost::bind(&_sharedMessageEntry::link, _1) == _params->link)
				) != sharedMessageVector.end()) {
				sharedMessageVector.erase(
					std::remove_if(
						sharedMessageVector.begin(),
						sharedMessageVector.end(),
						(boost::bind(&_sharedMessageEntry::hash, _1) == hashedMessage
							&& boost::bind(&_sharedMessageEntry::link, _1) == _params->link)
					), sharedMessageVector.end());
				processMessage = false;
			}
			pthread_mutex_unlock(_params->shmsgvecmtx);
			std::cout << "Chain[" << _params->link << "]: Checked hashed message in shared message vector." << std::endl;

			if(processMessage) {
				std::cout << "Chain[" << _params->link << "]: Forwarding message to receiver ..." << std::endl;
				try {
					_zmqSenderSocket.send(zmqSubscriberMessageIncoming);
				} catch(...) {
					std::cout << "Chain[" << _params->link << "]: Forwarding failed!" << std::endl;
					continue;
				}

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
					std::cout << "Chain[" << _params->link << "]: Forwarding successful." << std::endl;
				} else {
					std::cout << "Chain[" << _params->link << "]: Forwarding failed!" << std::endl;
				}
			} else {
				std::cout << "Chain[" << _params->link << "]: Not forwarding message to receiver as it was processed before." << std::endl;
			}
		}

		std::cout << std::endl << "Chain[" << _params->link << "]: Unsubscribing from link publisher at " << _params->link << " ..." << std::endl;
		_zmqSubscriberSocket.close();
		std::cout << std::endl << "Chain[" << _params->link << "]: Unsubscribed from link publisher." << std::endl;

		std::cout << std::endl << "Chain[" << _params->link << "]: Disconnecting from from receiver at " << _params->receiver << " ..." << std::endl;
		_zmqSenderSocket.close();
		std::cout << std::endl << "Chain[" << _params->link << "]: Disconnected from from receiver ..." << std::endl;

		std::cout << std::endl << "Chain[" << _params->link << "]: Goodbye!" << std::endl;
		std::cout.flush();
		delete _params;
	}
}
