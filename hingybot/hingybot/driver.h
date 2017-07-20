#pragma once

#include <memory>

#include "main.h"
#include "car_io.h"
#include "hingy_track.h"
#include "pid_controller.h"

class Driver {
public:
	Driver(stringmap params) {};
	virtual ~Driver() {};

	virtual void Cycle(CarSteers& steers, const CarState& state) = 0;
	virtual stringmap GetSimulatorInitParameters() = 0;
};

class HingyDriver : public Driver {
private:
	std::shared_ptr<HingyTrack> track;
	float last_timestamp = 0.0f;
	float last_dt = 0.0f;

	PidController cross_position_control;
	PidController angle_control;
	PidController speed_control;

	void SetClutchAndGear(const CarState& state, CarSteers& steers);

public:
	HingyDriver(stringmap params);
	virtual ~HingyDriver();

	virtual void Cycle(CarSteers& steers, const CarState& state);
	virtual stringmap GetSimulatorInitParameters();

	
};