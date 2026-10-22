// DeviceCD.cpp : implementation file
//

// #include "windows.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_audio.h"
#include "newcode/stb_vorbis.h"
#pragma pack(1)
#include "devicecd.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "resfile.h"
#include "gfile.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool PlayRandomTrack();

// Set when the music device is being torn down (CDeviceCD::Close). The
// audio device thread may be INSIDE PlayRandomTrack's multi-second ogg
// decode while the main thread closes the stream; without this flag the
// decode re-attaches the get-callback at its end and the device thread
// keeps feeding forever - SDL_CloseAudioDevice's join hung the shutdown.
static bool g_musicShutdown = false;

// Watchdog counter (see the stall check in ProcessSoundSystem)
volatile Uint64 g_musCallbackTicks = 0;

// Music track loader thread. CDeviceCD::Play decodes a whole Ogg Vorbis
// track (file I/O + multi-second CPU work). It used to run on the audio
// device thread (PipeWire's real-time thread, via the stream
// get-callback) when a track ended: the stall froze the whole audio
// device - music and sound effects share one device thread - and
// PipeWire then stopped scheduling the node, killing ALL audio
// permanently mid-game. Now the get-callback only requests the next
// track; this thread decodes it while the device thread keeps running
// (short silence between tracks instead of a dead device).
static SDL_Thread* g_musicLoader = NULL;
static SDL_Mutex* g_musicLoaderLock = NULL;
static SDL_Condition* g_musicLoaderCond = NULL;
enum { MUSIC_LOADER_IDLE, MUSIC_LOADER_REQUESTED, MUSIC_LOADER_LOADING };
static int g_musicLoaderState = MUSIC_LOADER_IDLE;
static bool g_musicLoaderQuit = false;
// Mirror of CDPLAY.audioStream (private member) for the loader thread,
// set in Open(), cleared in Close().
static SDL_AudioStream* g_musicStream = NULL;

static int SDLCALL MusicLoaderThread(void* userdata)
{
	while (true)
	{
		SDL_LockMutex(g_musicLoaderLock);
		while (g_musicLoaderState == MUSIC_LOADER_IDLE && !g_musicLoaderQuit)
		{
			SDL_WaitCondition(g_musicLoaderCond, g_musicLoaderLock);
		}
		if (g_musicLoaderQuit)
		{
			SDL_UnlockMutex(g_musicLoaderLock);
			break;
		}
		g_musicLoaderState = MUSIC_LOADER_LOADING;
		SDL_UnlockMutex(g_musicLoaderLock);

		// Decode the next track off the audio thread (the slow part).
		if (!g_musicShutdown && !PlayRandomTrack())
		{
			// Nothing playable (all track files missing) - stop re-arming
			// instead of spinning (same as the old in-callback behavior).
			if (g_musicStream)
			{
				SDL_SetAudioStreamGetCallback(g_musicStream, nullptr, nullptr);
			}
		}

		SDL_LockMutex(g_musicLoaderLock);
		g_musicLoaderState = MUSIC_LOADER_IDLE;
		SDL_UnlockMutex(g_musicLoaderLock);
	}
	return 0;
}

// Runs on the real-time audio device thread: must never block. Only
// signals the loader thread when the current track has fully drained.
static void SDLCALL MusicStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount)
{
	g_musCallbackTicks++;
	if (g_musicShutdown) return;
	if (!SDL_GetAudioStreamAvailable(stream))
	{
		if (g_musicLoaderLock)
		{
			SDL_LockMutex(g_musicLoaderLock);
			if (g_musicLoaderState == MUSIC_LOADER_IDLE)
			{
				g_musicLoaderState = MUSIC_LOADER_REQUESTED;
				SDL_SignalCondition(g_musicLoaderCond);
			}
			SDL_UnlockMutex(g_musicLoaderLock);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CDeviceCD
int StartTrack = 2;
int NTracks = 19;
byte TracksMask[32] = { 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };
CDeviceCD::CDeviceCD()
{
	m_volume = 100;
	// Intentionally deferred: this global object is constructed before
	// SDL_Init and before the game archives are registered (FilesInit), so
	// neither the audio device nor Gopen("Tracks.cd") would work here.
	// Play() does the lazy init on first use.
}

CDeviceCD::~CDeviceCD()
{
	Close();
}


bool CDeviceCD::Open()
{
	bool success = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (!success)
	{
		audioStream = nullptr;
		FOpened = false;
		audioError = true;
		return false;
	}

	SDL_AudioSpec audioSpec = { SDL_AUDIO_S16, 2, 44100 };
	audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audioSpec, nullptr, nullptr);
	if (!audioStream)
	{
		audioStream = nullptr;
		FOpened = false;
		audioError = true;
		return false;
	}

	SDL_SetAudioStreamGain(audioStream, static_cast<float>(m_volume) / 100.0f);
	success = SDL_ResumeAudioStreamDevice(audioStream);
	if (!success)
	{
		SDL_DestroyAudioStream(audioStream);
		audioStream = nullptr;
		FOpened = false;
		audioError = true;
		return false;
	}

	// Track loader thread (decodes the next track when one drains - must
	// not happen on the audio device thread, see MusicLoaderThread).
	g_musicStream = audioStream;
	if (!g_musicLoaderLock)
	{
		g_musicLoaderLock = SDL_CreateMutex();
		g_musicLoaderCond = SDL_CreateCondition();
	}
	if (g_musicLoaderLock && g_musicLoaderCond && !g_musicLoader)
	{
		g_musicLoader = SDL_CreateThread(MusicLoaderThread, "MusicLoader", nullptr);
	}

	FOpened = true;
	audioError = false;
	return true;
}

bool CDeviceCD::Close()
{
	if (FOpened)
	{
		// Teardown (this runs ONLY from ~CDeviceCD at exit()): set the
		// shutdown flag FIRST - Play()/MusicStreamCallback become no-ops.
		// The stream is NOT destroyed/paused here: the audio device thread
		// may be mid-callback (a multi-second stb_vorbis decode), and
		// destroy/pause block on the stream lock while the audio thread
		// queues decoded data -> the device-thread join deadlocks. The
		// process is exiting anyway - the OS reclaims the device.
		g_musicShutdown = true;

		// Stop the loader thread first: it may be mid-decode, but it never
		// waits on the audio device thread, so joining is safe here.
		if (g_musicLoaderLock)
		{
			SDL_LockMutex(g_musicLoaderLock);
			g_musicLoaderQuit = true;
			SDL_SignalCondition(g_musicLoaderCond);
			SDL_UnlockMutex(g_musicLoaderLock);
		}
		if (g_musicLoader)
		{
			SDL_WaitThread(g_musicLoader, nullptr);
			g_musicLoader = nullptr;
		}
		g_musicStream = nullptr;

		SDL_SetAudioStreamGetCallback(audioStream, nullptr, nullptr);

		FOpened = false;
		audioError = false;
		audioStream = nullptr;
		return true;
	}
	else
		return FALSE;
}

bool CDeviceCD::Pause()
{
	

	if (FOpened)
	{
		bool success = SDL_PauseAudioStreamDevice(audioStream);
		audioError = !success;

		if (!audioError)
			return true;
		else
			return false;
	}
	else
		return FALSE;
}

bool CDeviceCD::Resume()
{
	if (FOpened)
	{
		bool success = SDL_ResumeAudioStreamDevice(audioStream);
		audioError = !success;

		if (!audioError)
			return true;
		else
			return false;
	}
	else
		return FALSE;
}

bool CDeviceCD::Stop()
{
	return Pause();
}

DWORD CDeviceCD::GetVolume()
{
	return m_volume;
}

bool CDeviceCD::SetVolume(DWORD Volume)
{
	// Cache the desired volume; the options dialog calls this every frame,
	// and the stream may not exist yet (music not started) - the cached
	// value is applied when Open() creates the stream.
	m_volume = Volume;
	if (audioStream)
	{
		SDL_SetAudioStreamGain(audioStream, static_cast<float>(Volume) / 100.0f);
	}
	return true;
}
bool CDeviceCD::Play(DWORD Track)
{
	// Teardown guard: Play runs on the AUDIO DEVICE THREAD (from the
	// get-callback); once shutdown begins it must not touch the stream.
	if (g_musicShutdown) return false;

	if (!FOpened)
	{
		// Lazy init (constructor ran too early, before SDL/FilesInit):
		// load the track list, then open the audio device.
		GFILE* f = Gopen("Tracks.cd", "r");
		if (f) {
			Gscanf(f, "%d%d", &StartTrack, &NTracks);
			for (int i = 0; i < NTracks; i++)Gscanf(f, "%d", TracksMask + i);
			Gclose(f);
		}
		Open();
	}
	if (FOpened)
	{
		bool success;
		
		success = SDL_ClearAudioStream(audioStream);
		if (!success)
		{
			return false;
		}

		int channels;
		int sampleRate;
		short* buffer;
		int dataRead;

		//Uint8* audio_buf;
		//Uint32 audio_len;

		char filename[30];

		// Forward slashes: valid on Windows fopen too, required on Linux
		sprintf(filename, "Tracks/Track_%02d.ogg", Track);

		// Note: the decode blocks for seconds, so it must never run on the
		// audio device thread. The get-callback (re-arm path) loads tracks
		// via the loader thread (MusicLoaderThread). Direct calls from the
		// main thread (startup, music menu) block it briefly - acceptable.
		dataRead = stb_vorbis_decode_filename(filename, &channels, &sampleRate, &buffer);
		if (dataRead == -1)
		{
			return false;
		}

		// The decode takes seconds; shutdown may have begun meanwhile
		// (the main thread's atexit teardown). Do NOT touch the stream
		// now - its destruction may be in progress on the main thread
		// and the stream-lock/wait-thread combination deadlocks the join.
		if (g_musicShutdown)
		{
			SDL_free(buffer);
			return false;
		}

		SDL_AudioSpec srcSpec = { SDL_AUDIO_S16, channels, sampleRate };
		SDL_AudioSpec destSpec = { SDL_AUDIO_S16, 2, 44100 };
		SDL_AudioStream* conversionStream = SDL_CreateAudioStream(&srcSpec, &destSpec);
		if (!conversionStream)
		{
			SDL_free(buffer);
			return false;
		}
		success = SDL_PutAudioStreamData(conversionStream, buffer, dataRead * channels * 2);
		const char* error = SDL_GetError();
		if (!success)
		{
			SDL_free(buffer);
			SDL_DestroyAudioStream(conversionStream);
			return false;
		}
		success = SDL_FlushAudioStream(conversionStream);
		if (!success)
		{
			SDL_free(buffer);
			SDL_DestroyAudioStream(conversionStream);
			return false;
		}

		int availableBufLen = SDL_GetAudioStreamAvailable(conversionStream);

		void* convertedData = malloc(availableBufLen);
		int numRead = SDL_GetAudioStreamData(conversionStream, convertedData, availableBufLen);
		if (!(numRead == availableBufLen && numRead == dataRead * channels * 2))
		{
			SDL_free(buffer);
			SDL_DestroyAudioStream(conversionStream);
			free(convertedData);
			return false;
		}

		SDL_free(buffer);
		SDL_DestroyAudioStream(conversionStream);

		success = SDL_PutAudioStreamData(audioStream, convertedData, availableBufLen);
		SDL_free(convertedData);
		if (!success)
		{
			return false;
		}
		success = SDL_FlushAudioStream(audioStream);
		if (!success)
		{
			return false;
		}

		success = SDL_SetAudioStreamGetCallback(audioStream, MusicStreamCallback, nullptr);

		// One line per track start - low frequency, keep
		printf("Music: playing %s (%d samples, %d ch, %d Hz)\n", filename, dataRead, channels, sampleRate);

		return true;
	}
	else
		return false;
}
int PrevTrack3 = -1;
int PrevTrack2 = -1;
int PrevTrack1 = -1;
int NextCommand = -1;
bool PlayCDTrack(int Id);
extern int srando();

CDeviceCD CDPLAY;
// Returns false when the track file is missing or fails to decode
// (PlayRandomTrack uses that to fall back to another track).
bool PlayCDTrack(int Id) {
	if (g_musicShutdown) return false;
	return CDPLAY.Play(Id);
};
extern int CurrentNation;
extern int PlayMode;

extern uint64_t GetSDLTickCount();

bool PlayRandomTrack()
{
	if (PlayMode == 1 && CurrentNation != -1) {
		// National music: fall back to a random track when the nation's
		// track file is missing - e.g. the GOG Tracks/ rip has no
		// Track_20.ogg while the track table assigns 20 to one nation.
		if (PlayCDTrack(TracksMask[CurrentNation])) {
			return true;
		};
	};
	// Random track: time-based pseudo-random, no immediate repeats,
	// skips missing files, bounded (no spin on the audio thread).
	for (int attempts = 0; attempts < NTracks * 2; attempts++) {
		// Keep the pseudo-random spread INSIDE the track range: mask the
		// varied seed to 12 bits like the original (unwrapped sums gave
		// candidates above the last track - all decode failures - and
		// with an unlucky tick phase every attempt missed).
		int Track = (((((GetSDLTickCount() & 0xFFF) + attempts * 131) & 0xFFF) * NTracks) >> 12) + StartTrack;
		if (Track == PrevTrack1 || Track == PrevTrack2 || Track == PrevTrack3)continue;
		if (PlayCDTrack(Track)) {
			PrevTrack3 = PrevTrack2;
			PrevTrack2 = PrevTrack1;
			PrevTrack1 = Track;
			return true;
		};
	};
	return false;
}

void StopPlayCD()
{
	CDPLAY.Stop();
}

int GetCDVolume()
{
	return static_cast<int>(CDPLAY.GetVolume());
}

void SetCDVolume(int Vol)
{
	CDPLAY.SetVolume(static_cast<DWORD>(Vol));
}
