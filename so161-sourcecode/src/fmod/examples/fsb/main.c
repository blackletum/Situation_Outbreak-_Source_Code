/*===============================================================================================
 FSB Example
 Copyright (c), Firelight Technologies Pty, Ltd 2004-2009.

 This example demonstrates the usage of the FSB format, generated by the FSBank tool.
 It is simply a packed file format, with sounds contained within.  This is a cross platform
 format that is useful to keep the data format optimal for the platform you are targeting.
 (ie PS2, Xbox, Gamecube have native compression formats)
===============================================================================================*/
#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "../../api/inc/fmod.h"
#include "../../api/inc/fmod_errors.h"

//#include "../media/example.h"   /* Header with FSB indicies. */

void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}


int main(int argc, char *argv[])
{
    FMOD_SYSTEM    *system;
    FMOD_SOUND     *fsb;
    FMOD_CHANNEL   *channel = 0;
    FMOD_RESULT     result;
    int             key, count, numsubsounds;
    unsigned int    version;

    /*
        Create a System object and initialize.
    */
    result = FMOD_System_Create(&system);
    ERRCHECK(result);

    result = FMOD_System_GetVersion(system, &version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    result = FMOD_System_Init(system, 32, FMOD_INIT_NORMAL, 0);
    ERRCHECK(result);

    result = FMOD_System_CreateSound(system, "../media/example.fsb", FMOD_DEFAULT, 0, &fsb);
    ERRCHECK(result);

    printf("===================================================================\n");
    printf("FSB Example.  Copyright (c) Firelight Technologies 2004-2009.\n");
    printf("===================================================================\n");
    printf("\n");

    result = FMOD_Sound_GetNumSubSounds(fsb, &numsubsounds);
    ERRCHECK(result);

    for (count = 0; count < numsubsounds; count++)
    {
        FMOD_SOUND *subsound;
        char name[256];

        result = FMOD_Sound_GetSubSound(fsb, count, &subsound);
        ERRCHECK(result);

        result = FMOD_Sound_GetName(subsound, name, 256);
        ERRCHECK(result);

        printf("Press '%c' to play \"%s\"\n", '1' + count, name);
    }
    printf("Press 'Esc' to quit\n");
    printf("\n");

    /*
        Main loop.
    */
    do
    {
        if (kbhit())
        {
            key = getch();

            if (key >= '1' && key < '1' + numsubsounds)
            {
                FMOD_SOUND *subsound;
                int index = key - '1';

                FMOD_Sound_GetSubSound(fsb, index, &subsound);
               
                result = FMOD_System_PlaySound(system, FMOD_CHANNEL_FREE, subsound, FALSE, &channel);
                ERRCHECK(result);
            }
        }

        FMOD_System_Update(system);

        {
            unsigned int ms = 0;
            unsigned int lenms = 0;
            int          playing = 0;
            int          paused = 0;
            int          channelsplaying = 0;

            if (channel)
            {
                FMOD_SOUND *currentsound = 0;

                result = FMOD_Channel_IsPlaying(channel, &playing);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
                {
                    ERRCHECK(result);
                }

                result = FMOD_Channel_GetPaused(channel, &paused);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
                {
                    ERRCHECK(result);
                }

                result = FMOD_Channel_GetPosition(channel, &ms, FMOD_TIMEUNIT_MS);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
                {
                    ERRCHECK(result);
                }
               
                FMOD_Channel_GetCurrentSound(channel, &currentsound);
                if (currentsound)
                {
                    result = FMOD_Sound_GetLength(currentsound, &lenms, FMOD_TIMEUNIT_MS);
                    if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
                    {
                        ERRCHECK(result);
                    }
                }
            }

            FMOD_System_GetChannelsPlaying(system, &channelsplaying);

            printf("Time %02d:%02d:%02d/%02d:%02d:%02d : %s : Channels Playing %2d\r", ms / 1000 / 60, ms / 1000 % 60, ms / 10 % 100, lenms / 1000 / 60, lenms / 1000 % 60, lenms / 10 % 100, paused ? "Paused " : playing ? "Playing" : "Stopped", channelsplaying);
        }

        Sleep(10);

    } while (key != 27);

    printf("\n");

    /*
        Shut down
    */
    result = FMOD_Sound_Release(fsb);
    ERRCHECK(result);
    result = FMOD_System_Close(system);
    ERRCHECK(result);
    result = FMOD_System_Release(system);
    ERRCHECK(result);

    return 0;
}


