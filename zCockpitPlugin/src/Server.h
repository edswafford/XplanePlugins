#pragma once
#include <mutex>
#include <string>

#include "../../../shared_src/sim737Common.h"
#include "../../../shared_src/network.h"


static const int RECEIVE_BUFFER_SIZE = 4096;

class Server
{
public:

	Server();
	virtual ~Server();

	void update();

	void drop();


private:
	Network* network;

	short server_broadcast_rx_port{ SERVER_BROADCAST_RX_PORT_ADDRESS };
	short client_broadcast_rx_port{ CLIENT_BROADCAST_RX_PORT_ADDRESS };

	sockaddr_in send_to_server_addr;
	sockaddr_in server_broadcast_addr {};
	sockaddr_in client_broadcast_addr {};

	

	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};

	SOCKET revc_broadcast_socket{ INVALID_SOCKET };
	bool revc_broadcast_socket_valid{ false };

	SOCKET send_broadcast_socket{ INVALID_SOCKET };
	bool send_broadcast_socket_valid{ false };

	SOCKET hw_client_recv_from_socket{ INVALID_SOCKET };
	bool hw_client_recv_from_socket_valid{ false };

	SOCKET hw_client_send_to_socket{ INVALID_SOCKET };
	bool hw_client_send_to_socket_valid{ false };

	// Hardware Client
	bool hw_client_healthy{ false };
	unsigned long hw_client_ip{ 0 };
	int hw_client_recv_port{0};
	std::string hw_client_ip_str{ "unknown" };
	bool hw_client_send_data_available{ false };
	int server_recvfrom_port_for_hw_client{ 0 };
	std::string server_recvfrom_port_for_hw_client_str{"0"};


	int hw_client_time_since_last_send{ ONE_SECOND };
	int hw_client_time_since_last_recv{ FIVE_SECONDS };

	// Health Packets	
	HealthPacket my_health_packet_for_hw_client{};
	const int health_packet_size{ static_cast<int>(sizeof(HealthPacket))};
	
	int current_cycle{ ONE_SECOND };
	int currentSeconds{ ONE_MINUTE };

};