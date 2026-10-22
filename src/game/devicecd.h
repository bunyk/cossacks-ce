#include <SDL3/SDL.h>
#include "newcode/os.h"

typedef void VOIDFN();
class CDeviceCD
{
// Construction
public:
	CDeviceCD();

// Attributes
public:

// Operations
public:

// Implementation
public:
	bool Play(DWORD Track);
	bool SetVolume(DWORD Volume);
	DWORD GetVolume();
	bool Stop();
	bool Resume();
	bool Pause();
	bool Close();
	bool Open();
	virtual ~CDeviceCD();

	// Generated message map functions
protected:
	bool FOpened;
	SDL_AudioStream* audioStream;
	bool audioError;
	// Desired volume (0..100), independent of the stream's lifetime: the
	// stream may not exist yet (music never started) or be recreated, so
	// the volume is cached here and applied to the stream whenever one is
	// live. Without this, GetVolume() reported 0 before the first track
	// and the options dialog reset its slider to 0 on every reopen.
	DWORD m_volume;
};
