///////////////////////////////////////////////////////////
// CWAVE.H: Header file for the WAVE class.
///////////////////////////////////////////////////////////

#ifndef __CWAVE_H
#define __CWAVE_H

// #include <mmsystem.h>
#include "sdl3/sdl.h"
#include "sdl3/sdl_audio.h"

class CWave //: public CObject
{
protected:
    DWORD m_waveSize;
    BOOL m_waveOK;
    char* m_pWave;
    //WAVEFORMATEX m_waveFormatEx;
    SDL_AudioSpec m_waveSpec;

public:
    CWave(char* fileName);
    ~CWave();

    DWORD GetWaveSize();
    //LPWAVEFORMATEX GetWaveFormatPtr();
	SDL_AudioSpec* GetWaveSpecPtr();
    char* GetWaveDataPtr();
    BOOL WaveOK();

protected:
    BOOL LoadWaveFile(char* fileName);
};

#endif
