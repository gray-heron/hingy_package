#include <fstream>

#include "driver.h"
#include "GeneRegulatoryNetwork.h"

using std::string;
using std::fstream;
using std::ios;

HingyDriver::HingyDriver(stringmap params) : Driver(params)
{
    bool gui = std::stoi(params["gui"]);
    bool record = std::stoi(params["stage"]) == 0;

    float force1 = std::stof(params["force1"]);
    float force2 = std::stof(params["force2"]);

    float sa = std::stof(params["sa"]);
    float sb = std::stof(params["sb"]);
    float sc = std::stof(params["sc"]);

    master_output_factor = std::stof(params["master_output_factor"]);
    steering_factor = std::stof(params["steering_factor"]);

    speed_factor = std::stof(params["speed_factor"]);
    speed_base = std::stof(params["speed_base"]);

    if (params.find("grn") != params.end()) {
        size_t grn_filesize = file_size(params["grn"]);

        if (grn_filesize != -1) {
            fstream infile(params["grn"], ios::in | ios::binary);

	    assert(infile.is_open());
	    
            std::vector<uint8_t> data_vector;
            data_vector.resize(grn_filesize);
            infile.read((char*)data_vector.data(), grn_filesize);
            infile.close();

            grn_inputs.resize(13);

            fusion_grn = std::unique_ptr<GeneRegulatoryNetwork<2, float>>
		(new GeneRegulatoryNetwork<2, float>());
            fusion_grn->Deserialize(data_vector);
            fusion_grn->Reset();
        }
        else {
            log_warning("Couldn't open the GRN file!");
        }
    }

    int hinges_iterations = atoi(params["hinges_iterations"].c_str());

    if (gui)
        track = std::make_shared<HingyTrackGui>(params["track"], 1000, 1000);
    else
        track = std::make_shared<HingyTrack>(params["track"]);

    if (!record) {
        if (!file_exists(params["track"])) {
            log_error("Couldn't load the track!");
            throw;
        }

        track->ConstructBounds();
        track->ConstructHinges(13);

        if (!track->LoadHingesFromCache()) {
            for (int i = 0; i < hinges_iterations; i++) {
                track->SimulateHinges(force1, force2);
                if (gui && i % 1000 == 0)
                    std::static_pointer_cast<HingyTrackGui>(track)->TickGraphics();
            }

            track->CacheHinges();
        }
        track->SimulateHinges(force1, force2);

        track->ConstructSpeeds(sa, sb, sc);
        if (gui)
            std::static_pointer_cast<HingyTrackGui>(track)->TickGraphics();
    }
    else {
        track->BeginRecording();
    }

    cross_position_control = PidController(-0.29f, -0.0f, 0.0f, 1.0f);
    angle_control = PidController(-2.5f, -0.0f, 0.0f, 1.0f);
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

    auto hinge_data = track->GetHingePosAndHeading(state.absolute_odometer);
    auto hinge_data_next = track->GetHingePosAndHeading(state.absolute_odometer + track->hinge_sep / 4.0f);
    hinge_data.second = (hinge_data.second + hinge_data_next.second) / 2.0f;

    float master_out = cross_position_control.Update(hinge_data.first * 0.95f,
        -state.cross_position, dt);

    steers.steering_wheel = angle_control.Update(
        hinge_data.second - master_output_factor * master_out,
        steering_factor * state.angle, dt
    );

    track->MarkWaypoint(
        state.absolute_odometer,
        state.cross_position,
        -state.cross_position,
        (state.wheels_speeds[0] - state.wheels_speeds[1]) * -dt,
        state.speed_x);

    float target_speed = std::min(GetTargetSpeed(state), GetTargetSpeed(state));

    steers.hand_brake = std::max(-steers.gas, 0.0f);
    steers.gas = (target_speed - state.speed_x) / 35.0f;

    if(std::abs(steers.steering_wheel) > 0.9f)
	steers.gas /= 3.0f;
    
    if(std::abs(state.cross_position) > 1.0f)
	steers.gas = 0.1f;

    if(state.speed_x < 30.0f)
	steers.steering_wheel /= 1.5f;

    if(state.absolute_odometer < 100.0f){
	steering_enabled = true;
    }
    
    if (std::abs(state.angle) > HALF_PI ||
	(std::abs(state.cross_position) > 1.0f && state.speed_x < 5.0f))
	SetReverseGear(state, steers);
    else 
	SetClutchAndGear(state, steers);

    if(state.speed_x < 0.0f){
	steers.steering_wheel *= -1;
    }

    if(!steering_enabled)
	steers.steering_wheel = 0.0f;
}

void HingyDriver::SetReverseGear(const CarState & state, CarSteers & steers)
{
    int curent_gear = (int)state.gear;

    if(state.speed_x  > 10.0f){
	steers.clutch = 1.0f;
	steers.hand_brake = 1.0f;
	steers.gas = 0.0f;

	return;
    }
   
    steers.clutch = std::max(0.0f, steers.clutch - 0.1f);
    steers.gear = -1;
    steers.gas = 1.0f;
    
    last_rpm = state.rpm;
}
 
void HingyDriver::SetClutchAndGear(const CarState & state, CarSteers & steers)
{
    steers.gear = (int)state.gear;
    
    if (state.rpm > 9200.0f){
        steers.clutch = steers.gear > 0 ? 0.5f : 1.0f;
        gear_dir = 1;
    } else if (state.rpm < 5000.0f && last_rpm >= 5000.0f) {
        steers.clutch = 0.5f;
        gear_dir = -1;
    }	
    
    if (steers.clutch > 0.4f) {
        steers.gas = 0.0f;
        steers.gear += gear_dir;
    }

    if(state.speed_x < 30.0f && steers.gas > 0.4f)
    steers.gear = 1;

    steers.clutch = std::max(0.0f, steers.clutch - 0.1f);
    last_rpm = state.rpm;
}

float HingyDriver::GetTargetSpeed(const CarState& state)
{
    float agent_speed;

    if (track->Recording())
        return 45.0f;

    float hs = track->GetHingeSpeed();
    if (hs < 1.0f)
        agent_speed = hs * speed_factor + speed_base;
    else
        agent_speed = 1000.0f;

    if (fusion_grn) {
        grn_inputs[0] = 1.0f;
        grn_inputs[1] = state.speed_x / 300.0f;
        grn_inputs[2] = std::max(0.0f, state.cross_position);
        grn_inputs[3] = std::max(0.0f, -state.cross_position);
        for (int i = 0; i < 9; i++) 
            grn_inputs[4 + i] = state.sensors[i * 2 + 1];
        
        auto outputs = fusion_grn->Step(grn_inputs);
        return (outputs[0] - outputs[1]) / (outputs[0] + outputs[1])
            * 300.0f;
    }
    else {
        return agent_speed;
    }
}

void HingyDriver::StuckOverride(CarSteers& steers, const CarState& state, float dt) {
    /*    if(state.speed_x > 30.0f || state.forward > 30.0f) {
	stuck_counter = 0.0f;
	return;
    }

    stuck_counter += dt;

    if(stuck_counter > 2.0f) {
	log_info("Stuck!\n");
	} */
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
