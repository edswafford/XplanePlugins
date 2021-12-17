#include <winsock2.h>
#include <ws2tcpip.h>
#include <charconv>
#include <iostream>
#include <map>

#pragma comment(lib, "Ws2_32.lib")


#include "Server.h"
#include "../../../shared_src/logger.h"
#include "../../../shared_src/Util.h"

extern logger LOG;

Client::Client()
{

	my_health_packet_for_hw_client.simDataID = SIM_DAT_ID;
	my_health_packet_for_hw_client.packageType = PackageType::Health;


	// Fill-in server socket's address information
	server_broadcast_addr.sin_family = AF_INET; // Address family to use
	server_broadcast_addr.sin_port = htons(server_broadcast_rx_port); // Port num to use
	server_broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST; // Need this for Broadcast


	// Fill-in client socket's address information
	client_broadcast_addr.sin_family = AF_INET; // Address family to use
	client_broadcast_addr.sin_port = htons(client_broadcast_rx_port); // Port num to use
	client_broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST; 


	network = Network::instance();

	LOG() << "Starting Server";

}

Client::~Client()
{
	Network::drop();
}

void Client::drop()
{
	network->close_socket(revc_broadcast_socket);

	if (hw_client_send_to_socket_valid) {
		network->close_socket(hw_client_send_to_socket);
	}

	network->close_socket(hw_client_recv_from_socket);
	Network::drop();

}


void Client::update()
{
	if (recv_broadcast_status != Network::NETWORK_STATUS::VALID && recv_broadcast_status != Network::NETWORK_STATUS::HEALTHY) {

		// we need to init the socket for receiving the broadcast message from client(s)

		revc_broadcast_socket = network->create_bind_socket("broadcast", SERVER_BROADCAST_RX_PORT_ADDRESS);
		recv_broadcast_status = revc_broadcast_socket != INVALID_SOCKET ? Network::NETWORK_STATUS::VALID : Network::NETWORK_STATUS::FAILED;
	}

	// We have a valid socket or we've receiving packets from our client
	if (recv_broadcast_status == Network::NETWORK_STATUS::VALID || recv_broadcast_status == Network::NETWORK_STATUS::HEALTHY) {

		// setup address structure that will be filled with the client's ip 
		struct sockaddr_in broadcast_addr_of_sender;
		int addr_size = sizeof(broadcast_addr_of_sender);
		memset((char*)&broadcast_addr_of_sender, 0,addr_size);

		// look for Clients
		auto num_bytes = network->recv_data(revc_broadcast_socket, health_buffer, health_packet_size, &broadcast_addr_of_sender, &addr_size);
		if (revc_broadcast_socket == INVALID_SOCKET) {
			recv_broadcast_status = Network::NETWORK_STATUS::FAILED;
		}
		else if (num_bytes == 0) {
			recv_broadcast_status = Network::NETWORK_STATUS::VALID;
		}
		else {
			recv_broadcast_status = Network::NETWORK_STATUS::HEALTHY;
		}
		//recv_broadcast_status = network->recv_broadcast_health(revc_broadcast_socket, health_buffer, health_packet_size, broadcast_addr_of_sender);

		// We've received Client's Health message, so we know its IP address
		if (recv_broadcast_status == Network::NETWORK_STATUS::HEALTHY)
		{
			HealthPacket* client_health_packet = reinterpret_cast<HealthPacket*>(health_buffer);
			if (client_health_packet->id.client_type == CLIENT_TYPE_HARDWARE && !hw_client_healthy) {
				
				// Set up sendto for sending message to Client
				hw_client_ip = broadcast_addr_of_sender.sin_addr.S_un.S_addr;
				
				// Client RX port is zero right now because the Cleint cannot setup its RX port
				// until it knows the Server's IP
				// So we cannot setup the Our sendto port -- so we broadcast our IP to the Client
				

				// Setup our recvfrom to Receive Client messages
				if (!hw_client_recv_from_socket_valid) {
					// initialize socket used to receive data from the client
					// Use port 0 so the OS knows to assing a port to us from its pool
					hw_client_recv_from_socket = network->create_bind_socket("recvfrom", 0, broadcast_addr_of_sender.sin_addr.S_un.S_addr, true);
					hw_client_recv_from_socket_valid = hw_client_recv_from_socket != INVALID_SOCKET;

					if (hw_client_recv_from_socket_valid) {
						// Get the port that the OS assigned us
						// 
						auto receive_from_port = network->get_port(hw_client_recv_from_socket);
						if (receive_from_port == 0) {
							network->close_socket(hw_client_recv_from_socket);
							hw_client_recv_from_socket_valid = false;
						}
						else {
							std::string receive_from_port_str;
							try {
								receive_from_port_str = std::to_string(receive_from_port);
							}
							catch (std::bad_alloc) {
								std::cout << "Cannot convert RX Port number: " << receive_from_port << " to string";
								network->close_socket(hw_client_recv_from_socket);
								hw_client_recv_from_socket_valid = false;
							}

							if (hw_client_recv_from_socket_valid) {
								// Tell client our RX port
								memcpy(my_health_packet_for_hw_client.port, receive_from_port_str.c_str(), receive_from_port_str.size());
								my_health_packet_for_hw_client.size = static_cast<int>(receive_from_port_str.size());

								// We are broadcasting to all client --> identify which client
								my_health_packet_for_hw_client.id = client_health_packet->id;

								// Client Does know our IP or RX port -- so initialize ssoceket for broadcasting
								send_broadcast_socket = network->create_socket("broadcast", true);
								if (send_broadcast_socket != INVALID_SOCKET) {
									// Broadcast to Client
									auto num_bytes = sendto(send_broadcast_socket, reinterpret_cast<char*>(&my_health_packet_for_hw_client),
										health_packet_size, 0, (struct sockaddr*)&client_broadcast_addr, sizeof(client_broadcast_addr));

									if (num_bytes == SOCKET_ERROR) {
										const auto error = WSAGetLastError();
										LOG(Level::Severe) << "Cannot send_to broadcast socket winsock error :" << network->getErrorMessage(error);
										network->close_socket(send_broadcast_socket);
										send_broadcast_socket = INVALID_SOCKET;
									}
								}
							}
						}
					}
				}
			}

		} 
	} // End of Broadcast


	// Client has sent a Broadcast Message and we have responded with a RX port
	// Look for messages on our RX Port
	//
	if (hw_client_recv_from_socket_valid) {
		// Receiving on RX port
		int num_bytes = network->recv_data(hw_client_recv_from_socket, receive_buffer, receive_buffer_size);
		if (num_bytes == SOCKET_ERROR) {
			// Cannot use socket -- it has been closed due to error
			hw_client_recv_from_socket_valid = false;
		}
		else if (num_bytes > 0) {
			LOG() << "received data " << num_bytes;

			// Communication Established

			// setup the sendto socket unless it is already up and running
			if (!hw_client_send_to_socket_valid) {
				HealthPacket* client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);
				std::string str = std::string(client_health_packet->port);
				int hw_client_recv_port = 0;
				if (Util::to_int(str, hw_client_recv_port)) {

					// Initialize address and Port for sending to Client
					//
					send_to_server_addr.sin_family = AF_INET;
					send_to_server_addr.sin_port = htons(hw_client_recv_port);
					send_to_server_addr.sin_addr.s_addr = hw_client_ip;

					// initialize socket for sending
					hw_client_send_to_socket = network->create_socket("sendto");
					hw_client_send_to_socket_valid = hw_client_send_to_socket != INVALID_SOCKET;
					hw_client_healthy = hw_client_send_to_socket_valid;
				}
			}


			// Process Data from Client
			//


		}
		else {
			// keep sending broadcast message until client responds on RX port
			if (send_broadcast_socket == INVALID_SOCKET) {
				send_broadcast_socket = network->create_socket("broadcast", true);
			}
			if (send_broadcast_socket != INVALID_SOCKET) {
				// Broadcast to Client
				auto num_bytes = sendto(send_broadcast_socket, reinterpret_cast<char*>(&my_health_packet_for_hw_client),
					health_packet_size, 0, (struct sockaddr*)&client_broadcast_addr, sizeof(client_broadcast_addr));

				if (num_bytes == SOCKET_ERROR) {
					const auto error = WSAGetLastError();
					LOG(Level::Severe) << "Cannot send_to broadcast socket winsock error :" << network->getErrorMessage(error);
					network->close_socket(send_broadcast_socket);
					send_broadcast_socket = INVALID_SOCKET;
				}
			}
		}

	}


	// Hardware Client Communication Established
	if (hw_client_healthy) {

		// Send Data to Client
		//

		// send client my health using client's IP address and port
		network->sendData(hw_client_send_to_socket, send_to_server_addr, reinterpret_cast<char*>(&my_health_packet_for_hw_client), health_packet_size);
		// verify socket after sending data
		hw_client_send_to_socket_valid = hw_client_send_to_socket != INVALID_SOCKET;
		hw_client_healthy = hw_client_send_to_socket_valid;
	}



	// 1Hz task
	if (current_cycle >= ONE_SECOND)
	{

		total_health_packet_count += health_packet_count;

		health_byte_count = 0;
		health_packet_count = 0;

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


