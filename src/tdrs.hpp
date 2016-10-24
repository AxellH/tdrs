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
		std::string link;
		std::string receiver;
		pthread_mutex_t *shmsgvecmtx;
		std::vector<_sharedMessageEntry> *shmsgvec;
		bool run;
	};

	/**
	 * @brief      Chain client thread struct, containing the thread itself and the parameters.
	 */
	struct _chainClientThread {
		pthread_t thread;
		_chainClientParams *params;
	};

	/**
	 * @brief      Parameters struct for service discovery listener thread.
	 */
	struct _discoveryServiceListenerParams {
		std::string receiver;
		bool run;
	};

	/**
	 * @brief      Discovery service listener thread struct, containing the thread itself and the parameters.
	 */
	struct _discoveryServiceListenerThread {
		pthread_t thread;
		_discoveryServiceListenerParams *params;
	};

	/**
	 * @brief      Parameters struct for service discovery announcer thread.
	 */
	struct _discoveryServiceAnnouncerParams {
		bool run;
	};

	/**
	 * @brief      Discovery service announcer thread struct, containing the thread itself and the parameters.
	 */
	struct _discoveryServiceAnnouncerThread {
		pthread_t thread;
		_discoveryServiceAnnouncerParams *params;
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
			 * Instance storing discovery service announcer thread struct.
			 */
			_discoveryServiceAnnouncerThread _discoveryServiceAnnouncerThreadInstance;
			/**
			 * @brief      The discovery service announcer; static method instantiated as an own thread.
			 *
			 * @param      discoveryServiceParamsListener  The discovery service listener parameters (struct)
			 *
			 * @return     NULL
			 */
			static void *_discoveryServiceAnnouncer(void *discoveryServiceAnnouncerParams);

			/**
			 * @brief      Method for running discovery service thread.
			 */
			void _runDisoveryServiceThreads();

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
			 * @brief      Method for running one chain client thread.
			 *
			 * @param[in]  link  The link
			 */
			void _runChainClientThread(std::string link);
			/**
			 * @brief      Method for running all required chain client threads.
			 */
			void _runChainClientThreads();
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
	 * @brief      Class for HubDiscoveryServiceAnnouncer.
	 */
	class HubDiscoveryServiceAnnouncer {
		private:
			_discoveryServiceAnnouncerParams *_params;
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
			HubDiscoveryServiceAnnouncer(_discoveryServiceAnnouncerParams *params);
			// ~HubDiscoveryServiceAnnouncer();

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
			 * @brief      Requests an exit of the run-loop on its next iteration.
			 */
			void shutdown();
			/**
			 * @brief      Runs the chain client.
			 */
			void run();
	};
}
