#include <windows.h>
#include <math.h>
#include "cube.h"


float wind_angle = 45.0;

unsigned int base;
unsigned int last_time;

long long int offset_x = 0;
long long int offset_y = 0;


const unsigned long long int MAX_CLOUD_DISTANCE = 65536 * 1000;
void __stdcall AdjustClouds(Vector3_Int64* cloud){
    unsigned int now = timeGetTime();
    unsigned int delta_time = now - last_time;
    last_time = now;
    offset_x += (long long int)((float)(delta_time * 500) * cos(degrees_to_radians(wind_angle)));
    offset_y += (long long int)((float)(delta_time * 500) * sin(degrees_to_radians(wind_angle)));


    long long int d = sqrt((cloud->x * cloud->x) + (cloud->y * cloud->y));
    cloud->z += d/100;
    //Move the cloud
    cloud->x += offset_x;//(long long int)((float)(total_time_offset * 5000) * cos(degrees_to_radians(wind_angle)));
    cloud->y += offset_y;//(long long int)((float)(total_time_offset * 5000) * sin(degrees_to_radians(wind_angle)));

    //Constrain the cloud to be near the player
    //Cloud coordinates are already relative to the player.
    cloud->x = (cloud->x % (MAX_CLOUD_DISTANCE*2)) - MAX_CLOUD_DISTANCE;
    cloud->y = (cloud->y % (MAX_CLOUD_DISTANCE*2)) - MAX_CLOUD_DISTANCE;

    float delta_wind_angle = (float)rand() / (float)RAND_MAX;
    delta_wind_angle = delta_wind_angle * (float)delta_time;
    delta_wind_angle = delta_wind_angle / 1000.0;


    wind_angle += delta_wind_angle;
}

unsigned int AdjustClouds_ptr = (unsigned int)&AdjustClouds;

unsigned int injection_JMP_back;
void __declspec(naked) injection(){
    //eax will contain the source long vector3 ptr
    asm("pusha");
    asm("push eax"); //Vector3_Int64*
    asm("call [_AdjustClouds_ptr]");
    asm("popa");

    //original code
    asm("push eax");
    asm("lea ecx, [esp+0xE50]");
    asm("jmp [_injection_JMP_back]");
}

void WriteJMP(BYTE* location, BYTE* newFunction){
    DWORD dwOldProtection;
    VirtualProtect(location, 5, PAGE_EXECUTE_READWRITE, &dwOldProtection);
    location[0] = 0xE9; //jmp
    *((DWORD*)(location + 1)) = (DWORD)(( (unsigned INT32)newFunction - (unsigned INT32)location ) - 5);
    VirtualProtect(location, 5, dwOldProtection, &dwOldProtection);
}

extern "C" __declspec(dllexport) bool APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            base = (unsigned int)GetModuleHandle(NULL);
            cube::SetBase(base);
            //initialize time
            last_time = timeGetTime();

            //Inject during cloud creation, while it accesses a vector containing its position.
            injection_JMP_back = base + 0xB190B;
            WriteJMP((BYTE*)(base + 0xB1903), (BYTE*)injection);
            break;

    }
    return true;
}
