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

Server::Server()
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

Server::~Server()
{
	Network::drop();
}

void Server::drop()
{
	if (revc_broadcast_socket_valid) {
		network->close_socket(revc_broadcast_socket);
	}
	if (send_broadcast_socket_valid) {
		network->close_socket(send_broadcast_socket);
	}
	if (hw_client_recv_from_socket_valid) {
		network->close_socket(hw_client_recv_from_socket);
	}
	if (hw_client_send_to_socket_valid) {
		network->close_socket(hw_client_send_to_socket);
	}

	network->close_socket(hw_client_recv_from_socket);
	Network::drop();

}


void Server::update()
{
	// *****************
	// BROADCAST
	// *****************

	if (!revc_broadcast_socket_valid) {
		// we need to init the socket for receiving the broadcast message from client(s)
		revc_broadcast_socket = network->create_bind_socket("broadcast", revc_broadcast_socket_valid, SERVER_BROADCAST_RX_PORT_ADDRESS);;
	}

	// Always look for new broadcast
	if (revc_broadcast_socket_valid) {

		// setup address structure that will be filled with the client's ip 
		struct sockaddr_in broadcast_addr_of_sender;
		int addr_size = sizeof(broadcast_addr_of_sender);
		memset((char*)&broadcast_addr_of_sender, 0, addr_size);

		// look for Clients
		auto num_bytes = network->recv_data(revc_broadcast_socket, receive_buffer, receive_buffer_size, &broadcast_addr_of_sender, &addr_size);
		revc_broadcast_socket_valid = revc_broadcast_socket != INVALID_SOCKET;

		// We've received Client's Health message, so we know its IP address, but we don't know it's RX port
		// because it cannot setup its RX port until it knows our IP
		if (num_bytes > 0)
		{
			const auto client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);

			if (client_health_packet->id.client_type == ClientType::hardware && !hw_client_healthy) {

				// Set up sendto for sending message to Client
				hw_client_ip = broadcast_addr_of_sender.sin_addr.S_un.S_addr;
				hw_client_ip_str = network->get_ip_address(broadcast_addr_of_sender);

				// Client RX port will be zero until the Cleint sets up its RX port
				// which happen when we send a reply
				// 
				// Check for Health packet
				HealthPacket* client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);
				if (client_health_packet->packageType == PackageType::Health) {
					auto latest_port = ntohs(client_health_packet->port);
					LOG() << "Broadcast Health message received from IP " << hw_client_ip_str << " RX Port is " << latest_port;

					//
					// Set up sendto port if we can
					//
					if (latest_port != 0) {
						//has the Client's RX port changed 
						if (hw_client_send_to_socket_valid && hw_client_recv_port != latest_port) {
							network->close_socket(hw_client_send_to_socket);
							hw_client_send_to_socket_valid = false;
							hw_client_recv_port = latest_port;
						}

						else if (!hw_client_send_to_socket_valid) {
							hw_client_recv_port = latest_port;
						}
						if (!hw_client_send_to_socket_valid) {
							if (hw_client_recv_port) {
								// Initialize address and Port for sending to Client
								//
								hw_client_send_to_server_addr.sin_family = AF_INET;
								hw_client_send_to_server_addr.sin_port = htons(hw_client_recv_port);
								hw_client_send_to_server_addr.sin_addr.s_addr = hw_client_ip;

								// initialize socket for sending
								hw_client_send_to_socket = network->create_socket("sendto", hw_client_send_to_socket_valid);
								hw_client_healthy = hw_client_send_to_socket_valid;
							}
						}
					}
				}


				// Setup our recvfrom to Receive Client messages
				if (!hw_client_recv_from_socket_valid) {
					// initialize socket used to receive data from the client
					// Use port 0 so the OS knows to assing a port to us from its pool
					hw_client_recv_from_socket = network->create_bind_socket("recvfrom", hw_client_recv_from_socket_valid, 0, broadcast_addr_of_sender.sin_addr.S_un.S_addr, true);

					if (hw_client_recv_from_socket_valid) {
						// Get the port that the OS assigned us
						// 
						server_recvfrom_port_for_hw_client = network->get_port(hw_client_recv_from_socket);
						if (server_recvfrom_port_for_hw_client == 0) {
							network->close_socket(hw_client_recv_from_socket);
							hw_client_recv_from_socket_valid = false;
						}
						else {
							if (hw_client_recv_from_socket_valid) {
								// Tell client our RX port
								my_health_packet_for_hw_client.port = htons(server_recvfrom_port_for_hw_client);

								// We are broadcasting to all client --> identify which client
								my_health_packet_for_hw_client.id = client_health_packet->id;

								// Client Does know our IP or RX port -- so initialize ssoceket for broadcasting
								send_broadcast_socket = network->create_socket("broadcast", send_broadcast_socket_valid, true);
								if (send_broadcast_socket_valid) {
									// Broadcast to Client
									auto num_bytes = sendto(send_broadcast_socket, reinterpret_cast<char*>(&my_health_packet_for_hw_client),
										health_packet_size, 0, (struct sockaddr*)&client_broadcast_addr, sizeof(client_broadcast_addr));

									if (num_bytes == SOCKET_ERROR) {
										const auto error = WSAGetLastError();
										LOG(Level::Severe) << "Cannot send_to broadcast socket winsock error :" << network->getErrorMessage(error);
										network->close_socket(send_broadcast_socket);
										send_broadcast_socket = INVALID_SOCKET;
										send_broadcast_socket_valid = false;
									}
								}
							}
						}
					}
				}
			}

		}
	} // End of Broadcast


	// **************************************************
	// RECEIVE
	// **************************************************

	// Always look for messages on our RX Port
	if (hw_client_recv_from_socket_valid) {
		// Receiving on RX port
		int num_bytes = network->recv_data(hw_client_recv_from_socket, receive_buffer, receive_buffer_size);
		if (num_bytes == SOCKET_ERROR) {
			// Cannot use socket -- it has been closed due to error
			hw_client_recv_from_socket_valid = false;
		}
		else if (num_bytes > 0) {
			LOG() << "received data " << num_bytes;
			hw_client_time_since_last_recv = 0;

			// Check for Health packet
			HealthPacket* client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);
			if (client_health_packet->packageType == PackageType::Health) {
				auto latest_port = ntohs(client_health_packet->port);
				LOG() << "Health message received on My RX port " << server_recvfrom_port_for_hw_client <<" from IP " << hw_client_ip_str << " Client RX Port is " << latest_port;

				//has the Client's RX port changed 
				if (hw_client_send_to_socket_valid && hw_client_recv_port != latest_port) {
					LOG() << "Client IP " << hw_client_ip_str << " Port changed from " << hw_client_recv_port << " to " << latest_port;
					network->close_socket(hw_client_send_to_socket);
					hw_client_send_to_socket_valid = false;
					hw_client_recv_port = latest_port;
				}
				else if (!hw_client_send_to_socket_valid) {
					hw_client_recv_port = latest_port;
				}

				// if sendto socket has not been setup -- we have received the Client's RX port or it has changed
				//
				if (!hw_client_send_to_socket_valid) {
					if (hw_client_recv_port) {
						// Initialize address and Port for sending to Client
						//
						hw_client_send_to_server_addr.sin_family = AF_INET;
						hw_client_send_to_server_addr.sin_port = htons(hw_client_recv_port);
						hw_client_send_to_server_addr.sin_addr.s_addr = hw_client_ip;

						// initialize socket for sending
						hw_client_send_to_socket = network->create_socket("sendto", hw_client_send_to_socket_valid);
						if (hw_client_send_to_socket_valid) {
							auto retval = network->sendData(hw_client_send_to_socket, hw_client_send_to_server_addr, reinterpret_cast<char*>(&my_health_packet_for_hw_client), health_packet_size);
							// verify socket after sending data
							hw_client_send_to_socket_valid = hw_client_send_to_socket != INVALID_SOCKET;
							hw_client_healthy = hw_client_send_to_socket_valid;
							if (hw_client_send_to_socket_valid) {
								hw_client_time_since_last_send = 0;
							}
						}
					}
				}
			}
			// Process Data from Client
			//


		}
		// keep sending broadcast message until client responds on RX port
		if (!hw_client_healthy) {
			if (!send_broadcast_socket_valid) {
				send_broadcast_socket = network->create_socket("broadcast", send_broadcast_socket_valid, true);
			}
			if (send_broadcast_socket_valid) {
				// Broadcast to Client
				auto num_bytes = sendto(send_broadcast_socket, reinterpret_cast<char*>(&my_health_packet_for_hw_client),
					health_packet_size, 0, (struct sockaddr*)&client_broadcast_addr, sizeof(client_broadcast_addr));

				if (num_bytes == SOCKET_ERROR) {
					const auto error = WSAGetLastError();
					LOG(Level::Severe) << "Cannot send_to broadcast socket winsock error :" << network->getErrorMessage(error);
					network->close_socket(send_broadcast_socket);
					send_broadcast_socket = INVALID_SOCKET;
					send_broadcast_socket_valid = false;
				}
			}
		}

		if (hw_client_time_since_last_recv > FIVE_SECONDS) {
			// We've lost Comm with Server
			hw_client_healthy = false;
		}
		else {
			hw_client_time_since_last_recv += 1;
		}
	}

	// **************************************************
	// SEND
	// **************************************************

	if (hw_client_healthy) {

		// Send Data to Client
		//
		if (hw_client_send_data_available) {

		}
		else if (hw_client_time_since_last_send > ONE_SECOND) {
			// send Health packet
			// send client my health using client's IP address and port
			auto retval = network->sendData(hw_client_send_to_socket, hw_client_send_to_server_addr, reinterpret_cast<char*>(&my_health_packet_for_hw_client), health_packet_size);
			// verify socket after sending data
			hw_client_send_to_socket_valid = hw_client_send_to_socket != INVALID_SOCKET;
			hw_client_healthy = hw_client_send_to_socket_valid;
			if (hw_client_send_to_socket_valid) {
				hw_client_time_since_last_send = 0;
			}
		}
		if (hw_client_time_since_last_send <= ONE_SECOND) {
			hw_client_time_since_last_send += 1;
		}
		else {
			hw_client_time_since_last_send = ONE_SECOND;
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


