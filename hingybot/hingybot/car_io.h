#pragma once

#include <array>

struct CarState {
    float absolute_odometer = 0.0f, cross_position = 0.0f;
    float angle = 0.0f, current_lap_time = 0.0f, rpm = 0.0f;
    float speed_x = 0.0f, speed_y = 0.0f, speed_z = 0.0f;
    float height = 0.0f, clutch = 0.0f;

    std::array<float, 4> wheels_speeds = {{ 0.0f, 0.0f, 0.0f, 0.0f }};
    std::array<float, 19> sensors;
};

struct CarSteers {
    float gas = 0.0f, hand_brake = 0.0f, steering_wheel = 0.0f;
    int gear = 0;
    float clutch = 0.0f, focus = 0.0f, meta = 0.0f;
};
