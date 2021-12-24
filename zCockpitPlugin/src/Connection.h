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

	explicit Connection(Network* network_ptr, const uint64_t id, const unsigned long ip);
	virtual ~Connection();

	Connection() = delete;
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection(Connection&&) = delete;
	Connection& operator=(Connection&&) = delete;

	void create_sendto_socket(const uint16_t port);
	void create_recvfrom_socket();


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

	SOCKET recvfrom_socket{ INVALID_SOCKET };
	bool recvfrom_socket_valid{ false };
	uint16_t recvfrom_port{ 0 };

	SOCKET sendto_socket{ INVALID_SOCKET };
	bool sendto_socket_valid{ false };
	uint16_t sendto_port{ 0 };

	sockaddr_in sendto_addr{};


	sockaddr_in client_broadcast_addr{};
	short client_broadcast_rx_port{ CLIENT_BROADCAST_RX_PORT_ADDRESS };


	bool connected{ false };
	bool ten_minute_timeout{ false };


	// Health Packets	
	HealthPacket my_health_packet{};


	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};

	int client_time_since_last_send{ ONE_SECOND };
	int client_time_since_last_recv{ FIVE_SECONDS };
};
