#include <winsock2.h>
#include <ws2tcpip.h>
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
std::uniform_int_distribution<uint32_t> uid(1, UINT32_MAX);



Server::Server()
{
	// Must be called first to initialize Winsock
	network = std::make_unique<Network>();


	// Fill-in server socket's address information
	server_broadcast_addr.sin_family = AF_INET; // Address family to use
	server_broadcast_addr.sin_port = htons(server_broadcast_rx_port); // Port num to use
	server_broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST; // Need this for Broadcast


	LOG() << "Starting Server";
}

Server::~Server()
{
	if (revc_broadcast_socket_valid) {
		network->close_socket(revc_broadcast_socket);
	}
	if (send_broadcast_socket_valid) {
		network->close_socket(send_broadcast_socket);
	}

	network = nullptr;

}



void Server::update()
{
	// *****************
	// BROADCAST
	// *****************

	if (!revc_broadcast_socket_valid) {
		// we need to init the socket for receiving the broadcast message from client(s)
		revc_broadcast_socket = network->create_bind_socket("receive broadcast", revc_broadcast_socket_valid, SERVER_BROADCAST_RX_PORT_ADDRESS);;
	}
	if (!send_broadcast_socket_valid) {
		send_broadcast_socket = network->create_socket("send broadcast", send_broadcast_socket_valid, true);
	}
	// Always look for new broadcast
	if (revc_broadcast_socket_valid) {

		// clear address structure that will be filled with the client's ip 
		memset((char*)&broadcast_addr_of_sender, 0, sockaddr_in_size);

		// look for Broadcasting Clients
		const auto bytes_received = network->recv_data(revc_broadcast_socket, receive_buffer, receive_buffer_size, &broadcast_addr_of_sender, &sockaddr_in_size);
		revc_broadcast_socket_valid = revc_broadcast_socket != INVALID_SOCKET;

		// We've received Client's Health message, so we know its IP address, but we don't know it's RX port
		// because it cannot setup its RX port until it knows our IP, but we can set up our RX port --> recvfrom
		if (bytes_received > 0)
		{
			const auto client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);

			if (client_health_packet->packageType == PackageType::Health && client_health_packet->cmd == Command::connect) {
				Connection* connection = nullptr;
				auto client_id = ntohl(client_health_packet->id);
				const auto client_recv_port = ntohs(client_health_packet->port);
				const auto client_msg_id = ntohl(client_health_packet->msg_id);
				if (connections.contains(client_id))
				{
					connection = connections[client_health_packet->id].get();
				}
				else if (msg_id_map.contains(client_msg_id))
				{
					// We've seen this message -- don't create another connection because the connection is in progress
					client_id = msg_id_map[client_msg_id];
					connection = connections[client_id].get();
				}
				else
				{
					// Check for disconnected Clients
					std::vector<std::uint32_t> id_list;
					for (auto const& id : std::views::keys(connections)) {
						id_list.push_back(id);
					}

					for(auto id : id_list)
					{
						connection = connections[id].get();
						if (!connection->client_is_connected()) {
							if (connection->client_msg_id() == client_msg_id)
							{
								connection = nullptr;
								connections.erase(id);
							}
						}
					}

					// Create Connection -- first time through
					client_id = uid(gen);
					connections[client_id] = std::make_unique<Connection>(network.get(), client_id, client_msg_id, broadcast_addr_of_sender.sin_addr.S_un.S_addr);
					connection = connections[client_id].get();

					// link msg_id to client_id
					msg_id_map[client_msg_id] = client_id;
				}
				if (connection != nullptr) {
					//
					// Broadcast out IP and RX port as long as we are
					// receiving broadcast from the client
					//
					if (send_broadcast_socket_valid) {
						connection->broadcast_health(send_broadcast_socket, client_recv_port, client_msg_id);
						send_broadcast_socket_valid = send_broadcast_socket != INVALID_SOCKET;
					}
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
			auto [rx_valid, num_bytes] = connection->receive();
			if(!connection->client_is_connected())
			{
				// client has stopped sending messages
				// clear the list of client msg_id's so don't leave a dangling client  when same client tries to reconnect
				if(msg_id_map.contains(connection->client_msg_id()))
				{
					msg_id_map.erase(connection->client_msg_id());
				}
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


