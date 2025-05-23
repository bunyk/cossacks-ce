#ifndef GFILE_H
#define GFILE_H

#include "resfile.h"

class GFILE{
	byte Buf[16384];
	int NBytesRead;
	int BufPos;
	int GlobalPos;
	int Size;
	ResFile F;
public:
	FILE* rf;
	bool RealText;
	GFILE();
	~GFILE();
	bool Open(char* Name);
	void Close();
	int ReadByte();
	int CheckByte();
	//standart functions
	int Gscanf(char* Mask,va_list args);
	int Ggetch();
};

DLLEXPORT GFILE* Gopen(char* Name, char* Mode);
DLLEXPORT int Gscanf(GFILE* F, char* mask,...);
DLLEXPORT int Ggetch(GFILE* F);
DLLEXPORT void Gprintf(GFILE* F, const char *format,...);
DLLEXPORT void Gclose(GFILE* F);

#endif
