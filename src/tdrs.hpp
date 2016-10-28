#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <regex>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <zmq.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <zyrecpp.hpp>

namespace bpo = boost::program_options;

/**
 * tdrs namespace
 */
namespace tdrs {
	struct zeroAddress {
		std::string protocol;
		std::string address;
		std::string port;
	};

	struct peerMessage {
		std::string event;
		std::string id;
		std::string publisher;
		std::string receiver;
	};

	/**
	 * @brief      Shared message entry.
	 */
	struct _sharedMessageEntry {
		std::string hash;
		std::string link;
	};

	/**
	 * @brief      Parameters struct for chain client thread.
	 */
	struct _chainClientParams {
		std::string id;
		std::string link;
		std::string receiver;
		pthread_mutex_t *shmsgvecmtx;
		std::vector<_sharedMessageEntry> *shmsgvec;
		bool run;
		zmq::socket_t *subscriberSocket;
		zmq::socket_t *senderSocket;
	};

	/**
	 * @brief      Chain client thread struct, containing the thread itself and the parameters.
	 */
	struct _chainClientThread {
		pthread_t thread;
		pthread_attr_t thattr;
		_chainClientParams *params;
	};

	/**
	 * @brief      Parameters struct for service discovery listener thread.
	 */
	struct _discoveryServiceListenerParams {
		std::string publisher;
		std::string receiver;
		std::string interface;
		int port;
		size_t interval;
		std::string group;
		std::string key;
		bool run;
	};

	/**
	 * @brief      Discovery service listener thread struct, containing the thread itself and the parameters.
	 */
	struct _discoveryServiceListenerThread {
		pthread_t thread;
		pthread_attr_t thattr;
		_discoveryServiceListenerParams *params;
	};

	/**
	 * @brief      Class for Hub.
	 */
	class Hub {
		private:
			/**
			 * ZMQ Context.
			 */
			zmq::context_t _zmqContext;
			/**
			 * ZMQ Hub Socket.
			 */
			zmq::socket_t _zmqHubSocket;
			/**
			 * ZMQ Receiver Socket.
			 */
			zmq::socket_t _zmqReceiverSocket;
			/**
			 * The run-loop variable.
			 */
			bool _runLoop;

			/**
			 * Shared message vector mutex, for locking shared message vector.
			 */
			pthread_mutex_t _sharedMessageVectorMutex;
			/**
			 * Shared message vector between main process and chain client threads.
			 */
			std::vector<_sharedMessageEntry> _sharedMessageVector;

			/**
			 * Option: --publisher-listen
			 */
			std::string _optionPublisherListen;
			/**
			 * Option: --receiver-listen
			 */
			std::string _optionReceiverListen;
			/**
			 * Option: --chain-link
			 */
			std::vector<std::string> _optionChainLinks;
			/**
			 * Option: --discovery
			 */
			bool _optionDiscovery;
			/**
			 * Option: --discovery-port
			 */
			int _optionDiscoveryPort;
			/**
			 * Option: --discovery-interface
			 */
			std::string _optionDiscoveryInterface;
			/**
			 * Option: --discovery-interval
			 */
			size_t _optionDiscoveryInterval;
			/**
			 * Option: --discovery-group
			 */
			std::string _optionDiscoveryGroup;
			/**
			 * Option: --discovery-key
			 */
			std::string _optionDiscoveryKey;

			/**
			 * @brief      Binds the publisher.
			 */
			void _bindPublisher();
			/**
			 * @brief      Unbinds (closes) the publisher.
			 */
			void _unbindPublisher();
			/**
			 * @brief      Binds the receiver.
			 */
			void _bindReceiver();
			/**
			 * @brief      Unbinds (closes) the receiver.
			 */
			void _unbindReceiver();

			/**
			 * Instance storing discovery service listener thread struct.
			 */
			_discoveryServiceListenerThread _discoveryServiceListenerThreadInstance;
			/**
			 * @brief      The discovery service listener; static method instantiated as an own thread.
			 *
			 * @param      discoveryServiceParamsListener  The discovery service listener parameters (struct)
			 *
			 * @return     NULL
			 */
			static void *_discoveryServiceListener(void *discoveryServiceListenerParams);

			/**
			 * @brief      Method for running discovery service thread.
			 */
			void _runDisoveryServiceThreads();
			/**
			 * @brief      Method for shutting down all running discovery service threads.
			 */
			void _shutdownDisoveryServiceThreads();

			/**
			 * Vector storing chain client thread structs.
			 */
			std::vector<_chainClientThread> _chainClientThreads;

			/**
			 * @brief      The chain client; static method instantiated as an own thread.
			 *
			 * @param      chainClientParams  The chain client parameters (struct)
			 *
			 * @return     NULL
			 */
			static void *_chainClient(void *chainClientParams);
			/**
			 * @brief      The chain client cleanup; static method for cleaning up the thread.
			 *
			 * @param      chainClientParams  The chain client parameters (struct)
			 *
			 * @return     NULL
			 */
			static void _chainClientCleanup(void *chainClientParams);

			/**
			 * @brief      Method for running one chain client thread.
			 *
			 * @param[in]  id    The identifier
			 * @param[in]  link  The link
			 */
			void _runChainClientThread(std::string id, std::string link);
			/**
			 * @brief      Method for running all required chain client threads.
			 */
			void _runChainClientThreads();
			/**
			 * @brief      Method for shutting down one running chain client thread.
			 */
			bool _shutdownChainClientThread(std::string id);
			/**
			 * @brief      Method for shutting down all running chain client threads.
			 */
			void _shutdownChainClientThreads();

			/**
			 * @brief      Method for rewriting a receiver address if necessarry.
			 *
			 * @param      receiver  The receiver address
			 *
			 * @return     The rewritten address
			 */
			std::string _rewriteReceiver(std::string *receiver);

			/**
			 * @brief      Static method for parsing a peer message into
			 * peerMessage type.
			 *
			 * @param[in]  message  The message
			 *
			 * @return     The peerMessage
			 */
			static peerMessage *_parsePeerMessage(const std::string &message);
		public:
			/**
			 * @brief      Constructs the object.
			 *
			 * @param[in]  ctxn  The number of context IO threads
			 */
			Hub(int ctxn);
			// ~Hub();

			/**
			 * @brief      Static method for hashing a string using SHA1.
			 *
			 * @param      source  The source string
			 *
			 * @return     The hash.
			 */
			static std::string hashString(std::string *source);

			/**
			 * @brief      Static method for parsing a ZeroMQ address string into
			 * zeroAddress type.
			 *
			 * @param[in]  address  The address
			 *
			 * @return     The zeroAddress
			 */
			static zeroAddress *parseZeroAddress(const std::string &address);

			/**
			 * @brief      Sets the Hub options.
			 *
			 * @param[in]  argc  The main argc
			 * @param      argv  The main argv
			 *
			 * @return     True on success, false on failure.
			 */
			bool options(int argc, char *argv[]);
			/**
			 * @brief      Requests an exit of the run-loop on its next iteration.
			 */
			void shutdown();
			/**
			 * @brief      Runs the Hub.
			 */
			void run();
	};

	/**
	 * @brief      Class for HubDiscoveryServiceListener.
	 */
	class HubDiscoveryServiceListener {
		private:
			_discoveryServiceListenerParams *_params;
			/**
			 * The run-loop variable.
			 */
			bool _runLoop;
		public:
			/**
			 * @brief      Constructs the object.
			 *
			 * @param[in]  ctxn  The number of context IO threads
			 */
			HubDiscoveryServiceListener(_discoveryServiceListenerParams *params);
			// ~HubDiscoveryServiceListener();

			/**
			 * @brief      Requests an exit of the run-loop on its next iteration.
			 */
			void shutdown();
			/**
			 * @brief      Runs the discovery service.
			 */
			void run();
	};

	/**
	 * @brief      Class for HubChainClient.
	 */
	class HubChainClient {
		private:
			_chainClientParams *_params;
			/**
			 * ZMQ Context.
			 */
			zmq::context_t _zmqContext;
			/**
			 * ZMQ Subscriber Socket.
			 */
			zmq::socket_t _zmqSubscriberSocket;
			/**
			 * ZMQ Sender Socket.
			 */
			zmq::socket_t _zmqSenderSocket;
			/**
			 * The run-loop variable.
			 */
			bool _runLoop;
		public:
			/**
			 * @brief      Constructs the object.
			 *
			 * @param[in]  ctxn  The number of context IO threads
			 */
			HubChainClient(int ctxn, _chainClientParams *params);
			// ~HubChainClient();

			/**
			 * @brief      Runs the chain client.
			 */
			void run();
	};
}
