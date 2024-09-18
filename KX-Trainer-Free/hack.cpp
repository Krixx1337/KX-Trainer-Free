#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <windows.h>
#include "hack.h"

// Helper function to sleep
void sleep_for_seconds(int seconds) {
	std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

// Helper function to set console color
void Hack::set_color(int color) {
	SetConsoleTextAttribute(hConsole, color);
}

// Helper function to reduce duplication
uintptr_t Hack::refresh_addr(std::vector<unsigned int> offsets)
{
	return FindDMAAddy(hProcess, dynamicPtrBaseAddr, offsets);
}

void Hack::find_process() {
	if (hProcess == NULL) {
		int exit_time = 5;
		while (exit_time > 0 && hProcess == NULL) {
			std::cout << "Gw2-64.exe process not found!" << std::endl;
			std::cout << "Exiting in " << exit_time << " seconds..." << std::endl;
			sleep_for_seconds(1);
			exit_time--;
			system("cls");
			processID = GetProcID(L"Gw2-64.exe");
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processID);
		}
		if (exit_time == 0) {
			exit(0);
		}
	}
}

void Hack::base_scan() {
	int scans = 0;
	unsigned int base_value = 0;
	while (dynamicPtrBaseAddr == NULL || base_value <= 10000) {
		baseAddress = PatternScanExModule(hProcess, gw2_name, gw2_name,
			BASE_SCAN_PATTERN, BASE_SCAN_MASK);
		if (baseAddress != nullptr) {
			uintptr_t addrWithOffset = reinterpret_cast<uintptr_t>(baseAddress) - 0x8;
			baseAddress = reinterpret_cast<void*>(addrWithOffset);
		}

		dynamicPtrBaseAddr = reinterpret_cast<std::uintptr_t>(baseAddress);
		if (dynamicPtrBaseAddr == NULL) {
			system("cls");
			scans++;
			std::cout << "Gw2-64.exe process found!" << std::endl;
			std::cout << "Scanning for Base Address... " << scans << std::endl;
		}
		else {
			system("cls");
			ReadProcessMemory(hProcess, (BYTE*)baseAddress, &base_value, sizeof(base_value), nullptr);
			std::cout << "Gw2-64.exe process found!" << std::endl;
			std::cout << "Waiting for character selection..." << std::endl;
		}
		sleep_for_seconds(1);
	}

	fogAddress = PatternScanExModule(hProcess, gw2_name, gw2_name, FOG_PATTERN, FOG_MASK);
	objectClippingAddress = PatternScanExModule(hProcess, gw2_name, gw2_name, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK);
	betterMovementAddress = PatternScanExModule(hProcess, gw2_name, gw2_name, BETTER_MOVEMENT_PATTERN, BETTER_MOVEMENT_MASK);
	system("cls");
}


void Hack::refresh_address() {
	x_addr = refresh_addr(x_offsets);
	y_addr = refresh_addr(y_offsets);
	z_addr = refresh_addr(z_offsets);
	zheight1_addr = refresh_addr(zheight1_offsets);
	zheight2_addr = refresh_addr(zheight2_offsets);
	gravity_addr = refresh_addr(gravity_offsets);
	speed_addr = refresh_addr(speed_offsets);
	wallclimb_addr = refresh_addr(wallclimb_offsets);
}

void Hack::refresh_xyz() {
	x_addr = refresh_addr(x_offsets);
	y_addr = refresh_addr(y_offsets);
	z_addr = refresh_addr(z_offsets);
}

void Hack::read_xyz() {
	ReadProcessMemory(hProcess, (BYTE*)x_addr, &x_value, sizeof(x_value), nullptr);
	ReadProcessMemory(hProcess, (BYTE*)y_addr, &y_value, sizeof(y_value), nullptr);
	ReadProcessMemory(hProcess, (BYTE*)z_addr, &z_value, sizeof(z_value), nullptr);
}

void Hack::write_xyz() {
	WriteProcessMemory(hProcess, (BYTE*)x_addr, &x_value, sizeof(x_value), nullptr);
	WriteProcessMemory(hProcess, (BYTE*)y_addr, &y_value, sizeof(y_value), nullptr);
	WriteProcessMemory(hProcess, (BYTE*)z_addr, &z_value, sizeof(z_value), nullptr);
}

void Hack::info() {
	std::cout << "[Hotkeys]\n"
		<< "NUMPAD1 - Save Position\n"
		<< "NUMPAD2 - Load Position\n"
		<< "NUMPAD3 - Invisibility (for mobs)\n"
		<< "NUMPAD4 - Wall Climb\n"
		<< "NUMPAD5 - Clipping\n"
		<< "NUMPAD6 - Object Clipping\n"
		<< "NUMPAD7 - Better Movement\n"
		<< "NUMPAD+ - Super Sprint (hold)\n"
		<< "CTRL - Fly (hold)\n"
		<< "SHIFT - Sprint\n"
		<< "[Logs]\n";
}

void Hack::start() {
	std::cout << "[";
	set_color(BLUE);
	std::cout << "KX Trainer by Krixx";
	set_color(DEFAULT);
	std::cout << "]";

	std::cout << "\nIf you like the trainer, consider upgrading to the paid version at ";
	set_color(BLUE);
	std::cout << "kxtools.xyz";
	set_color(DEFAULT);
	std::cout << " for more features and support!" << std::endl;

	std::cout << "\nGw2-64.exe process found!\n" << std::endl;

	info();
}


void Hack::hacks_loop()
{
	while (1)
	{
		refresh_address();
		//hacks_fog();
		hacks_object_clipping();
		hacks_better_movement();
		hacks_sprint();
		hacks_save_position();
		hacks_load_position();
		hacks_super_sprint();
		hacks_invisibility();
		hacks_wall_climb();
		hacks_clipping();
		hacks_fly();

		Sleep(1);
	}
}

void Hack::hacks_fog()
{
	bool fog_toggle = false;
	if (GetAsyncKeyState(key_fog) & 1)
	{
		fog_toggle = !fog_toggle;
	}
	if (fog_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)fogAddress, &fog, sizeof(fog), nullptr);
		if (fog == 1)
		{
			fog = 0;
			WriteProcessMemory(hProcess, (BYTE*)fogAddress, &fog, sizeof(fog), nullptr);
			set_color(RED);
			std::cout << "\nFog: Off" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			fog = 1;
			WriteProcessMemory(hProcess, (BYTE*)fogAddress, &fog, sizeof(fog), nullptr);
			set_color(GREEN);
			std::cout << "\nFog: On" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_object_clipping()
{
	bool object_clipping_toggle = false;
	if (GetAsyncKeyState(key_object_clipping) & 1)
	{
		object_clipping_toggle = !object_clipping_toggle;
	}
	if (object_clipping_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)objectClippingAddress, &object_clipping, sizeof(object_clipping), nullptr);
		if (object_clipping == 4051)
		{
			object_clipping = 4059;
			WriteProcessMemory(hProcess, (BYTE*)objectClippingAddress, &object_clipping, sizeof(object_clipping), nullptr);
			set_color(GREEN);
			std::cout << "\nObject Clipping: On" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			object_clipping = 4051;
			WriteProcessMemory(hProcess, (BYTE*)objectClippingAddress, &object_clipping, sizeof(object_clipping), nullptr);
			set_color(RED);
			std::cout << "\nObject Clipping: Off" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_better_movement()
{
	bool movement_toggle = false;
	if (GetAsyncKeyState(key_better_movement) & 1)
	{
		movement_toggle = !movement_toggle;
	}
	if (movement_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)betterMovementAddress, &better_movement, sizeof(better_movement), nullptr);
		if (better_movement == 6003)
		{
			better_movement = 6005;
			WriteProcessMemory(hProcess, (BYTE*)betterMovementAddress, &better_movement, sizeof(better_movement), nullptr);
			set_color(GREEN);
			std::cout << "\nBetter Movement: On" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			better_movement = 6003;
			WriteProcessMemory(hProcess, (BYTE*)betterMovementAddress, &better_movement, sizeof(better_movement), nullptr);
			set_color(RED);
			std::cout << "\nBetter Movement: Off" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_sprint()
{
	//Freeze Speed
	if (speed_freeze == 1) {
		ReadProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
		if (speed < sprint_settings)
		{
			speed = sprint_settings;
			WriteProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
		}
	}

	//Sprint
	bool speed_toggle = false;
	if (GetAsyncKeyState(key_sprint) & 1)
	{
		speed_toggle = !speed_toggle;
	}
	if (speed_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
		if (speed < sprint_settings)
		{
			speed_freeze = 1;
			set_color(GREEN);
			std::cout << "\nOn: Sprint" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			speed_freeze = 0;
			speed = 9.1875;
			WriteProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
			set_color(RED);
			std::cout << "\nOff: Sprint" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_super_sprint()
{
	//Sprint turbo
	bool turbo_toggle = false;
	if (GetAsyncKeyState(key_super_sprint))
	{
		turbo_toggle = !turbo_toggle;
	}
	if (turbo_toggle)
	{
		if (turbo_check == false)
		{
			ReadProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
			turbo_speed = speed;
			set_color(GREEN);
			std::cout << "\nOn: Super Sprint" << std::endl;
			set_color(DEFAULT);
		}
		speed = 30;
		WriteProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
		turbo_check = true;
	}
	else
	{
		if (turbo_check == true)
		{
			//Write previous speed
			speed = turbo_speed;
			WriteProcessMemory(hProcess, (BYTE*)speed_addr, &speed, sizeof(speed), nullptr);
			turbo_check = false;
			set_color(RED);
			std::cout << "\nOff: Super Sprint" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_save_position()
{
	//Save Position
	bool save_pos = false;
	if (GetAsyncKeyState(key_savepos) & 1)
	{
		save_pos = !save_pos;
	}
	if (save_pos)
	{
		ReadProcessMemory(hProcess, (BYTE*)x_addr, &x_save, sizeof(x_save), nullptr);
		ReadProcessMemory(hProcess, (BYTE*)y_addr, &y_save, sizeof(y_save), nullptr);
		ReadProcessMemory(hProcess, (BYTE*)z_addr, &z_save, sizeof(z_save), nullptr);
		set_color(BLUE);
		std::cout << "\nSaved Position" << std::endl;
		set_color(DEFAULT);
	}
}

void Hack::hacks_load_position()
{
	//Load Position
	bool load_pos = false;
	if (GetAsyncKeyState(key_loadpos) & 1)
	{
		load_pos = !load_pos;
	}
	if (load_pos)
	{
		if (x_save == 0 && y_save == 0 && z_save == 0)
		{
			set_color(BLUE);
			std::cout << "\nYou must first save your position" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			WriteProcessMemory(hProcess, (BYTE*)x_addr, &x_save, sizeof(x_save), nullptr);
			WriteProcessMemory(hProcess, (BYTE*)y_addr, &y_save, sizeof(y_save), nullptr);
			WriteProcessMemory(hProcess, (BYTE*)z_addr, &z_save, sizeof(z_save), nullptr);
			set_color(BLUE);
			std::cout << "\nLoaded Position" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_invisibility()
{
	//Invisibility for mobs
	bool invisibility_toggle = false;
	if (GetAsyncKeyState(key_invisibility) & 1)
	{
		invisibility_toggle = !invisibility_toggle;
	}
	if (invisibility_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)zheight1_addr, &invisibility, sizeof(invisibility), nullptr);
		if (invisibility < 2.7)
		{
			invisibility = 2.7;
			WriteProcessMemory(hProcess, (BYTE*)zheight1_addr, &invisibility, sizeof(invisibility), nullptr);
			set_color(GREEN);
			std::cout << "\nOn: Invisibility" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			ReadProcessMemory(hProcess, (BYTE*)y_addr, &y_value, sizeof(y_value), nullptr);
			y_value = y_value + 3;
			WriteProcessMemory(hProcess, (BYTE*)y_addr, &y_value, sizeof(y_value), nullptr);
			invisibility = 1;
			WriteProcessMemory(hProcess, (BYTE*)zheight1_addr, &invisibility, sizeof(invisibility), nullptr);
			set_color(RED);
			std::cout << "\nOff: Invisibility" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_wall_climb()
{
	//Wall Climb
	bool wallclimb_toggle = false;
	if (GetAsyncKeyState(key_wallclimb) & 1)
	{
		wallclimb_toggle = !wallclimb_toggle;
	}
	if (wallclimb_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)wallclimb_addr, &wallclimb, sizeof(wallclimb), nullptr);
		if (wallclimb < wallclimb_settings)
		{
			wallclimb = wallclimb_settings;
			WriteProcessMemory(hProcess, (BYTE*)wallclimb_addr, &wallclimb, sizeof(wallclimb), nullptr);
			set_color(GREEN);
			std::cout << "\nOn: Wall Climb" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			wallclimb = 2.1875;
			WriteProcessMemory(hProcess, (BYTE*)wallclimb_addr, &wallclimb, sizeof(wallclimb), nullptr);
			set_color(RED);
			std::cout << "\nOff: Wall Climb" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_clipping()
{
	//Clipping
	bool clipping_toggle = false;
	if (GetAsyncKeyState(key_clipping) & 1)
	{
		clipping_toggle = !clipping_toggle;
	}
	if (clipping_toggle)
	{
		ReadProcessMemory(hProcess, (BYTE*)zheight2_addr, &clipping, sizeof(clipping), nullptr);
		if (clipping < 99999)
		{
			clipping = 99999;
			WriteProcessMemory(hProcess, (BYTE*)zheight2_addr, &clipping, sizeof(clipping), nullptr);
			set_color(GREEN);
			std::cout << "\nOn: Clipping" << std::endl;
			set_color(DEFAULT);
		}
		else
		{
			clipping = 0;
			WriteProcessMemory(hProcess, (BYTE*)zheight2_addr, &clipping, sizeof(clipping), nullptr);
			set_color(RED);
			std::cout << "\nOff: Clipping" << std::endl;
			set_color(DEFAULT);
		}
	}
}

void Hack::hacks_fly()
{
	//Fly on hold
	ReadProcessMemory(hProcess, (BYTE*)gravity_addr, &fly, sizeof(fly), nullptr);
	bool fly_toggle = false;
	if (GetAsyncKeyState(key_fly))
	{
		fly_toggle = !fly_toggle;
	}
	if (fly_toggle)
	{
		if (fly_check == false)
		{
			if (fly < fly_settings)
			{
				fly = fly_settings;
				WriteProcessMemory(hProcess, (BYTE*)gravity_addr, &fly, sizeof(fly), nullptr);
				set_color(GREEN);
				std::cout << "\nOn: Fly" << std::endl;
				set_color(DEFAULT);
			}
		}
	}
	else
	{
		if (fly_check == false)
		{
			if (fly == 0)
			{
				fly_check = true;
			}
			else if (fly != -40.625)
			{
				fly = -40.625;
				WriteProcessMemory(hProcess, (BYTE*)gravity_addr, &fly, sizeof(fly), nullptr);
				set_color(RED);
				std::cout << "\nOff: Fly" << std::endl;
				set_color(DEFAULT);
			}
		}
		else
		{
			if (fly != 0)
			{
				fly_check = false;
			}
		}
	}
}
