#pragma once
#include <map>
#include <memory>
#include <string>
#include <winsock2.h>

class Client
{

public:
	Client();

	const std::unique_ptr<Client>& get_client()
	{

		return nullptr;
	}

	const std::unique_ptr<Client>& create_client()
	{
		

		return nullptr;
	}

	SOCKET hw_client_recv_from_socket{ INVALID_SOCKET };
	bool hw_client_recv_from_socket_valid{ false };

	SOCKET hw_client_send_to_socket{ INVALID_SOCKET };
	bool hw_client_send_to_socket_valid{ false };

	sockaddr_in hw_client_send_to_server_addr{};

	bool hw_client_healthy{ false };
	unsigned long hw_client_ip{ 0 };
	int hw_client_recv_port{ 0 };
	std::string hw_client_ip_str{ "unknown" };
	bool hw_client_send_data_available{ false };
	int server_recvfrom_port_for_hw_client{ 0 };
	std::string server_recvfrom_port_for_hw_client_str{ "0" };

	std::map<std::string, std::unique_ptr<Client>> clients;
};


/*
	if (client_health_packet->cmd == Command::connect)
	{
		if (clients.contains(client_health_packet->id.client_type))
		{
			//auto client_ = clients[client_health_packet->id.client_type];
			//auto num_clients = client_.size();
			//auto client = std::make_unique<Client>();
			//client_.push_back(client);
		}
	}
*/