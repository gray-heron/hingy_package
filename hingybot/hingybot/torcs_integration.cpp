#include <climits>
#include <unordered_map>

#include "torcs_integration.h"
#include "main.h"

using std::string;

const std::unordered_map<string, std::pair<size_t, int>> car_state_offset_table = {
	{ "angle",			{ offsetof(CarState, angle),				1  } },
	{ "curLapTime",		{ offsetof(CarState, current_lap_time),		1  } },
	{ "distFromStart",	{ offsetof(CarState, absolute_odometer),	1  } },
	{ "rpm",			{ offsetof(CarState, rpm),					1  } },
	{ "speedX",			{ offsetof(CarState, speed_x),				1  } },
	{ "speedY",			{ offsetof(CarState, speed_y),				1  } },
	{ "speedZ",			{ offsetof(CarState, speed_z),				1  } },
	{ "track",			{ offsetof(CarState, sensors),				19 } },
	{ "trackPos",		{ offsetof(CarState, cross_position),		1  } },
	{ "wheelSpinVel",	{ offsetof(CarState, wheels_speeds),		4  } },
	{ "z",				{ offsetof(CarState, height),				1  } },
};

TorcsIntegration::TorcsIntegration(stringmap params)
{
	SDLNet_Init();

	packet_in.data = data_in.data();
	packet_out.data = data_out.data();
	packet_in.maxlen = USHRT_MAX;
	packet_out.maxlen = USHRT_MAX;
	packet_out.channel = -1;

	if (params.find("port") != params.end())
		port = std::stoi(params["port"]);

	if (params.find("host") != params.end()) {
		if (SDLNet_ResolveHost(&simulator_address, params["ip"].c_str(), port) == -1) {
			log_error("Simulator address couldn't be resolved!");
			throw;
		}
	}
	else {
		if (SDLNet_ResolveHost(&simulator_address, "localhost", port) == -1) {
			log_error("Simulator address couldn't be resolved!");
			throw;
		}
	}
}

void TorcsIntegration::PreparePacketOut(string data)
{
	memcpy(data_out.data(), data.c_str(), data.length());
	packet_out.len = (int)data.length();
}

void TorcsIntegration::ParseCarState(std::string in, CarState& state)
{
	char* cursor = (char*)in.c_str();
	string param_name;

	while (*cursor != '\0') {
		if (*(cursor++) != '(') {
			log_error("Unimplemented case!");
			throw;
		}

		param_name = ParseString(&cursor);
		auto offset = car_state_offset_table.find(param_name);

		if (offset != car_state_offset_table.end()) {
			for (int i = 0; i < offset->second.second; i++) {
				*((float*)
					((uint8_t*)&state + offset->second.first + (i * sizeof(float)))
				) = strtof(cursor, &cursor);
			}

			if (*(cursor++) != ')')
				throw;
		}
		else {
			while (*(cursor++) != ')');
		}
	}

}

string TorcsIntegration::ParseString(char ** cursor)
{
	string out;
	char * tmp_cursor = *cursor;

	while (*(tmp_cursor++) != ' ')
		out += *(tmp_cursor - 1);

	*cursor = tmp_cursor;

	return out;
}

CarState TorcsIntegration::Begin(stringmap params)
{
	CarState out;
	packet_out.address = simulator_address;

	socket = SDLNet_UDP_Open(0);
	if (!socket) {
		log_error(string("UDP socket couldn't be opened, error: ") +
			SDLNet_GetError());
		throw;
	}

	string init_string = "SCR(init", instring;
	for (int i = -9; i <= 9; i++) {
		init_string += string(" ") + params[string("ds") + std::to_string(i)];
	}
	init_string += ")";

	PreparePacketOut(init_string);

	while (true) {
		do {
			if (!SDLNet_UDP_Send(socket, -1, &packet_out))
			{
				log_error(string("UDP send error: ") +
					SDLNet_GetError());
				throw;
			}

			SDL_Delay(50);
		} while (!SDLNet_UDP_Recv(socket, &packet_in));

		if (strcmp((char*)data_in.data(), "***identified***") == 0) {
			break;
		}
		else {
			log_warning("Communication error on init!");
			throw;
		}
	}

	while (!SDLNet_UDP_Recv(socket, &packet_in));

	if (data_in[0] == '*' && data_in[1] == '*' && data_in[2] == '*') {
		log_error("Unimplemented case!");
		throw;
	}

	data_in[packet_in.len] = '\0';

	ParseCarState((char*)data_in.data(), out);
	return out;
}

void TorcsIntegration::Cycle(CarSteers&, CarState&)
{

}
