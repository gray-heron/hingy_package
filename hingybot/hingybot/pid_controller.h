#pragma once

#include <cmath>

struct PidController {
	float P, I, D, max;

	float integral = 0;
	float last_v = 0;

	PidController() {};
	PidController(float K, float Ti, float Td, float max) {
		P = K;
		I = Ti;
		D = Td;

		this->max = max;
	};

	float Update(float target, float state, float dt) {
		integral += (target - state) * dt;

		float pc = (target - state) * P;
		float ic = integral * I;
		float dc = (state - last_v) * D / dt;

		last_v = state;

		float out = pc + ic + dc;

		if (std::abs(out) > max)
			out /= std::abs(out) * max;

		return out;
	}

	void AntiWindup()
	{
		integral = 0.0;
	}
};