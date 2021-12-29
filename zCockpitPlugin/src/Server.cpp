#define _WINSOCK_DEPRECATED_NO_WARNINGS




#include <WinSock2.h>
#include <WS2tcpip.h>
#include <charconv>
#include <map>

#pragma comment(lib, "Ws2_32.lib")


#include "Server.h"

#include <ranges>

#include "../../../shared_src/logger.h"
#include "../../../shared_src/Util.h"


// random number generator
std::random_device rd;  //Will be used to obtain a seed for the random number engine
std::seed_seq seed{ rd(), rd(), rd(), rd() };
std::mt19937 Server::gen(seed); //Standard mersenne_twister_engine seeded with rd()
std::uniform_int_distribution<uint32_t> make_uid(1, UINT32_MAX);



Server::Server()
{
	// Must be called first to initialize Winsock
	udp = std::make_unique<Udp>();
	network = std::make_unique<Network>();

	LOG() << "Starting Server";
}

Server::~Server()
{
	if (network != nullptr) {
		if (broadcaster.valid()) {
			Udp::close_endpoint(&broadcaster);
		}
		if (broadcast_receiver.valid()) {
			Udp::close_endpoint(&broadcast_receiver);
		}
	}
	udp = nullptr;
	network = nullptr;
}



void Server::update()
{
	// 
	// To Established Communication with a Client
	// 1. Listen for Broadcast from the Client
	// 2. We can open our RX because we know the Client IP
	// 3. Respond by Broadcasting our RX port
	// 4. Client then open its transmitter and Receiver
	//
	// 5. Keep listening on Client's Broadcast for its RX port change 
	// 6. Open transmitter with RX port
	// 7. Connection Complete
	//

	// *****************
	// BROADCAST
	// *****************

	// listen for Client on Broadcast port
	if (!broadcast_receiver.valid()) {
		// we need to init the socket for receiving the broadcast message from_addr the Server
		broadcast_receiver = Udp::create_receiver("broadcast", SERVER_BROADCAST_RX_PORT_ADDRESS);
	}
	// Used to initially respond to Client's broadcast
	if (!broadcaster.valid()) {
		broadcaster = Udp::create_transmitter("send broadcast", CLIENT_BROADCAST_RX_PORT_ADDRESS, Udp::get_subnet_ip(), true);
	}

	// Always look for new broadcast
	if (broadcast_receiver.valid()) {
		// use recveive_from so we can get its IP address
		const auto num_bytes = Udp::receive_from(&broadcast_receiver, receive_buffer, receive_buffer_size);

		if (num_bytes > 0) {
			// We've received Client's Health message, so we know its IP address, but we don't know it's RX port
			// because it cannot setup its RX port until it knows our IP, but we can set up our RX port --> recvfrom

			const auto client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);

			if (client_health_packet->packageType == PackageType::Health && client_health_packet->cmd == Command::connect) {
				Connection* connection = nullptr;
				auto client_id = ntohll(client_health_packet->client_id);
				const auto client_recv_port = ntohs(client_health_packet->port);
				const auto package_id = ntohl(client_health_packet->package_id);
				if (client_id == 0)
				{
					// Connection is in the process of being initialized
					// It takes a round trip from the Client to complete
					if (connections_in_process.contains(package_id))
					{
						auto id = connections_in_process[package_id];
						if (connections.contains(id))
						{
							client_id = id;
							connection = connections[client_id].get();
						}
						// Can we clear connection_in_process
						if (connection->client_is_connected())
						{
								connections_in_process.erase(package_id);
								connection->clear_connected_package_id();
						}
					}
					else {
						// Create a new Connection
						while (client_id == 0) {
							const uint64_t uid = static_cast<uint64_t>(make_uid(gen)) << 32;
							client_id = uid | package_id;
							// extra check to make sure client_id is not in use
							if (connections.contains(client_id))
							{
								client_id = 0;
							}
						}
						connections[client_id] = std::make_unique<Connection>(udp.get(), client_id, broadcast_receiver.get_from_ip());
						connections_in_process[package_id] = client_id;
						connection = connections[client_id].get();
					}
				}
				else
				{
					// Client has restarted -- No need to reinitialize
					// BUT -- Client RX port should have changed which impact the transmitter
					//
					if (connections.contains(client_id))
					{
						connection = connections[client_id].get();
					}
					else
					{
						// Client has alread been assigned an ID from a previous execution
						// no reason to create a new one
						connections[client_id] = std::make_unique<Connection>(udp.get(), client_id, broadcast_receiver.get_from_ip());
						connections_in_process[package_id] = client_id;
						connection = connections[client_id].get();
					}
				}
				if (connection != nullptr) {
					//
					// Broadcast out IP and RX port as long as we are
					// receiving broadcast from the client
					//
					if (broadcaster.valid()) {
						connection->broadcast_health(&broadcaster, package_id);
					}

					// set up transmitter if required
					connection->check_transmitter(client_recv_port, broadcast_receiver.get_from_ip());
				}
			}
		}
	} // End of Broadcast


	//
	//
	// CLIENTS RECEIVE AND SEND
	//
	//
	for (const auto& [client_id, connection] : connections)
	{
		// **************************************************
		// RECEIVE
		// **************************************************

		if (connection != nullptr) {
			// Receiving on Client's RX port
			auto num_bytes = connection->receive(receive_buffer, receive_buffer_size);
			if(num_bytes > 0)
			{
				const auto packet = reinterpret_cast<DefaultPacket*>(receive_buffer);
				if(packet->packageType == PackageType::SimData)
				{
					LOG() << "Sim Data Received";
				}
			}



			if (!connection->client_is_connected())
			{
				LOG() << "Client " << connection->client_id() << " has stopped sending packages.";
			}

			//
			// Process Data from Client
			//

		}


		// **************************************************
		// SEND
		// **************************************************

			//
			// Send Data to Client
			//
		if (hw_client_send_data_available) {

		}
		else
		{
			connection->send_health();
		}
		if (connection->client_timeout())
		{
			timeout_clients.push_back(client_id);
		}
	}

	// Clean UP
	if (!timeout_clients.empty())
	{
		for (auto id : timeout_clients)
		{
			connections.erase(id);
		}
		timeout_clients.clear();
	}

	// 1Hz task
	if (current_cycle >= ONE_SECOND)
	{


	}


	// 20 Hz task
	//recv_client_data();

	// update counters
	if (currentSeconds >= ONE_MINUTE)
	{
		currentSeconds = 0;
	}
	else
	{
		currentSeconds++;
	}

	// update counters
	if (current_cycle >= ONE_SECOND)
	{
		current_cycle = 0;
	}
	else
	{
		current_cycle++;
	}
}


