/*==============================================================================
Play Sound Example
Copyright (c), Firelight Technologies Pty, Ltd 2004-2018.

This example shows how to simply load and play multiple sounds, the simplest 
usage of FMOD. By default FMOD will decode the entire file into memory when it
loads. If the sounds are big and possibly take up a lot of RAM it would be
better to use the FMOD_CREATESTREAM flag, this will stream the file in realtime
as it plays.
==============================================================================*/
#include "fmod.hpp"
#include "lib/sound/common.h"

#include "play_sound.h"

#include <iostream>
#include "math.h"
#include "time.h"
using namespace std;


 FMOD::System     *system1;
 FMOD::Sound      *sound1, *sound2, *sound3, *sound4, *sound5;
 FMOD::Channel    *channel = 0;
 FMOD_RESULT       result;
 unsigned int      version;
 void             *extradriverdata = 0;

void init_sound()
{
    Common_Init(&extradriverdata);

    //Create a System object and initialize
    FMOD::System_Create(&system1);
    ERRCHECK(result);
    
    system1->init(32, FMOD_INIT_NORMAL, extradriverdata);
    ERRCHECK(result);

    // Add sounds to play from files
    system1->createSound("../../NiHandTracker/media/banque_son_wav/cymbale/cymbale6.wav", FMOD_DEFAULT, 0, &sound1);
    sound1->setMode(FMOD_LOOP_OFF);    

    system1->createSound("../../NiHandTracker/media/banque_son_wav/caisse_claire/caisse_claire14.wav", FMOD_DEFAULT, 0, &sound2);
    sound2->setMode(FMOD_LOOP_OFF);     

    system1->createSound("../../NiHandTracker/media/banque_son_wav/Grosse_caisse/grossecaisse9.wav", FMOD_DEFAULT, 0, &sound3);
    sound3->setMode(FMOD_LOOP_OFF);

    system1->createSound("../../NiHandTracker/media/banque_son_wav/Tom/Tom8.wav", FMOD_DEFAULT, 0, &sound4);
    sound4->setMode(FMOD_LOOP_OFF);

    system1->createSound("../../NiHandTracker/media/banque_son_wav/cymbale/cymbale4.wav", FMOD_DEFAULT, 0, &sound5);
    sound5->setMode(FMOD_LOOP_OFF);
}


void play_sound(int area)
{
    if (area==1)
    {
        result = system1->playSound(sound1, 0, false, &channel);
        channel->setVolume(0.8f);
    }

    if (area==2)
    {
        result = system1->playSound(sound2, 0, false, &channel);
        channel->setVolume(0.8f);
    }

    if (area==3)
    {
        result = system1->playSound(sound3, 0, false, &channel);
        channel->setVolume(3.0f); 
    }

    if (area==4)
    {
        result = system1->playSound(sound4, 0, false, &channel);
        channel->setVolume(0.8f);
    }

    if (area==5)
    {
        result = system1->playSound(sound5, 0, false, &channel);
        channel->setVolume(0.8f);
    }


    bool playing = false;

    do
    {
        result = channel->isPlaying(&playing);
    } while (playing);
}
