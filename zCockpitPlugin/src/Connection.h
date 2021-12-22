#pragma once
#include <cstdint>
#include <string>
#include <winsock2.h>
#include "../../../shared_src/network.h"
#include "../../../shared_src/logger.h"

extern logger LOG;


class Connection
{

public:
	Connection() = delete;
	virtual ~Connection();
	void create_sendto_socket(const uint16_t port);
	void create_recvfrom_socket();
	explicit Connection(Network* network_ptr, const uint32_t id, const uint32_t msg_id, const unsigned long ip);

	std::tuple<bool, int>  receive();

	void broadcast_health(SOCKET& socket, uint16_t client_port, uint32_t msg_id);

	void send_health();

	bool client_is_connected() const { return connected; }
	uint32_t client_msg_id() const { return msg_id; }

private:
	Network* network{ nullptr };
	uint32_t id{ 0 };
	uint32_t msg_id{ 0 };

	unsigned long client_ip{ 0 };

	std::string client_ip_str{};



	///////////////////


	SOCKET recv_from_socket{ INVALID_SOCKET };
	bool recv_from_socket_valid{ false };
	uint16_t recvfrom_port{ 0 };

	SOCKET send_to_socket{ INVALID_SOCKET };
	bool send_to_socket_valid{ false };
	uint16_t sendto_port{ 0 };

	sockaddr_in hw_client_send_to_server_addr{};


	sockaddr_in client_broadcast_addr{};
	short client_broadcast_rx_port{ CLIENT_BROADCAST_RX_PORT_ADDRESS };


	bool connected{ false };

	// Health Packets	
	HealthPacket my_health_packet{};


	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};

	int hw_client_time_since_last_send{ ONE_SECOND };
	int hw_client_time_since_last_recv{ FIVE_SECONDS };
};
