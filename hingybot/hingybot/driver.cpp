#include "driver.h"

using std::string;


HingyDriver::HingyDriver(stringmap params) : Driver(params)
{
	bool gui = std::stoi(params["gui"]);
	bool record = std::stoi(params["stage"]) == 0;

	float force1 = std::stof(params["force1"]);
	float force2 = std::stof(params["force2"]);

	float sa = std::stof(params["sa"]);
	float sb = std::stof(params["sb"]);
	float sc = std::stof(params["sc"]);

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

		track->ConstructSpeeds(sa, sb, sc);
		if (gui)
			std::static_pointer_cast<HingyTrackGui>(track)->TickGraphics();
	} else {
		track->BeginRecording();
	}

	cross_position_control = PidController(-0.34f, -0.0f, 0.0f, 1.0f);
	angle_control = PidController(-2.7f, -0.0f, 0.0f, 1.0f);
}

void HingyDriver::Cycle(CarSteers& steers, const CarState& state)
{
	float dt = state.current_lap_time - last_timestamp;
	if (dt < 0.0)
		dt = last_dt;
	last_dt = dt;
	last_timestamp = state.current_lap_time;

	if (state.speed_x < 20.0f) {
		cross_position_control.AntiWindup();
		angle_control.AntiWindup();
	}

	auto hinge_data = track->GetHingePosAndHeading();

	float master_out = cross_position_control.Update(hinge_data.first,
		-state.cross_position, dt);

	steers.steering_wheel = angle_control.Update(
		hinge_data.second - 1.0f * master_out,
		1.0f * state.angle, dt
	);

	track->MarkWaypoint(
		state.absolute_odometer,
		state.cross_position,
		-state.cross_position,
		(state.wheels_speeds[0] - state.wheels_speeds[1]) * -dt,
		state.speed_x);

	steers.gas = 0.6f;
	
	SetClutchAndGear(state, steers);
}

void HingyDriver::SetClutchAndGear(const CarState & state, CarSteers & steers)
{
	if (state.rpm > 7500.0f)
		steers.clutch = 1.0f;

	if (state.clutch == 1.0f) {
		steers.gas = 0.0f;
		steers.gear += 1;
	}

	steers.clutch = std::max(0.0f, state.clutch - 0.1f);
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

HingyDriver::~HingyDriver()
{
}
