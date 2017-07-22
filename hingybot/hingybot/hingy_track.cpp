#include <fstream>
#include <assert.h>
#include <algorithm>
#include <cfloat>

#include "hingy_track.h"
#include "utils.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

#define GUI_SKIP 50

using std::string;
using namespace rapidxml;

inline float sgn(float& a1) {
	if (a1 > 0) {
		return 1.0;
	}

	return -1.0f;
}

HingyTrack::HingyTrack(string filename) : filename(filename)
{
	if (file_exists(filename)) {
		int size = file_size(filename);
		char* buf = new char[size + 1];
		xml_document <> doc;
		FILE *f = fopen(filename.c_str(), "rb");

		while (ftell(f) != size) {
			fread(&buf[ftell(f)], size, 1, f);
		}

		fclose(f);
		buf[size] = '\0';

		doc.parse<0>(buf);
		auto track_node = doc.first_node();
		assert(strcmp(track_node->name(), "track") == 0);
		auto waypoint_node = track_node->first_node();

		while (waypoint_node != nullptr) {
			HingyTrack::Waypoint waypoint;

			assert(strcmp(waypoint_node->name(), "waypoint") == 0);
			assert(strcmp(waypoint_node->first_node()->name(), "forward") == 0);
			assert(strcmp(waypoint_node->first_node()->next_sibling()->name(), "left") == 0);
			assert(strcmp(waypoint_node->last_node()->previous_sibling()->name(), "right") == 0);
			assert(strcmp(waypoint_node->last_node()->name(), "angle") == 0);

			waypoint.f = atof(waypoint_node->first_node()->value());
			waypoint.l = atof(waypoint_node->first_node()->next_sibling()->value());
			waypoint.r = atof(waypoint_node->last_node()->previous_sibling()->value());
			waypoint.a = atof(waypoint_node->last_node()->value());

			waypoints.push_back(waypoint);
			waypoint_node = waypoint_node->next_sibling();
		}

		delete[] buf;
	}

	waypoints.reserve(1000000);
}

void HingyTrack::BeginRecording()
{
	waypoints.clear();

	recording = true;
	fuse = true;
	fuse2 = false;
}

void HingyTrack::StopRecording()
{
	std::fstream fs;
	fs.open(filename.c_str(), std::fstream::out | std::fstream::trunc);
	std::vector<char*> to_be_deallocated;

	xml_document <> doc;
	xml_node <> * doc_track = doc.allocate_node(node_element, "track");

	for (auto & waypoint : waypoints) {
		char *fb = new char[62]; sprintf(fb, "%f", waypoint.f);
		char *lb = new char[62]; sprintf(lb, "%f", waypoint.l);
		char *rb = new char[62]; sprintf(rb, "%f", waypoint.r);
		char *ab = new char[62]; sprintf(ab, "%f", waypoint.a);
		to_be_deallocated.insert(to_be_deallocated.end(), { fb, lb, rb, ab });

		xml_node <> * waypoint_node = doc.allocate_node(node_element, "waypoint");
		waypoint_node->append_node(doc.allocate_node(node_element, "forward", fb));
		waypoint_node->append_node(doc.allocate_node(node_element, "left", lb));
		waypoint_node->append_node(doc.allocate_node(node_element, "right", rb));
		waypoint_node->append_node(doc.allocate_node(node_element, "angle", ab));
		doc_track->append_node(std::move(waypoint_node));
	}
	
	doc.append_node(doc_track);
	fs << doc;
	fs.close();

	for (auto str : to_be_deallocated)
		delete[] str;

	recording = false;
}

void HingyTrack::CacheHinges()
{
	FILE * f = fopen(((string)"tmp/" + (string)filename + (string)".hinges").c_str(), "wb");
	if (f) {
		assert(fwrite(hinges.data(), sizeof(Hinge), hinges.size(), f)
			== hinges.size());
		fclose(f);
	}
}

void HingyTrack::LoadHingesFromCache()
{
	FILE * f = fopen(((string)"tmp/" + (string)filename + (string)".hinges").c_str(), "rb");
	assert(fread(hinges.data(), sizeof(Hinge), hinges.size(), f) == hinges.size());
	fclose(f);
}

void HingyTrack::MarkWaypoint(float forward, float l, float r, float angle, float speed)
{
	if (hinges.size() > 0) {
		forward -= fshift; //40.4f
		current_hinge = 0;

		while (current_hinge < hinges.size() && forward > hinges[current_hinge].forward)
			current_hinge = current_hinge + 1;
		current_hinge = (current_hinge + hinges.size() - 1) % hinges.size();

		interhinge_pos = forward - hinges[current_hinge].forward;
		interhinge_pos /= hinges[(current_hinge + 1) % hinges.size()].forward -
			hinges[current_hinge].forward;
	}

	if (recording) {
		if (last_forward - forward > 60.0f && !fuse) {
			last_forward = forward;
			fuse2 = true;
			return;
		}

		if (!fuse) {
			if (forward > 50.0f && fuse2) {
				StopRecording();
				return;
			}
			waypoints.push_back(std::move(HingyTrack::Waypoint{
				forward - last_forward, angle, l, r }));
		}
		else if (forward > 50.0f && forward < 60.0f) {
			fuse = false;
		}
	}


	last_forward = forward;
}

void HingyTrack::ConstructBounds()
{
	float heading = HALF_PI / 2.0f;
	float x = 0, y = 0;

	for (const auto& waypoint : waypoints) {
		float lx = x + std::cos(heading + M_PI / 2.0f) * /*waypoint.l * */bound_factor;
		float ly = y + std::sin(heading + M_PI / 2.0f) * /*waypoint.l * */bound_factor;

		float rx = x + std::cos(heading - M_PI / 2.0f) * /*waypoint.r * */bound_factor;
		float ry = y + std::sin(heading - M_PI / 2.0f) * /*waypoint.r * */bound_factor;

		bounds.push_back(std::pair<Vector2D, Vector2D>(
			Vector2D{ lx, ly }, Vector2D{ rx, ry }
		));

		x += std::cos(heading) * forward_factor * waypoint.f;
		y += std::sin(heading) * forward_factor * waypoint.f;
		heading += waypoint.a * angle_factor;
	}
}

void HingyTrack::ConstructHinges(float skip)
{
	hinge_sep = skip;
	hinges.clear();
	float fuse = FLT_MAX, forward_sum = 0.0f;
	int i = 0;

	for (auto bound = bounds.begin(); bound != bounds.end(); ++bound) {
		if (waypoints[i].f + fuse > skip) {
			Hinge h;

			if (bound->first.x == bound->second.x)
				bound->first.x += 0.01f;

			h.x = (bound->first.x + bound->second.x) / 2.0f;
			h.a = bound->first.y - bound->second.y;
			h.a /= bound->first.x - bound->second.x;
			h.b = bound->first.y - h.a * bound->first.x;
			h.y = h.x * h.a + h.b;
			h.direction = bound->first.x > bound->second.x;

			h.hx = std::max(bound->first.x, bound->second.x);
			h.lx = std::min(bound->first.x, bound->second.x);

			h.forward = forward_sum;

			hinges.push_back(std::move(h));
			fuse = 0.0f;
		}
		else {
			fuse += waypoints[i].f;
		}

		forward_sum += waypoints[i].f;
		i++;
	}

	for (int i = 0; i < hinges.size(); i++) {
		auto in = (i + 1) % hinges.size();

		Hinge& me = hinges[i];
		const Hinge& next = hinges[in];

		me.true_heading.h = std::atan2(next.ToWaypoint().y - me.ToWaypoint().y,
			next.x - me.x);
	}
}

void HingyTrack::SimulateHinges(float straightening_factor, float pulling_factor)
{
	auto forces = std::vector<Vector2D>(hinges.size());

	for (int i = 0; i < hinges.size(); i++) {
		auto force = Vector2D(0.0f, 0.0f);
		auto ip = (i - 1 + hinges.size()) % hinges.size();
		auto in = (i + 1) % hinges.size();

		const Hinge& prev = hinges[ip];
		Hinge& me = hinges[i];
		const Hinge& next = hinges[in];

		auto angle_to_next = (Vector2D(next.x, next.y) - Vector2D(me.x, me.y)).ToDirection();
		auto angle_from_prev = (Vector2D(me.x, me.y) - Vector2D(prev.x, prev.y)).ToDirection();
		auto angle_diff = angle_to_next - angle_from_prev;
		auto angle_diff2 = (angle_to_next - angle_from_prev.Inv());
		auto perpendicular_angle = angle_from_prev.Inv() + angle_diff2 / 2.0f;
		auto direction = std::abs((Vector2D(next.x, next.y) -
			Vector2D(prev.x, prev.y)).ToDirection().h);

		auto dist_to_prev = (Vector2D(prev.x, prev.y) - Vector2D(me.x, me.y)).Length();
		auto dist_to_next = (Vector2D(next.x, next.y) - Vector2D(me.x, me.y)).Length();

		forces[i] += Vector2D(straightening_factor * 2.0f, 0.0f) * (perpendicular_angle)*
			std::abs(std::pow(angle_diff, 2.0f));
		forces[ip] += Vector2D(straightening_factor, 0.0f) * (perpendicular_angle.Inv()) *
			std::abs(std::pow(angle_diff, 2.0f));
		forces[in] += Vector2D(straightening_factor, 0.0f) * (perpendicular_angle.Inv()) *
			std::abs(std::pow(angle_diff, 2.0f));

		me.curve = std::abs(angle_diff);

		forces[i] += Vector2D(pulling_factor, 0.0f) * angle_to_next;
		forces[in] += Vector2D(-pulling_factor, 0.0f) * angle_to_next;

		forces[i] += Vector2D(-pulling_factor, 0.0f) * angle_from_prev;
		forces[ip] += Vector2D(pulling_factor, 0.0f) * angle_from_prev;
	}

	hinges.begin()->x = (hinges.begin()->lx + hinges.begin()->hx) / 2.0f;
	(hinges.end() - 1)->x = ((hinges.end() - 1)->lx + (hinges.end() - 1)->hx) / 2.0f;

	for (int i = 0; i < hinges.size(); i++) {
		hinges[i].x += forces[i].x;
		hinges[i].y += forces[i].y;
		hinges[i].ClapToAxis();
	}
}

std::pair<float, float> HingyTrack::GetHingePosAndHeading()
{
	if (hinges.size() == 0)
		return std::pair<float, float>(0.0f, 0.0f);

	const auto& hp = hinges[current_hinge % hinges.size()];
	const auto& hn = hinges[(current_hinge + 1) % hinges.size()];
	const auto& hnn = hinges[(current_hinge + 2) % hinges.size()];

	float op = ((hp.x - hp.lx) / (hp.hx - hp.lx) - 0.5f) * 2.0f;
	float on = ((hn.x - hn.lx) / (hn.hx - hn.lx) - 0.5f) * 2.0f;
	//float on = ((hnn.x - hnn.lx) / (hnn.hx - hnn.lx) - 0.5f) * 2.0f;

	if (!hp.direction)
		op *= -1;
	if (!hn.direction)
		on *= -1;

	float out_pos = op * (1.0f - interhinge_pos) + on * interhinge_pos;
	//float out_h = std::atan(hp.a * (1.0f - p) + hn.a * p);

	auto hinge_dir1 = Vector2D(hn.x - hp.x, hn.ToWaypoint().y - hp.ToWaypoint().y).
		ToDirection();

	float out_h1 = hinge_dir1 - hn.true_heading;

	auto hinge_dir2 = Vector2D(hnn.x - hn.x, hnn.ToWaypoint().y - hn.ToWaypoint().y).
		ToDirection();

	float out_h2 = hinge_dir2 - (hnn.true_heading);

	float out_h = out_h1 * (1.0f - interhinge_pos) + interhinge_pos * out_h2;

	//printf("%f \t %f\t %f\n", interhinge_pos, out_pos, out_h);
	return std::pair<float, float>(out_pos * 0.76f, out_h * -0.5f);
}

float HingyTrack::GetHingeSpeed()
{
	if (hinges.size() == 0)
		return 0.1f;

	return hinges[current_hinge].desired_speed;
}

bool HingyTrack::Recording()
{
	return recording;
}

void HingyTrack::ConstructSpeeds(float s, float p, float c)
{
	float energy = 1.0f;
	int counter = hinges.size();

	if (hinges.size() == 0)
		return;

	for (auto i = hinges.rbegin(); i != hinges.rend(); i++) {
		energy -= (s * std::pow(i->curve, 2.0f) + p) * i->curve - c;

		if (energy > 1.0f)
			energy = 1.0f;
		else if (energy < 0.0f)
			energy = 0.0f;

		if (energy > 0.70f && i->curve > 0.075f)
			energy = 0.7f;

		i->desired_speed = energy;
		counter -= 1;
	}
}

// === GUI ===


HingyTrackGui::~HingyTrackGui()
{
	KillGui();
}

HingyTrackGui::HingyTrackGui(string filename, int resx, int resy) :
	HingyTrack(filename), rx(resx), ry(resy)
{
	SDL_Init(SDL_INIT_VIDEO);

	win = SDL_CreateWindow("HingyBot", 100, 100, resx, resy, 0);
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	SDL_ShowWindow(win);
}

void HingyTrackGui::MarkWaypoint(float forward, float l, float r, float angle, float speed)
{
	TickGraphics();
	HingyTrack::MarkWaypoint(forward, l, r, angle, speed);
}

void HingyTrackGui::DrawTrack()
{
	SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);

	float heading = HALF_PI / 2.0f;
	float lx = rx / 2, ly = ry / 2;

	int i = 0;

	for (const auto& waypoint : waypoints) {

		float nx = lx + std::cos(heading) * forward_factor * waypoint.f;
		float ny = ly + std::sin(heading) * forward_factor * waypoint.f;

		if (i++ % GUI_SKIP == 0)
			SDL_RenderDrawLine(renderer, lx, ly, nx, ny);

		lx = nx; ly = ny;
		heading += waypoint.a * angle_factor;
	}
}

void HingyTrackGui::DrawBounds()
{
	int i = 0;
	SDL_SetRenderDrawColor(renderer, 160, 0, 0, 255);

	if (bounds.size() == 0)
		return;

	auto& last_bound = bounds[0];

	for (const auto& bound : bounds) {
		if (i++ % GUI_SKIP)
			continue;

		SDL_RenderDrawLine(renderer,
			bound.first.x + rx / 2, bound.first.y + ry / 2,
			last_bound.first.x + rx / 2, last_bound.first.y + ry / 2
		);

		SDL_RenderDrawLine(renderer,
			bound.second.x + rx / 2, bound.second.y + ry / 2,
			last_bound.second.x + rx / 2, last_bound.second.y + ry / 2
		);

		last_bound = bound;
	}
}

void HingyTrackGui::DrawHinges()
{
	SDL_SetRenderDrawColor(renderer, 0, 160, 0, 255);

	if (hinges.size() == 0)
		return;

	auto last_hinge = hinges[hinges.size() - 1];

	for (const auto& hinge : hinges) {
		auto my_pos = hinge.ToWaypoint();
		auto last_pos = last_hinge.ToWaypoint();

		SDL_SetRenderDrawColor(renderer, 0, hinge.desired_speed * 200.0f + 50.0f, 0, 255);
		//SDL_SetRenderDrawColor(renderer, 0, 255.0f, 0, 255);

		SDL_RenderDrawLine(renderer,
			my_pos.x + rx / 2, my_pos.y + ry / 2,
			last_pos.x + rx / 2, last_pos.y + ry / 2
		);

		last_hinge = hinge;
	}

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderDrawLine(renderer,
		hinges[current_hinge % hinges.size()].x + rx / 2,
		hinges[current_hinge % hinges.size()].y + ry / 2,
		hinges[(current_hinge + 1) % hinges.size()].x + rx / 2,
		hinges[(current_hinge + 1) % hinges.size()].y + ry / 2
	);
}

void HingyTrackGui::DrawMiddle()
{
	SDL_SetRenderDrawColor(renderer, 12, 12, 12, 255);

	SDL_RenderDrawLine(renderer, 0, 0, rx, ry);
	SDL_RenderDrawLine(renderer, 0, ry, rx, 0);
	SDL_RenderDrawLine(renderer, rx / 2.0f, 0, rx / 2.0f, ry);
	SDL_RenderDrawLine(renderer, 0, ry / 2.0f, rx, ry / 2.0f);
}

void HingyTrackGui::TickGraphics(bool clear)
{
	bool killsdl = false;
	if (renderer == nullptr)
		return;

	SDL_Event e;
	if (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			killsdl = true;
		}
	}

	if (clear) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
	}

	//DrawMiddle();
	if (recording)
		DrawTrack();
	//DrawBounds();

	DrawHinges();

	if (!killsdl) {
		SDL_RenderCopy(renderer, bitmapTex, NULL, NULL);
		SDL_RenderPresent(renderer);
	}
	else {
		KillGui();
	}
}

void HingyTrackGui::KillGui()
{
	if (renderer == nullptr)
		return;

	SDL_DestroyTexture(bitmapTex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);

	SDL_Quit();
	renderer = nullptr;
	StopRecording();
}

Vector2D HingyTrack::Hinge::ToWaypoint() const
{
	return Vector2D{ x, y };
}

void HingyTrack::Hinge::ClapToAxis()
{
	float pa = -1.0f / a;
	float pb = y - (pa * x);

	x = -(pb - b) / (pa - a);

	if (x > hx)
		x = hx;
	else if (x < lx)
		x = lx;

	y = a * x + b;
}
