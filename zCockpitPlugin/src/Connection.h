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

	explicit Connection(Udp* udp_ptr, const uint64_t id, const unsigned long client_ip);
	virtual ~Connection();

	void check_transmitter(uint16_t client_recv_port, const uint32_t ip);

	Connection() = delete;
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection(Connection&&) = delete;
	Connection& operator=(Connection&&) = delete;

	int receive();

	void broadcast_health(TxEndPoint* broadcast_receiver, const uint32_t package_id);
	void send_health();


	bool client_is_connected() const { return connected; }

	uint32_t can_clear_connection_in_process() const;

	uint64_t client_id() const { return unique_id; }
	bool client_timeout() const { return ten_minute_timeout; }
	void clear_connected_package_id() { connected_package_id = 0; }

private:
	void create_transmitter(const uint16_t port, const uint32_t ip);
	void create_receiver(const unsigned long client_ip);


	Udp* udp{ nullptr };
	uint64_t unique_id{ 0 };

	uint32_t connected_package_id{ 0 };

	std::string client_ip_str{};


	uint16_t transmitter_port{ 0 };


	bool connected{ false };
	bool ten_minute_timeout{ false };


	// Health Packets	
	HealthPacket my_health_packet{};


	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};

	int client_time_since_last_send{ ONE_SECOND };
	int client_time_since_last_recv{ FIVE_SECONDS };


	TxEndPoint transmitter;
	RxEndPoint receiver;
};
