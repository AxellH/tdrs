#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <regex>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <zmq.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

namespace bpo = boost::program_options;

/**
 * tdrs namespace
 */
namespace tdrs {
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
			std::vector<std::string> _sharedMessageVector;

			/**
			 * @brief      Parameters struct for chain client thread.
			 */
			struct _chainClientParams {
				std::string link;
				std::string receiver;
				pthread_mutex_t *shmsgvecmtx;
				std::vector<std::string> *shmsgvec;
				bool run;
			};

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
			 * @brief      Chain client thread struct, containing the thread itself and the parameters.
			 */
			struct _chainClientThread {
				pthread_t thread;
				_chainClientParams *params;
			};
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
			 * @brief      Method for running all required chain client threads.
			 */
			void _runChainClientThreads();
			/**
			 * @brief      Method for shutting down all running chain client threads.
			 */
			void _shutdownChainClientThreads();

			/**
			 * @brief      Static method for hashing a string using SHA1.
			 *
			 * @param      source  The source string
			 *
			 * @return     The hash.
			 */
			static std::string _hashString(std::string *source);
		public:
			/**
			 * @brief      Constructs the object.
			 *
			 * @param[in]  ctxn  The number of context IO threads
			 */
			Hub(int ctxn);
			// ~Hub();

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
}
