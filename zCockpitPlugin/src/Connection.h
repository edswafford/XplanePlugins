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
	explicit Connection(Network* network_ptr, const uint64_t id, const unsigned long ip);

	std::tuple<bool, int>  receive();

	void broadcast_health(SOCKET& socket, uint16_t client_port, uint32_t package_id);

	void send_health();

	bool client_is_connected() const { return connected; }
	uint64_t client_id() const { return id; }
	bool client_timeout() const { return ten_minute_timeout; }
	uint32_t get_connected_package_id() const { return connected_package_id; }
	void clear_connected_package_id() { connected_package_id = 0; }

private:
	Network* network{ nullptr };
	uint64_t id{ 0 };

	unsigned long client_ip{ 0 };
	uint32_t connected_package_id{ 0 };

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
	bool ten_minute_timeout{ false };


	// Health Packets	
	HealthPacket my_health_packet{};


	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};

	int hw_client_time_since_last_send{ ONE_SECOND };
	int hw_client_time_since_last_recv{ FIVE_SECONDS };
};
