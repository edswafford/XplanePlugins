#include "Connection.h"

Connection::Connection(Udp* udp_ptr, const uint64_t id, const unsigned long client_ip) : udp{ udp_ptr }, unique_id{ id }
{
	my_health_packet.simDataID = SIM_DAT_ID;
	my_health_packet.packageType = PackageType::Health;

	// Create recvfrom Socket
	create_receiver(client_ip);

	const auto client_ip_str = udp->get_ip_address(client_ip);
	LOG() << "Broadcast Health message received from client " << id << " IP " << client_ip_str;
}


Connection::~Connection()
{
	if (transmitter.valid()) {
		Udp::close_endpoint(&transmitter);
	}
	if (receiver.valid()) {
		Udp::close_endpoint(&receiver);
	}
}



// initialize socket used to receive data from the Server
// Use port 0 so the OS knows to assing a port to us from its pool
void Connection::create_receiver(const unsigned long client_ip)
{
	// initialize socket used to receive data from_addr the Server
	// Use port 0 so the OS knows to assing a port to us from_addr its pool
	receiver = Udp::create_receiver("recvfrom", 0, client_ip);
	
	if (receiver.valid()) {
		// Get the port that the OS assigned us
		//
		const auto receiver_port = Udp::get_my_port(&receiver);
		if (receiver_port != 0) {
			receiver.port = receiver_port;
			// Tell Server our RX port
			if (receiver.valid()) {
				my_health_packet.port = htons(receiver_port);
			}
		}
	}
}


void Connection::check_transmitter(uint16_t client_recv_port, const uint32_t ip)
{
	if (client_recv_port != 0) {
		//has the Client's RX port changed 
		if (transmitter.valid() && transmitter_port != client_recv_port) {
			LOG() << "Client IP " << client_ip_str << " Port changed from " << transmitter_port << " to " << client_recv_port;
			Udp::close_endpoint(&transmitter);
		}

		// setup the sendto socket unless it is already up and running
		if (!transmitter.valid()) {
				create_transmitter(client_recv_port, ip);
		}
	}
}
void Connection::create_transmitter(const uint16_t port, const uint32_t ip)
{
	//
	// Set up sendto port if we can
	//
	if (!transmitter.valid()) {
		if (port != 0) {

			// initialize socket for sending
			transmitter = Udp::create_transmitter("sendto", port, ip, true);
			if (transmitter.valid()) {
				transmitter_port = port;
				// send_data Server my health using Servers's IP address and RX port
				auto sent_bytes = Udp::send_to(&transmitter, reinterpret_cast<char*>(&my_health_packet), health_packet_size);

				// We can now receive and send_data without using Broadcast
				if (sent_bytes == health_packet_size) {
					if (transmitter.valid()) {
						client_time_since_last_send = 0;
					}
				}
				else
				{
					LOG() << "Error creating Transmitter for Server's RX port " << port;
				}
			}
		}
	}
}



// Receive Client Data
int Connection::receive()
{
	int num_bytes = 0;
	if (receiver.valid()) {
		// Receiving on RX port
		num_bytes = Udp::receive(&receiver, receive_buffer, receive_buffer_size);

		if (num_bytes > 0) {
			LOG() << "received " << num_bytes << " from " << unique_id;
			client_time_since_last_recv = 0;
			connected = true;

			// Check for Health packet
			const auto client_health_packet = reinterpret_cast<HealthPacket*>(receive_buffer);

			if (client_health_packet->packageType == PackageType::Health) {
				const auto latest_port = ntohs(client_health_packet->port);
				const auto package_id = ntohl(client_health_packet->package_id);
				connected_package_id = package_id;

				LOG() << "Health message received on My RX port " << receiver.port << " from IP " << client_ip_str << " Client RX Port is " << latest_port;
			}

		}
		if (client_time_since_last_recv > FIVE_SECONDS) {
			// We've lost Comm with Server
			connected = false;
			if(client_time_since_last_recv > TEN_MINUTES)
			{
				// we should remove client
				ten_minute_timeout = true;
			}
		}
		else {
			client_time_since_last_recv += 1;
		}
	}
	return num_bytes;
}


void Connection::broadcast_health(TxEndPoint* broadcaster, const uint32_t package_id)
{
	//  identify which client for all future messages
	my_health_packet.client_id = htonll(unique_id);

	// We are broadcasting to all client --> Client uses this to verify it is the reply to its broadcast 
	my_health_packet.package_id = htonl(package_id);

	// Broadcast our IP and RX port
	const auto num_bytes = Udp::send_to(broadcaster, reinterpret_cast<char*>(&my_health_packet), health_packet_size);

	if (num_bytes == SOCKET_ERROR) {
		LOG(Level::Severe) << "Broadcast to Client failed";
	}
}

void Connection::send_health()
{
	if (transmitter.valid()) {
		if (client_time_since_last_send > ONE_SECOND) {
			// send Health packet
			HealthPacket health_packet{ SIM_DAT_ID, PackageType::Health, htonll(unique_id), Command::no_op, htons(receiver.port) };

			auto retval = Udp::send_to(&transmitter, reinterpret_cast<char*>(&health_packet), health_packet_size);
			if (transmitter.valid()) {
				client_time_since_last_send = 0;
			}
			else
			{
				LOG() << "Error Sending Health packet for Server's RX port " << transmitter_port;
			}
		}
		if (client_time_since_last_send <= ONE_SECOND) {
			client_time_since_last_send += 1;
		}
		else {
			client_time_since_last_send = ONE_SECOND;
		}
	}
}

uint32_t Connection::can_clear_connection_in_process() const
{
	if (connected_package_id == 0 || !connected)
	{
		return 0;
	}
	return connected_package_id;
}
