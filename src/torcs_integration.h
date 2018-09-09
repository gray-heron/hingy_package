#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "car_io.h"
#include "main.h"

class SimIntegration
{
  public:
    virtual CarState Begin(stringmap driver_params) = 0;
    virtual CarState Cycle(const CarSteers &) = 0;

    virtual ~SimIntegration() = default;
};

class TorcsIntegration : public SimIntegration
{
    using udp = boost::asio::ip::udp;
    udp::endpoint server_endpoint;
    boost::asio::io_service io_service;
    std::unique_ptr<udp::socket> socket;

    CarState ParseCarState(std::string in);
    std::string ParseString(char **cursor);

    void Send(std::string msg);
    std::string Receive();

  public:
    virtual CarState Cycle(const CarSteers &) override;
    virtual CarState Begin(stringmap driver_params) override;

    TorcsIntegration(stringmap params);
    virtual ~TorcsIntegration();
};
