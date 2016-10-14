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
	Hub::Hub(int ctxn) : zmqContext(ctxn), zmqHubSocket(zmqContext, ZMQ_PUB), zmqReceiverSocket(zmqContext, ZMQ_REP) {
		runLoop = true;
	}

	/**
	 * @brief      Binds the external publisher.
	 */
	void Hub::_bindExternalPublisher() {
		std::cout << "Hub: Binding external publisher ..." << std::endl;
		int zmqHubSocketLinger = 0;
		zmqHubSocket.setsockopt(ZMQ_LINGER, &zmqHubSocketLinger, sizeof(zmqHubSocketLinger));
		zmqHubSocket.bind("tcp://*:19790");
		std::cout << "Hub: Bound external publisher." << std::endl;
	}

	/**
	 * @brief      Unbinds (closes) the external publisher.
	 */
	void Hub::_unbindExternalPublisher() {
		std::cout << "Hub: Sending termination to external subscribers ..." << std::endl;
		zmqHubSocket.send("TERMINATE", 9, 0);
		std::cout << "Hub: Sent termination to external subscribers." << std::endl;
		std::cout << "Hub: Unbinding external publisher ..." << std::endl;
		zmqHubSocket.close();
		std::cout << "Hub: Unbound external publisher." << std::endl;
	}

	/**
	 * @brief      Binds the external receiver.
	 */
	void Hub::_bindExternalReceiver() {
		std::cout << "Hub: Binding external receiver ..." << std::endl;
		int zmqReceiverSocketLinger = 0;
		zmqReceiverSocket.setsockopt(ZMQ_LINGER, &zmqReceiverSocketLinger, sizeof(zmqReceiverSocketLinger));
		zmqReceiverSocket.bind("tcp://*:19780");
		std::cout << "Hub: Bound external receiver." << std::endl;
	}

	/**
	 * @brief      Unbinds (closes) the external receiver.
	 */
	void Hub::_unbindExternalReceiver() {
		std::cout << "Hub: Unbinding external receiver ..." << std::endl;
		zmqReceiverSocket.close();
		std::cout << "Hub: Unbound external receiver." << std::endl;
	}

	/**
	 * @brief      Requests an exit of the run-loop on its next iteration.
	 */
	void Hub::shutdown() {
		runLoop = false;
	}

	/**
	 * @brief      Runs the Hub.
	 */
	void Hub::run() {
		// Bind the external publisher
		_bindExternalPublisher();
		// Bind the external receiver
		_bindExternalReceiver();

		// Run loop
		while(runLoop == true) {
			zmq::message_t zmqReceiverMessageIncoming;

			try {
				zmqReceiverSocket.recv(&zmqReceiverMessageIncoming);
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
				zmqHubSocket.send(zmqIpcMessageOutgoing);
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
			zmqReceiverSocket.send(zmqReceiverMessageOutgoing);
			std::cout << "Hub: Response sent to initiator." << std::endl;
		}

		// Unbind the external receiver
		_unbindExternalReceiver();
		// Unbind the external publisher
		_unbindExternalPublisher();

		std::cout << "Hub: Hasta la vista." << std::endl;
	}
}
