#pragma once
#include <mutex>
#include <string>

#include "../../../shared_src/sim737Common.h"
#include "../../../shared_src/network.h"


static const int RECEIVE_BUFFER_SIZE = 4096;

class Client
{
public:

	Client();
	virtual ~Client();

	void update();

	void drop();


private:

	short server_broadcast_rx_port{ SERVER_BROADCAST_RX_PORT_ADDRESS };
	short client_broadcast_rx_port{ CLIENT_BROADCAST_RX_PORT_ADDRESS };

	int health_byte_count{ 0 };
	int health_packet_count{ 0 };
	int total_health_packet_count{ 0 };

	sockaddr_in send_to_server_addr;

	std::string client_ip_address;
	bool client_ip_address_changed{ true };

	Network* network;

	struct sockaddr_in server_broadcast_addr {};
	struct sockaddr_in client_broadcast_addr {};
	sockaddr_in server_tx_addr{};
	

	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};
	int number_of_bytes_sent{ 0 };
	int current_cycle{ ONE_SECOND };
	int currentSeconds{ ONE_MINUTE };



	/////////
	//
	Network::NETWORK_STATUS recv_broadcast_status{ Network::NETWORK_STATUS::UNKNOWN };
	SOCKET revc_broadcast_socket{ INVALID_SOCKET };
	SOCKET send_broadcast_socket{ INVALID_SOCKET };

	// Hardware Client
	bool hw_client_healthy{ false };
	unsigned long hw_client_ip{ 0 };
	bool hw_client_recv_from_socket_valid{ false };
	SOCKET hw_client_recv_from_socket{ INVALID_SOCKET };
	HealthPacket my_health_packet_for_hw_client{};
	bool hw_client_send_to_socket_valid{ false };
	SOCKET hw_client_send_to_socket{ INVALID_SOCKET };




	// Health Packets
	const int health_packet_size{ static_cast<int>(sizeof(HealthPacket))};

	char health_buffer[64];

	
};