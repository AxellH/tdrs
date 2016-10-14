#include <iostream>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <zmq.hpp>

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
			zmq::context_t zmqContext;
			/**
			 * ZMQ Hub Socket.
			 */
			zmq::socket_t zmqHubSocket;
			/**
			 * ZMQ Receiver Socket.
			 */
			zmq::socket_t zmqReceiverSocket;
			/**
			 * The run-loop variable.
			 */
			bool runLoop;

			/**
			 * @brief      Binds the external publisher.
			 */
			void _bindExternalPublisher();
			/**
			 * @brief      Unbinds (closes) the external publisher.
			 */
			void _unbindExternalPublisher();
			/**
			 * @brief      Binds the external receiver.
			 */
			void _bindExternalReceiver();
			/**
			 * @brief      Unbinds (closes) the external receiver.
			 */
			void _unbindExternalReceiver();
		public:
			/**
			 * @brief      Constructs the object.
			 *
			 * @param[in]  ctxn  The number of context IO threads
			 */
			Hub(int ctxn);
			// ~Hub();

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
