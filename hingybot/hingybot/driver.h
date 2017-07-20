#pragma once

#include "main.h"
#include "car_io.h"

class Driver {
public:
	Driver(stringmap params) {};

	virtual CarSteers Cycle(const CarState&) = 0;
	virtual stringmap GetSimulatorInitParameters() = 0;
};

class HingyDriver : public Driver {
public:
	HingyDriver(stringmap params);

	virtual CarSteers Cycle(const CarState&);
	virtual stringmap GetSimulatorInitParameters();
};