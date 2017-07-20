#pragma once

#include <SDL2\SDL_net.h>

#include "car_io.h"
#include "main.h"

class SimIntegration {
public:
	virtual CarState Begin(stringmap driver_params) = 0;
	virtual void Cycle(CarSteers&, CarState&) = 0;
};

class TorcsIntegration : public SimIntegration {
	UDPsocket socket;
	SDLNet_SocketSet socket_set;
	IPaddress all;
	IPaddress simulator_address = { 0, 3001 };

	std::array<uint8_t, USHRT_MAX> data_in, data_out;
	UDPpacket packet_in, packet_out;

	int port = 3001;

	void PreparePacketOut(std::string data);
	void ParseCarState(std::string in, CarState& state);
	std::string ParseString(char** cursor);
	
public:
	virtual void Cycle(CarSteers&, CarState&);
	virtual CarState Begin(stringmap driver_params);

	TorcsIntegration(stringmap params);
};