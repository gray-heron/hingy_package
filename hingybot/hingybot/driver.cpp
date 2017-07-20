#include "driver.h"

HingyDriver::HingyDriver(stringmap params) : Driver(params)
{
}

CarSteers HingyDriver::Cycle(const CarState&)
{
	return CarSteers();
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
