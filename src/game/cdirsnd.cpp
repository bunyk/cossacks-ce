///////////////////////////////////////////////////////////
// CDIRSND.CPP -- Implementation file for the CDirSound
//                class.
///////////////////////////////////////////////////////////

#include "cdirsnd.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// Mixes up to additionalAmount bytes from one playing sound buffer into dest.
// dest receives additionalAmount BYTES of S16 stereo samples.
// Volume/pan use the DirectSound-style dB values stored on the buffer.
static void StreamAudioBuffer(SoundBuffer* pSoundBuffer, Sint16* destSamples, int additionalAmount)
{
	if (!pSoundBuffer->playing)
	{
		return;
	}

	int chunkBytes = (int)(pSoundBuffer->size - pSoundBuffer->pos);
	if (chunkBytes > additionalAmount)
	{
		chunkBytes = additionalAmount;
	}
	if (chunkBytes <= 0)
	{
		pSoundBuffer->playing = false;
		return;
	}

	// DirectSound volume: 0 (loudest) .. -10000; pow scale, clamped to 0..1
	float volume = pow(10.0f, pSoundBuffer->volume / 2000.0f);
	if (volume < 0.0001f) volume = 0.0f;
	else if (volume > 1.0f) volume = 1.0f;

	// DirectSound pan: -10000 (left only) .. 10000 (right only)
	float leftPan, rightPan;
	if (pSoundBuffer->pan < 0)
	{
		leftPan = 1.0f;
		rightPan = pow(10.0f, pSoundBuffer->pan / 2000.0f);
	}
	else if (pSoundBuffer->pan > 0)
	{
		leftPan = pow(10.0f, -pSoundBuffer->pan / 2000.0f);
		rightPan = 1.0f;
	}
	else
	{
		leftPan = 1.0f;
		rightPan = 1.0f;
	}
	if (leftPan < 0.0001f) leftPan = 0;
	else if (leftPan > 1.0f) leftPan = 1.0f;
	if (rightPan < 0.0001f) rightPan = 0;
	else if (rightPan > 1.0f) rightPan = 1.0f;

	const Sint16* src = (const Sint16*)((byte*)pSoundBuffer->data + pSoundBuffer->pos);
	int chunkSamples = chunkBytes / 2;
	for (int i = 0; i < chunkSamples; i += 2)
	{
		float l = src[i] * volume * leftPan;
		float r = src[i + 1] * volume * rightPan;
		int dl = destSamples[i] + (int)l;
		int dr = destSamples[i + 1] + (int)r;
		destSamples[i] = (Sint16)(dl > 32767 ? 32767 : (dl < -32768 ? -32768 : dl));
		destSamples[i + 1] = (Sint16)(dr > 32767 ? 32767 : (dr < -32768 ? -32768 : dr));
	}

	pSoundBuffer->pos += chunkBytes;

	if (pSoundBuffer->pos >= pSoundBuffer->size)
	{
		pSoundBuffer->playing = false;
	}
}

// One get-callback for the whole system: mix every playing buffer into a
// fresh block and put the result on the (single) master stream.
static void SDLCALL MasterStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount)
{
	CDirSound* ds = (CDirSound*)userdata;

	Sint16* mix = (Sint16*)malloc(additionalAmount);
	if (!mix)
	{
		return;
	}
	memset(mix, 0, additionalAmount);

	for (int i = 1; i <= (int)ds->m_currentBufferNum; i++)
	{
		SoundBuffer* sb = ds->m_bufferPointers[i];
		if (sb && sb->playing)
		{
			StreamAudioBuffer(sb, mix, additionalAmount);
		}
	}

	SDL_PutAudioStreamData(stream, mix, additionalAmount);
	free(mix);
}

///////////////////////////////////////////////////////////
// CDirSound::CDirSound()
//
// This is the class's constructor.
///////////////////////////////////////////////////////////
void CDirSound::CreateDirSound()
{
	// Initialize class data members.
	m_currentBufferNum = 0;
	m_masterStream = NULL;

	for (UINT x = 0; x < MAXSND1; ++x)
	{
		m_bufferPointers[x] = NULL;
	}

	// Create the main SDL audio device.
	bool success;
	success = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (success)
	{
		m_SDLAudioDeviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_audioSpec);
			if (m_SDLAudioDeviceID)
		{
			SDL_ResumeAudioDevice(m_SDLAudioDeviceID);
			// ONE master stream for the whole system; its get-callback mixes
			// all playing buffers (replaces the old one-stream-per-buffer
			// design that opened ~200 streams on the device).
			m_masterStream = SDL_OpenAudioDeviceStream(m_SDLAudioDeviceID, &m_audioSpec, MasterStreamCallback, this);
					if (m_masterStream)
			{
				SDL_ResumeAudioStreamDevice(m_masterStream);
			}
		}
	}
}

///////////////////////////////////////////////////////////
// CDirSound::~CDirSound()
//
// This is the class's destructor.
///////////////////////////////////////////////////////////
CDirSound::~CDirSound()
{
	if (m_SDLAudioDeviceID)
		ReleaseAll();
}

///////////////////////////////////////////////////////////
// CDirSound::ReleaseAll()
//
// This member function releases all the DirectSound
// objects created in the class.
///////////////////////////////////////////////////////////
void CDirSound::ReleaseAll()
{
	// Release all sound buffers.
	for (UINT x = 1; x <= m_currentBufferNum; ++x)
	{
		if (!m_bufferPointers[x]->isDuplicated)
		{
			SDL_free(m_bufferPointers[x]->data);
		}
		free(m_bufferPointers[x]);
		m_bufferPointers[x] = NULL;
	}

	// Release the master stream, then the DirectSound object. Detach the
	// get-callback first so the device thread goes idle during teardown
	// (see CDeviceCD::Close for the same pattern).
	if (m_masterStream)
	{
		SDL_SetAudioStreamGetCallback(m_masterStream, nullptr, nullptr);
		SDL_DestroyAudioStream(m_masterStream);
		m_masterStream = NULL;
	}
	if (m_SDLAudioDeviceID)
		SDL_CloseAudioDevice(m_SDLAudioDeviceID);
}

///////////////////////////////////////////////////////////
// CDirSound::CreateSoundBuffer()
//
// This member function creates secondary sound buffers.
// The class can accommodate up to MAXSND such buffers.
///////////////////////////////////////////////////////////
UINT CDirSound::CreateSoundBuffer(CWave* pWave)
{
	// Make sure there's room for another buffer.
	if (m_currentBufferNum == MAXSND)
		return 0;

	// Calculate the next available buffer number.
	++m_currentBufferNum;

	m_bufferPointers[m_currentBufferNum] = (SoundBuffer*)malloc(sizeof(SoundBuffer));
	if (!m_bufferPointers[m_currentBufferNum])
	{
		return 0;
	}
	memset(m_bufferPointers[m_currentBufferNum], 0, sizeof(SoundBuffer));

	return m_currentBufferNum;
}
///////////////////////////////////////////////////////////
// CDirSound::DuplicateSoundBuffer()
//
// This member function creates secondary sound buffers.
// The class can accommodate up to MAXSND such buffers.
///////////////////////////////////////////////////////////
UINT CDirSound::DuplicateSoundBuffer(UINT bufferNum)
{
	// TODO: maybe remove this and create different buffer for the same sound

	// Make sure there's room for another buffer.
	if (m_currentBufferNum == MAXSND)
		return 0;
	// Calculate the next available buffer number.
	++m_currentBufferNum;

	m_bufferPointers[m_currentBufferNum] = (SoundBuffer*)malloc(sizeof(SoundBuffer));
	if (!m_bufferPointers[m_currentBufferNum])
	{
		return 0;
	}
	memcpy(m_bufferPointers[m_currentBufferNum], m_bufferPointers[bufferNum], sizeof(SoundBuffer));
	m_bufferPointers[m_currentBufferNum]->isDuplicated = true;
	// No per-buffer stream: the master stream's callback mixes all buffers.

	return m_currentBufferNum;
}
///////////////////////////////////////////////////////////
// CDirSound::CopyWaveToBuffer()
//
// This function copies wave data to a DirectSound buffer.
///////////////////////////////////////////////////////////
BOOL CDirSound::CopyWaveToBuffer(CWave* pWave, UINT bufferNum)
{
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return FALSE;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer =
		m_bufferPointers[bufferNum];

	// No per-buffer stream: the master stream's callback mixes all buffers.

	// Copy the data into the buffer.
	char* pWaveData = pWave->GetWaveDataPtr();
	pSoundBuffer->size = pWave->GetWaveSize();
	pSoundBuffer->data = malloc(pSoundBuffer->size);
	pSoundBuffer->spec = *pWave->GetWaveSpecPtr();
	memcpy(pSoundBuffer->data, pWaveData, pSoundBuffer->size);

	return TRUE;
}

///////////////////////////////////////////////////////////
// CDirSound::DirectSoundOK()
//
// This member function returns TRUE if the DirectSound
// object was created and initialized okay. Otherwise, it
// returns FALSE.
///////////////////////////////////////////////////////////
// TODO: rename to SDLAudioOK
BOOL CDirSound::DirectSoundOK()
{
	if (!m_SDLAudioDeviceID)
		return FALSE;

	return TRUE;
}

///////////////////////////////////////////////////////////
// CDirSound::PlaySound()
//
// This member function plays the sound stored in the given
// buffer.
///////////////////////////////////////////////////////////
BOOL CDirSound::PlaySoundNum(UINT bufferNum)
{
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return FALSE;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	// Make sure the buffer is set to play from the beginning.
	pSoundBuffer->pos = 0;
	// Play the sound.
	pSoundBuffer->playing = true;
	return true;
}
BOOL CDirSound::PlayCoorSound(UINT bufferNum, int x, int vx)
{
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return FALSE;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	// Make sure the buffer is set to play from the beginning.
	pSoundBuffer->pos = 0;
	// Play the sound.
	pSoundBuffer->playing = true;

	pSoundBuffer->x = x;
	pSoundBuffer->y = vx;

	return TRUE;
}

extern int CenterX;

void CDirSound::ControlPan(UINT bufferNum) 
{
	if (m_bufferPointers[bufferNum]->playing)
	{
		m_bufferPointers[bufferNum]->x += m_bufferPointers[bufferNum]->y;
		int pan = (m_bufferPointers[bufferNum]->x - CenterX) << 1;

		if (pan < -4000)
		{
			pan = -4000;
		}

		if (pan > 4000)
		{
			pan = 4000;
		}

		SetPan(bufferNum, pan);

		if (rand() < 350)
		{
			IsPlaying(bufferNum);
		}
	}
}

void CDirSound::SetVolume(UINT bufferNum, int vol) {
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	// Make sure the buffer is set to play from the beginning.
	pSoundBuffer->volume = vol;
};
void CDirSound::SetPan(UINT bufferNum, int pan) {
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	pSoundBuffer->pan = pan;
}

void CDirSound::ProcessSoundSystem()
{
	for (int i = 1; i < (MAXSND1 < m_currentBufferNum+1 ? MAXSND1 : m_currentBufferNum+1); i++)
	{
		if (m_bufferPointers[i]->playing)
		{
			ControlPan(i);
		}
	}
}

///////////////////////////////////////////////////////////
// CDirSound::StopSound()
//
// This member function stops the sound stored in the given
// buffer.
///////////////////////////////////////////////////////////
BOOL CDirSound::StopSound(UINT bufferNum)
{
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return FALSE;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	// Make sure the buffer is set to play from the beginning.
	pSoundBuffer->playing = false;

	return TRUE;
}
int CDirSound::GetPos(UINT bufferNum)
{
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
		return 0;

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	// Make sure the buffer is set to play from the beginning.
	return pSoundBuffer->pos;

}

bool CDirSound::IsPlaying(UINT bufferNum)
{
	// Check for a valid buffer number.
	if ((bufferNum > m_currentBufferNum) || (bufferNum == 0))
	{
		return 0;
	}

	// Get a pointer to the requested buffer.
	SoundBuffer* pSoundBuffer = m_bufferPointers[bufferNum];

	return pSoundBuffer->playing;
}