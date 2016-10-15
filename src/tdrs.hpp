#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <zmq.hpp>
#include <boost/program_options.hpp>
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

			std::string _optionPublisherListen;
			std::string _optionReceiverListen;

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
