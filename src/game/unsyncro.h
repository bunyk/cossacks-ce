#define MaxUns 100
#define maxUline 5

typedef char* lpCHAR;
char* its(int i);

struct RandSlot
{
	char* FileName;
	int Line;
	int Param;
	int rpos;
	int Type;
};

class Syncro
{
public:
	RandSlot RSL[maxUline];
	int NSlots;

	FILE* Outf;
	void Clear();
	void Save();
	void Copy(Syncro* Syn);
	Syncro() {};
	~Syncro() {};
};

extern Syncro SYN;
extern Syncro SYN1;
extern word rpos;

//Lockstep RNG divergence diagnostics (unsyncro.cpp): ring of recent
//deterministic random-read call sites, dumped once on the first
//RandIndex fork of a network game
void ResetRandTrace();
void DumpRandTrace();
