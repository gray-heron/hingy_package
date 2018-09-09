#include <climits>
#include <thread>
#include <unordered_map>

#include "main.h"
#include "torcs_integration.h"

using namespace std::chrono_literals;
using std::string;

// clang-format off
const std::unordered_map<string, std::pair<size_t, int>> car_state_offset_table =
{
    { "angle",             { offsetof(CarState, angle),             1  } },
    { "curLapTime",        { offsetof(CarState, current_lap_time),  1  } },
    { "distFromStart",     { offsetof(CarState, absolute_odometer), 1  } },
    { "rpm",               { offsetof(CarState, rpm),               1  } },
    { "speedX",            { offsetof(CarState, speed_x),           1  } },
    { "speedY",            { offsetof(CarState, speed_y),           1  } },
    { "speedZ",            { offsetof(CarState, speed_z),           1  } },
    { "track",             { offsetof(CarState, sensors),           19 } },
    { "trackPos",          { offsetof(CarState, cross_position),    1  } },
    { "wheelSpinVel",      { offsetof(CarState, wheels_speeds),     4  } },
    { "z",                 { offsetof(CarState, height),            1  } },
    { "gear",              { offsetof(CarState, gear),              1  } },
};
// clang-format on

TorcsIntegration::TorcsIntegration(stringmap params)
{
    int port;

    if (params.find("port") != params.end())
        port = std::stoi(params["port"]);
    else
        assert(false);

    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), params["host"], "87623");
    server_endpoint = *resolver.resolve(query);

    assert(server_endpoint.address().to_string() != "");

    socket = std::make_unique<udp::socket>(io_service,
                                           udp::endpoint(udp::v4(), port));

    socket->non_blocking(true);
}

CarState TorcsIntegration::ParseCarState(std::string in)
{
    CarState out;

    char *cursor = (char *)in.c_str();
    string param_name;

    if (in == "***shutdown***")
    {
        log_info("Shutdown command received. Bye, bye.");
        exit(0);
    }

    while (*cursor != '\0')
    {
        if (*(cursor++) != '(')
        {
            log_error("Unimplemented case!");
            throw;
        }

        param_name = ParseString(&cursor);
        auto offset = car_state_offset_table.find(param_name);

        if (offset != car_state_offset_table.end())
        {
            for (int i = 0; i < offset->second.second; i++)
            {
                *((float *) // behold!
                  ((uint8_t *)&out + offset->second.first +
                   (i * sizeof(float)))) = strtof(cursor, &cursor);
            }

            if (*(cursor++) != ')')
                throw;
        }
        else
        {
            while (*(cursor++) != ')')
                ;
        }
    }

    return out;
}

string TorcsIntegration::ParseString(char **cursor)
{
    string out;
    char *tmp_cursor = *cursor;

    while (*(tmp_cursor++) != ' ')
        out += *(tmp_cursor - 1);

    *cursor = tmp_cursor;

    return out;
}

CarState TorcsIntegration::Begin(stringmap params)
{
    string init_string = "SCR(init", instring;
    string in_msg;

    for (int i = -9; i <= 9; i++)
    {
        init_string += string(" ") + params[string("ds") + std::to_string(i)];
    }
    init_string += ")";

    while (true)
    {
        do
        {
            Send(init_string);
            std::this_thread::sleep_for(1s);
        } while ((in_msg = Receive()).length() == 0);

        if (in_msg == "***identified***")
        {
            break;
        }
        else
        {
            log_warning("Communication error on init!");
            throw;
        }
    }

    while ((in_msg = Receive()).length() == 0)
        ;

    if (in_msg[0] == '*' && in_msg[1] == '*' && in_msg[2] == '*')
    {
        log_error("Unimplemented case!");
        throw;
    }

    return ParseCarState(in_msg);
}

CarState TorcsIntegration::Cycle(const CarSteers &steers)
{
    string out, in;

    out += "(accel " + std::to_string(steers.gas) + ")";
    out += "(brake " + std::to_string(steers.hand_brake) + ")";
    out += "(gear " + std::to_string(steers.gear) + ")";
    out += "(clutch " + std::to_string(steers.clutch) + ")";
    out += "(steer " + std::to_string(steers.steering_wheel) + ")";
    // out += "(focus " + std::to_string(steers.focus) + ")";
    // out += "(meta " + std::to_string(steers.gas) + ")";

    while ((in = Receive()).length() == 0)
        ;

    auto state = ParseCarState(in);
    Send(out);

    return state;
}

std::string TorcsIntegration::Receive()
{
    boost::system::error_code ec;

    boost::array<char, UINT16_MAX> recv_buf;
    size_t len = socket->receive_from(boost::asio::buffer(recv_buf),
                                      server_endpoint, 0, ec);

    if (ec == boost::asio::error::would_block)
        return "";
    else
        return std::string(recv_buf.data(), len);
}

void TorcsIntegration::Send(std::string msg)
{
    boost::system::error_code ignored_error;
    std::vector<char> send_buf(msg.begin(), msg.end());
    socket->send_to(boost::asio::buffer(send_buf), server_endpoint, 0,
                    ignored_error);
}

TorcsIntegration::~TorcsIntegration() {}
