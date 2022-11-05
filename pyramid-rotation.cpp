#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
using namespace std;

#include <stdio.h>
//#include <Windows.h>
#include <windows.h>
#include <cwchar>

int screen_width = 240; // Console Screen Size X (columns)
int screen_height = 80; // Console Screen Size Y (rows)

float alpha = 0, beta = 0, gamma = 0;
float view_angle = 60;
float camera_arm = 150;
float dist_to_disp = 50;
float viewer = 10;
float pyramid_face = 20;
float rotation_speed = 1;
int console_color = 10;

float RotCoordX(float x, float y, float z, float alpha, float beta, float gamma)
{
	return y * sin(alpha) * sin(beta) * cos(gamma) - z * cos(alpha) * sin(beta) * cos(gamma) +
		y * cos(alpha) * sin(gamma) + z * sin(alpha) * sin(gamma) + x * cos(beta) * cos(gamma);
}

float RotCoordY(float x, float y, float z, float alpha, float beta, float gamma)
{
	return y * cos(alpha) * cos(gamma) + x * sin(alpha) * cos(gamma) -
		y * sin(alpha) * sin(beta) * sin(gamma) + z * cos(alpha) * sin(beta) * sin(gamma) -
		x * cos(beta) * sin(gamma);
}

float RotCoordZ(float x, float y, float z, float alpha, float beta, float gamma)
{
	return z * cos(alpha) * cos(beta) - y * sin(alpha) * cos(beta) + x * sin(beta);
}

float DisplayCoordX(float x, float y, float z, size_t width, size_t height, float angle, float dist_disp, float cam_arm)
{
	z += (cam_arm - dist_disp) / 2 + dist_disp;
	return (width / height) * x * (1 / tan(angle / 2)) * (1 / z);
}

float DisplayCoordY(float x, float y, float z, size_t width, size_t height, float angle, float dist_disp, float cam_arm)
{
	z += (cam_arm - dist_disp) / 2 + dist_disp;
	return (width / height) * y * (1 / tan(angle / 2)) * (1 / z);
}

float DisplayCoordZ(float x, float y, float z, float dist_disp, float cam_arm)
{
	z += (cam_arm - dist_disp) / 2 + dist_disp;
	return (z - (dist_disp)) * (cam_arm / (cam_arm - dist_disp));
}

template <typename T>
struct Vec3d
{
	Vec3d(T c_x = 0, T c_y = 0, T c_z = 0)
	{
		x = c_x;
		y = c_y;
		z = c_z;
	}

	T x;
	T y;
	T z;

	void Swap(Vec3d& other) noexcept
	{
		Vec3d loc(x, y, z);
		x = other.x;
		y = other.y;
		z = other.z;
		other.x = loc.x;
		other.y = loc.y;
		other.z = loc.z;
	}

	void ScreenNormalize(size_t width, size_t height)
	{
		x = (x + 1) * (T)width * 0.5;
		y = (y + 1) * (T)height * 0.5;
	}

	void DisplayCoordXYZ(size_t width, size_t height, float angle, float dist_disp, float cam_arm)
	{
		x = DisplayCoordX(x, y, z, width, height, angle, dist_disp, cam_arm);
		y = DisplayCoordY(x, y, z, width, height, angle, dist_disp, cam_arm);
		z = DisplayCoordZ(x, y, z, dist_disp, cam_arm);
	}

	void Rotate(float alpha, float beta, float gamma)
	{
		T tmp_x = x;
		T tmp_y = y;
		T tmp_z = z;
		x = RotCoordX(tmp_x, tmp_y, tmp_z, alpha, beta, gamma);
		y = RotCoordY(tmp_x, tmp_y, tmp_z, alpha, beta, gamma);
		z = RotCoordZ(tmp_x, tmp_y, tmp_z, alpha, beta, gamma);
	}

	Vec3d operator +(const Vec3d& other)
	{
		T loc_x, loc_y, loc_z;
		loc_x = x + other.x;
		loc_y = y + other.y;
		loc_z = z + other.z;
		Vec3d loc(loc_x, loc_y, loc_z);

		return loc;
	}

	Vec3d operator +(float f)
	{
		T loc_x, loc_y, loc_z;
		loc_x = x + f;
		loc_y = y + f;
		loc_z = z + f;
		Vec3d loc(loc_x, loc_y, loc_z);

		return loc;
	}

	Vec3d operator -(const Vec3d& other)
	{
		T loc_x, loc_y, loc_z;
		loc_x = x - other.x;
		loc_y = y - other.y;
		loc_z = z - other.z;
		Vec3d loc(loc_x, loc_y, loc_z);

		return loc;
	}

	Vec3d operator -(float f)
	{
		T loc_x, loc_y, loc_z;
		loc_x = x - f;
		loc_y = y - f;
		loc_z = z - f;
		Vec3d loc(loc_x, loc_y, loc_z);

		return loc;
	}

	Vec3d operator *(float f)
	{
		T loc_x, loc_y, loc_z;
		loc_x = x * f;
		loc_y = y * f;
		loc_z = z * f;
		Vec3d loc(loc_x, loc_y, loc_z);

		return loc;
	}
};

template <typename T>
inline void swap(Vec3d<T>& a, Vec3d<T>& b) noexcept { a.Swap(b); }

namespace std
{
	template <typename T>
	void swap(Vec3d<T>& a, Vec3d<T>& b) noexcept { a.Swap(b); }
};

template <typename T>
struct Triangle
{
	Triangle(Vec3d<T> p0, Vec3d<T> p1, Vec3d<T> p2)
	{
		p_zero = p0;
		p_one = p1;
		p_two = p2;
	}

	Vec3d<T> p_zero;
	Vec3d<T> p_one;
	Vec3d<T> p_two;

	void ScreenNormalize(size_t width, size_t height)
	{
		p_zero.ScreenNormalize(width, height);
		p_one.ScreenNormalize(width, height);
		p_two.ScreenNormalize(width, height);
	}

	void ComputeForScreen(size_t width, size_t height, float angle, float dist_disp, float cam_arm)
	{
		p_zero.DisplayCoordXYZ(width, height, angle, dist_disp, cam_arm);
		p_one.DisplayCoordXYZ(width, height, angle, dist_disp, cam_arm);
		p_two.DisplayCoordXYZ(width, height, angle, dist_disp, cam_arm);
	}

	void sortY()
	{
		if (p_zero.y > p_one.y)
		{
			std::swap(p_zero, p_one);
		}
		if (p_zero.y > p_two.y)
		{
			std::swap(p_zero, p_two);
		}
		if (p_one.y > p_two.y)
		{
			std::swap(p_one, p_two);
		}
	}
};

template <typename T>
struct Pyramid
{
	Vec3d<T> p_zero;
	Vec3d<T> p_one;
	Vec3d<T> p_two;
	Vec3d<T> p_three;

	Pyramid(Vec3d<T> zero, Vec3d<T> one, Vec3d<T> two, Vec3d<T> three)
	{
		p_zero = zero;
		p_one = one;
		p_two = two;
		p_three = three;
	}

	void Rotate(float alpha, float beta, float gamma)
	{
		p_zero.Rotate(alpha, beta, gamma);
		p_one.Rotate(alpha, beta, gamma);
		p_two.Rotate(alpha, beta, gamma);
		p_three.Rotate(alpha, beta, gamma);
	}
};

template <typename T>
void TriangleConsole(Triangle<T>& triangle, wchar_t* screen_buffer, size_t width, size_t height, size_t view_angle,
                     short symbol)
{
	//we display object on a 2d XY display
	if (triangle.p_zero.y == triangle.p_one.y && triangle.p_zero.y == triangle.p_two.y) return;


	//normalize for display
	{
		triangle.ComputeForScreen(width, height, view_angle, dist_to_disp, camera_arm);
		triangle.ScreenNormalize(width, height);
	}

	triangle.sortY();
	int total_height = triangle.p_two.y - triangle.p_zero.y;
	for (int i = 0; i < total_height; i++)
	{
		bool second_half = i > triangle.p_one.y - triangle.p_zero.y || triangle.p_one.y == triangle.p_zero.y;
		int segment_height = second_half ? triangle.p_two.y - triangle.p_one.y : triangle.p_one.y - triangle.p_zero.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? triangle.p_one.y - triangle.p_zero.y : 0)) / segment_height;
		// be careful: with above conditions no division by zero here

		Vec3d<T> A = triangle.p_zero + (triangle.p_two - triangle.p_zero) * alpha;
		Vec3d<T> B = second_half
			             ? triangle.p_one + (triangle.p_two - triangle.p_one) * beta
			             : triangle.p_zero + (triangle.p_one - triangle.p_zero) * beta;
		if (A.x > B.x)
		{
			std::swap(A.x, B.x);
			std::swap(A.y, B.y);
		}
		for (int j = A.x; j <= B.x; j++)
		{
			if (((triangle.p_zero.y + i) < height) && ((triangle.p_zero.y + i) >= 0) && (j < width) && (j >= 0))
			{
				screen_buffer[int(triangle.p_zero.y + i) * width + int(j)] = symbol;
				//memset(screen_buffer+ int(triangle.p_zero.y +i) * width+int(A.x), symbol, (B.x - A.x) * sizeof(wchar_t));
			}
		}
	}
}

template <typename T>
void PyramidConsole(Pyramid<T>& pyramid, wchar_t* screen_buffer, size_t width, size_t height, size_t view_angle,
                    short symbol_zero, short symbol_one, short symbol_two, short symbol_three)
{
	Triangle<T> tri_zero(pyramid.p_zero, pyramid.p_one, pyramid.p_two);
	Triangle<T> tri_one(pyramid.p_zero, pyramid.p_one, pyramid.p_three);
	Triangle<T> tri_two(pyramid.p_zero, pyramid.p_three, pyramid.p_two);
	Triangle<T> tri_three(pyramid.p_one, pyramid.p_three, pyramid.p_two);
	TriangleConsole(tri_three, screen_buffer, width, height, view_angle, symbol_zero);
	TriangleConsole(tri_two, screen_buffer, width, height, view_angle, symbol_three);
	TriangleConsole(tri_one, screen_buffer, width, height, view_angle, symbol_two);
	TriangleConsole(tri_zero, screen_buffer, width, height, view_angle, symbol_one);
}


Pyramid<float> pyr(Vec3d<float>(0, 0, -60), Vec3d<float>(60, 60, 10), Vec3d<float>(-60, 30, 10),
                   Vec3d<float>(0, -60, 10));

void ClearScreen(wchar_t* screen, size_t width, size_t height, short symbol)
{
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			// Each Row

			screen[j * width + i] = symbol;
		}
	}
}

void SetConsoleFont(int font_y)
{
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0; // Width of each character in the font
	cfi.dwFontSize.Y = font_y; // Height
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	std::wcscpy(cfi.FaceName, L"Consolas"); // Choose your font
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

void SetConsoleWindowSize(HANDLE hConsole, int width, int height)
{
	COORD coord{};
	coord.X = width;
	coord.Y = height;
	SMALL_RECT rect{};
	rect.Bottom = coord.X - 1;
	rect.Right = coord.Y - 1;
	SetConsoleWindowInfo(hConsole, true, &rect);

	HWND console = GetConsoleWindow();
	COORD window{};
	window.X = GetConsoleFontSize(hConsole, 0).X;
	window.Y = GetConsoleFontSize(hConsole, 0).Y;
	MoveWindow(console, 0, 0, (coord.X + window.X) * window.X, coord.Y * window.Y,TRUE);
}

int main()
{
	SetConsoleFont(8);
	// Create Screen Buffer
	wchar_t* screen = new wchar_t[screen_width * screen_height];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleTextAttribute(hConsole, console_color);
	SetConsoleActiveScreenBuffer(hConsole);
	SetConsoleWindowSize(hConsole, screen_width, screen_height);


	DWORD dwBytesWritten = 0;


	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	while (1)
	{
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		//alpha = -5*fElapsedTime;
		alpha = std::fmod(alpha, 360);
		beta = fElapsedTime;
		beta = std::fmod(beta, 360);
		gamma = 2 * fElapsedTime;
		gamma = std::fmod(gamma, 360);


		pyr.Rotate(alpha, beta, gamma);


		ClearScreen(screen, screen_width, screen_height, ' ');


		// Display Stats
		swprintf_s(screen, 16, L"FPS=%3.2f ",
		           1.0f / fElapsedTime);


		PyramidConsole<float>(pyr, screen, screen_width, screen_height,
		                      view_angle, 0x2591, 0x2588, 0x2593, 0x2592);

		// Display Frame
		screen[screen_width * screen_height - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, screen_width * screen_height, {0, 0}, &dwBytesWritten);
	}

	return 0;
}
