#include "driver.h"

using std::string;

HingyDriver::HingyDriver(stringmap params) : Driver(params)
{
	bool gui = atoi(params["gui"].c_str());
	bool record = atoi(params["stage"].c_str()) == 0;

	float force1 = atof(params["force1"].c_str());
	float force2 = atof(params["force2"].c_str());

	int hinges_iterations = atoi(params["hinges_iterations"].c_str());

	if (gui) 
		track = std::make_shared<HingyTrackGui>(params["track"], 1000, 1000);
	else
		track = std::make_shared<HingyTrack>(params["track"]);

	if (!record) {
		track->ConstructBounds();
		track->ConstructHinges(13);

		if (!file_exists((string)"tmp/" + params["track"] + ".hinges")) {
			for (int i = 0; i < hinges_iterations; i++) {
				track->SimulateHinges(force1, force2);
				if (gui && i % 1000 == 0)
					std::static_pointer_cast<HingyTrackGui>(track)->TickGraphics();
			}

			track->CacheHinges();
		} else {
			track->LoadHingesFromCache();
		}
	} else {
		track->BeginRecording();
	}
}

HingyDriver::~HingyDriver()
{
}

CarSteers HingyDriver::Cycle(const CarState& state)
{
	CarSteers out;

	float dt = state.current_lap_time - last_timestamp;
	if (dt < 0.0)
		dt = last_dt;
	last_dt = dt;
	last_timestamp = state.current_lap_time;

	out.gear = 1;
	out.gas = std::max(0.0f, -state.speed_x + 30.0f);
	out.hand_brake = std::max(0.0f, state.speed_x - 30.0f);

	track->MarkWaypoint(
		state.absolute_odometer,
		state.cross_position,
		-state.cross_position,
		(state.wheels_speeds[0] - state.wheels_speeds[1]) * -dt,
		state.speed_x);

	return out;
}

stringmap HingyDriver::GetSimulatorInitParameters()
{
	stringmap params;
	params["ds-9"] = "-90"; params["ds-8"] = "-75"; params["ds-7"] = "-60";
	params["ds-6"] = "-45"; params["ds-5"] = "-30"; params["ds-4"] = "-20";
	params["ds-3"] = "-15"; params["ds-2"] = "-10"; params["ds-1"] = "-5";
	params["ds0"] = "0";
	params["ds9"] = "90"; params["ds8"] = "75"; params["ds7"] = "60";
	params["ds6"] = "45"; params["ds5"] = "30"; params["ds4"] = "20";
	params["ds3"] = "15"; params["ds2"] = "10"; params["ds1"] = "5";

	return params;
}
