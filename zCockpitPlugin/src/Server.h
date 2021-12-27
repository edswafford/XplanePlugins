#pragma once
#include <mutex>
#include <string>
#include <map>
#include <random>
#include <vector>

#include "../../../shared_src/sim737Common.h"
#include "../../../shared_src/network.h"
#include "Connection.h"
#include "Connection.h"


class Server
{
public:

	Server();
	virtual ~Server();

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;
	Server(Server&&) = delete;
	Server& operator=(Server&&) = delete;

	void update();



private:
	// Random number generator
	static std::mt19937 gen;

	std::unique_ptr<Udp> udp{ nullptr };
	std::unique_ptr<Network> network{ nullptr };


	std::map<uint64_t, std::unique_ptr<Connection>> connections;
	std::map<uint32_t, uint64_t> connections_in_process;
	std::vector<uint64_t> timeout_clients;

	sockaddr_in broadcast_addr_of_sender;
	int sockaddr_in_size = sizeof(broadcast_addr_of_sender);


	short server_broadcast_rx_port{ SERVER_BROADCAST_RX_PORT_ADDRESS };


	sockaddr_in server_broadcast_addr {};



	int receive_buffer_size{ RECEIVE_BUFFER_SIZE };
	char receive_buffer[RECEIVE_BUFFER_SIZE]{};

	SOCKET revc_broadcast_socket{ INVALID_SOCKET };
	bool revc_broadcast_socket_valid{ false };

	SOCKET send_broadcast_socket{ INVALID_SOCKET };
	bool send_broadcast_socket_valid{ false };


	int current_cycle{ ONE_SECOND };
	int currentSeconds{ ONE_MINUTE };



	//////////
	//
	//  TODO 
	//



	bool hw_client_send_data_available{ false };
	
};