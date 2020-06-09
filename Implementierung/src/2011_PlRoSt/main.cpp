#include "2011_PlRoSt/2011_PlRoSt.h"
#include "2011_PlRoSt/constants.h"
#include "misc.h"
#include <simple2d.h>
#include "shift_register.h"
#include <thread>

using namespace std;

// ========== CONSTANTS ========== //

// window title
const char *TITLE = "2011 Plumbley, Robertson, Stark - Real-Time Visual Beat Tracking Using a Comb Filter Matrix";

// font for text inside the window
const char *FONT = "res/roboto.ttf";

// window size in pixels
const int WIDTH = 1000;
const int HEIGHT = 300;

// colors
struct Color { float r, g, b, a; };
#define COMMA_SPLIT_COLOR(col) col.r, col.g, col.b, col.a
#define COMMA_SPLIT_COLOR_4(col) COMMA_SPLIT_COLOR(col), COMMA_SPLIT_COLOR(col), COMMA_SPLIT_COLOR(col), COMMA_SPLIT_COLOR(col)
#define SET_TEXT_COL(textptr, col) ((textptr)->color.r = col.r, (textptr)->color.g = col.g, (textptr)->color.b = col.b, (textptr)->color.a = col.a)

const Color C_GREEN = Color{ .r = 0, .g = 1, .b = 0, .a = 1 };
const Color C_WHITE = Color{ .r = 1, .g = 1, .b = 1, .a = 1 };
const Color C_GRID_LINES = Color{ .r = 1, .g = 1, .b = 1, .a = 0.25 };

// ========== APP CLASS ========== //
struct Bounds
{
	float left, top, right, bottom;

	float width() const { return right - left; }
	float height() const { return bottom - top; }
	float y_miggle() const { return (bottom + top) / 2; }
};

class MyApp
{
	private:
		// BEAT TRACKING
		_2011_PlRoSt beat_tracking;

		// RENDER DATA
		ShiftRegister input_samples_min;
		ShiftRegister input_samples_max;
		S2D_Text *text_audio_input;

		// GUI
		bool halt;
		S2D_Window *window;
		thread input_thread;

		void render_audio_input(Bounds b, Color c);

		static void render_grid_lines(Bounds b, Color c);

		static void render();

		static void input_thread_main();

	public:
		static void init();

		static void free();

		static void run();
};

MyApp app;

void MyApp::render_audio_input(Bounds b, Color c)
{
	float height = b.height();
	float y_miggle = b.y_miggle();

	int n = this->input_samples_min.get_len();

	// grid lines
	MyApp::render_grid_lines(b, C_GRID_LINES);

	// waveform
	for (int i = n - (int)b.width(); i < n; ++i)
	{
		float x = (float)(i - n) + b.right;
		float y0 = y_miggle + this->input_samples_min[i] * height / 2;
		float y1 = y_miggle + this->input_samples_max[i] * height / 2;

		S2D_DrawLine(
			x, y0, x, y1, 1,
			COMMA_SPLIT_COLOR_4(c)
		);
	}

	// zero line
	S2D_DrawLine(
		b.left, y_miggle, b.right, y_miggle, 1,
		COMMA_SPLIT_COLOR_4(c)
	);

	// text
	SET_TEXT_COL(app.text_audio_input, c);
	app.text_audio_input->x = b.left;
	app.text_audio_input->y = b.top;
	S2D_DrawText(app.text_audio_input);
}

void MyApp::render_grid_lines(Bounds b, Color c)
{
	float spacing = 1.0f / ODF_SAMPLE_INTERVAL;

	for (int i = 0;; ++i)
	{
		float x = b.right - (float)i * spacing;

		if (x < b.left)
		{
			break;
		}

		S2D_DrawLine(
			x, b.top, x, b.bottom, 1,
			COMMA_SPLIT_COLOR_4(c)
		);
	}
}

void MyApp::render()
{
	app.render_audio_input(Bounds {0, 0, WIDTH, HEIGHT}, C_GREEN);
}

void MyApp::input_thread_main()
{
	ShiftRegister input_samples(STFT_HOP_SIZE);
	float sample;

	while (!app.halt && (read(0, &sample, sizeof(sample)) == sizeof(sample)))
	{
		input_samples.push(sample);

		if (app.beat_tracking(sample))
		{
			float samples[input_samples.get_len()];
			input_samples.get_content(samples);

			app.input_samples_max.push(max(samples, input_samples.get_len()));
			app.input_samples_min.push(min(samples, input_samples.get_len()));
		}
	}
}

void MyApp::init()
{
	app.beat_tracking = _2011_PlRoSt();
	app.input_samples_min = ShiftRegister(WIDTH);
	app.input_samples_max = ShiftRegister(WIDTH);
	app.text_audio_input = S2D_CreateText(
		FONT, "Audio Input", 20
	);
	app.halt = false;
	app.window = S2D_CreateWindow(
		TITLE, WIDTH, HEIGHT, nullptr, MyApp::render, 0
	);
	app.input_thread = thread(MyApp::input_thread_main);
}

void MyApp::free()
{
	app.halt = true;
	app.input_thread.join();

	S2D_FreeText(app.text_audio_input);
	S2D_FreeWindow(app.window);
}

void MyApp::run()
{
	S2D_Show(app.window);
}


// ========== FUNCTIONS ========== //

int main()
{
	MyApp::init();
	MyApp::run();
	MyApp::free();
}