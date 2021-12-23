#include "Connection.h"

// initialize socket used to receive data from the Server
// Use port 0 so the OS knows to assing a port to us from its pool
void Connection::create_recvfrom_socket()
{
	recv_from_socket = network->create_bind_socket("recvfrom", recv_from_socket_valid, 0, client_ip, true);

	if (recv_from_socket_valid) {
		// Get the port that the OS assigned us
		// 
		recvfrom_port = network->get_port(recv_from_socket);
		if (recvfrom_port != 0) {

			// Tell Server our RX port
			if (recv_from_socket_valid) {
				my_health_packet.port = htons(recvfrom_port);
			}
		}
	}
}

Connection::Connection(Network* network_ptr, const uint64_t id, const unsigned long ip) : network{ network_ptr }, id{ id }, client_ip{ ip }
{
	// Fill-in client socket's address information
	client_broadcast_addr.sin_family = AF_INET; // Address family to use
	client_broadcast_addr.sin_port = htons(client_broadcast_rx_port); // Port num to use
	client_broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

	my_health_packet.simDataID = SIM_DAT_ID;
	my_health_packet.packageType = PackageType::Health;

	// Create recvfrom Socket
	create_recvfrom_socket();

	const auto client_ip_str = network->get_ip_address(ip);
	LOG() << "Broadcast Health message received from client " << id << " IP " << client_ip_str;
}


Connection::~Connection()
{
	if (recv_from_socket_valid) {
		network->close_socket(recv_from_socket);
	}
	if (send_to_socket_valid) {
		network->close_socket(send_to_socket);
	}

}

void Connection::create_sendto_socket(const uint16_t port)
{
	//
	// Set up sendto port if we can
	//
	if (port != 0) {
		//has the Client's RX port changed 
		if (send_to_socket_valid && sendto_port != port) {
			network->close_socket(send_to_socket);
			send_to_socket_valid = false;
			sendto_port = port;
		}

		else if (!send_to_socket_valid) {
			sendto_port = port;
		}
		if (!send_to_socket_valid) {
			// Initialize address and Port for sending to Client
			//
			hw_client_send_to_server_addr.sin_family = AF_INET;
			hw_client_send_to_server_addr.sin_port = htons(sendto_port);
			hw_client_send_to_server_addr.sin_addr.s_addr = client_ip;

			// initialize socket for sending
			send_to_socket = network->create_socket("sendto", send_to_socket_valid);
			if (send_to_socket_valid)
			{
				auto retval = network->sendData(send_to_socket, hw_client_send_to_server_addr, reinterpret_cast<char*>(&my_health_packet), health_packet_size);
				// verify socket after sending data
				send_to_socket_valid = send_to_socket != INVALID_SOCKET;
				if (send_to_socket_valid) {
					hw_client_time_since_last_send = 0;
				}
			}
		}
	}
}


// Receive Client Data
std::tuple<bool, int> Connection::receive()
{
	int num_bytes = 0;
	if (recv_from_socket_valid) {
		// Receiving on RX port
		num_bytes = network->recv_data(recv_from_socket, receive_buffer, receive_buffer_size);
		if (num_bytes == SOCKET_ERROR) {
			// Cannot use socket -- it has been closed due to error
			recv_from_socket_valid = false;
		}
		else if (num_bytes > 0) {
			LOG() << "received " << num_bytes << " from " << id;
			hw_client_time_since_last_recv = 0;
			connected = true;

			// Check for Health packet
			const HealthPacket* client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);
			if (client_health_packet->packageType == PackageType::Health) {
				const auto latest_port = ntohs(client_health_packet->port);
				const auto package_id = ntohl(client_health_packet->package_id);
				connected_package_id = package_id;

				LOG() << "Health message received on My RX port " << recvfrom_port << " from IP " << client_ip_str << " Client RX Port is " << latest_port;

				//has the Client's RX port changed 
				if (send_to_socket_valid && sendto_port != latest_port) {
					LOG() << "Client IP " << client_ip_str << " Port changed from " << sendto_port << " to " << latest_port;
					create_sendto_socket(latest_port);
				}
				else if (!send_to_socket_valid) {
					create_sendto_socket(latest_port);
				}
			}

		}
		if (hw_client_time_since_last_recv > FIVE_SECONDS) {
			// We've lost Comm with Server
			connected = false;
			if(hw_client_time_since_last_recv > TEN_MINUTES)
			{
				// we should remove client
				ten_minute_timeout = true;
			}
		}
		else {
			hw_client_time_since_last_recv += 1;
		}
	}
	return std::make_tuple(recv_from_socket_valid, num_bytes);
}

void Connection::broadcast_health(SOCKET& socket, uint16_t client_port, uint32_t package_id)
{
	if (!send_to_socket_valid && client_port != 0)
	{
		create_sendto_socket(client_port);
	}
	if (!recv_from_socket_valid)
	{
		create_recvfrom_socket();
	}

	//  identify which client for all future messages
	my_health_packet.client_id = htonll(id);

	// We are broadcasting to all client --> Client uses this to verify it is the reply to its broadcast 
	my_health_packet.package_id = htonl(package_id);

	// Broadcast our IP and RX port
	const auto bytes_sent = network->sendData(socket, client_broadcast_addr,
		reinterpret_cast<char*>(&my_health_packet), health_packet_size);

	if (bytes_sent == SOCKET_ERROR) {
		const auto error = WSAGetLastError();
		LOG(Level::Severe) << "Cannot send_to broadcast socket winsock error :" << network->getErrorMessage(error);
		network->close_socket(socket);
		socket = INVALID_SOCKET;
	}

}

void Connection::send_health()
{
	if (hw_client_time_since_last_send > ONE_SECOND) {
		// send Health packet
		HealthPacket health_packet{ SIM_DAT_ID, PackageType::Health, htonll(id), Command::no_op, htons(recvfrom_port) };

		auto retval = network->sendData(send_to_socket, hw_client_send_to_server_addr,
			reinterpret_cast<char*>(&health_packet), health_packet_size);
		// verify socket after sending data
		send_to_socket_valid = send_to_socket != INVALID_SOCKET;
		if (send_to_socket_valid) {
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
