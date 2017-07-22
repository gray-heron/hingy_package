#pragma once

#include <string>
#include <vector>
#include <tuple>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include "utils.h"
#include "hingy_math.h"

#define THREADS_COUNT 4

class HingyTrack {
public:
    struct Waypoint {
        float f, a, l, r;
    };

protected:

    struct Hinge {
        float a, b, x, hx, lx, forward;
        float y; //y might be garbage, use ToWaypoint()
        float desired_speed = 55.0f;
        float curve;
        int direction;
        Direction true_heading;

        Vector2D ToWaypoint() const;
        void ClapToAxis();
    };

    std::vector<Waypoint> waypoints;
    std::vector<std::pair<Vector2D, Vector2D>> bounds;
    std::vector<Hinge> hinges;
    std::string filename;

    float angle_factor = 0.1965f;
    float bound_factor = 0.75;
    float forward_factor = 0.40f;

    float hinge_sep = 1.0f;
    float sep_dist;
    float interhinge_pos;
    bool recording = false;

    float last_forward = 0.0f;
    bool fuse, fuse2;

    int current_hinge = 0;

public:
    virtual ~HingyTrack() {};
    HingyTrack(std::string filename);

    float fshift = 38.0f;

    bool Recording();
    virtual void BeginRecording();
    virtual void StopRecording();
    virtual void MarkWaypoint(float forward, float l, float r, float angle, float speed);
    virtual void ConstructBounds();
    virtual void ConstructHinges(float skip);
    virtual void SimulateHinges(float straightening_factor, float pulling_factor);
    virtual std::pair<float, float> GetHingePosAndHeading();
    virtual float GetHingeSpeed();
    virtual void ConstructSpeeds(float s, float p, float c);

    void CacheHinges();
    void LoadHingesFromCache();
};

class HingyTrackGui : public HingyTrack {
    int rx, ry;

    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *bitmapTex = NULL;
    SDL_Surface *bitmapSurface = NULL;

public:
    void DrawTrack();
    void DrawBounds();
    void DrawHinges();
    void DrawMiddle();
    void TickGraphics(bool clear = true);

    virtual ~HingyTrackGui();
    HingyTrackGui(std::string filename, int resx, int resy);

    virtual void MarkWaypoint(float forward, float l, float r, float angle, float speed);
    void KillGui();
};
