#pragma once

struct GAMEOBJ
{
	unsigned short Index;
	unsigned short Serial;
	int Type;
};

GAMEOBJ pikiner;
GAMEOBJ krestian;
GAMEOBJ pehota;
GAMEOBJ grenader;
GAMEOBJ strelec;
GAMEOBJ mameluk;
GAMEOBJ pushka;
GAMEOBJ mortira;
GAMEOBJ mortira_b;
GAMEOBJ lodka;
GAMEOBJ strelec2;
GAMEOBJ kozak;
GAMEOBJ oficer;
GAMEOBJ barabanshik;
GAMEOBJ officer_ATTACK;
GAMEOBJ officer_SHIELD;
GAMEOBJ center;
GAMEOBJ melnica;
GAMEOBJ barack;
GAMEOBJ sclad;
GAMEOBJ konushnia;
GAMEOBJ diplomatic;
GAMEOBJ minaret;
GAMEOBJ art_depo;
GAMEOBJ rinok;
GAMEOBJ kuznia;
GAMEOBJ mechet;
GAMEOBJ port;
GAMEOBJ galera;
GAMEOBJ peres_kor;
GAMEOBJ shebeka;
GAMEOBJ linkor;
GAMEOBJ kazarma;
GAMEOBJ MAIN;
GAMEOBJ AKA29;
GAMEOBJ AKA06;
GAMEOBJ Melnica_tuGETRES;
GAMEOBJ KUZ01;
GAMEOBJ pikiner_ATTACK;
GAMEOBJ pikiner_ATTACK3;
GAMEOBJ pikiner_ATTACK4;
GAMEOBJ pikiner_ATTACK5;
GAMEOBJ pikiner_ATTACK6;
GAMEOBJ pikiner_SHIELD;
GAMEOBJ pikiner_SHIELD3;
GAMEOBJ pikiner_SHIELD4;
GAMEOBJ pikiner_SHIELD5;
GAMEOBJ pikiner_SHIELD6;
GAMEOBJ pehota_ATTACK;
GAMEOBJ pehota_ATTACK3;
GAMEOBJ pehota_ATTACK4;
GAMEOBJ pehota_SHIELD;
GAMEOBJ pehota_SHIELD3;
GAMEOBJ pehota_SHIELD4;
GAMEOBJ mameluk_ATTACK;
GAMEOBJ mameluk_ATTACK3;
GAMEOBJ mameluk_ATTACK4;
GAMEOBJ mameluk_ATTACK5;
GAMEOBJ mameluk_ATTACK6;
GAMEOBJ mameluk_ATTACK7;
GAMEOBJ mameluk_SHIELD;
GAMEOBJ mameluk_SHIELD3;
GAMEOBJ mameluk_SHIELD4;
GAMEOBJ mameluk_SHIELD5;
GAMEOBJ mameluk_SHIELD6;
GAMEOBJ mameluk_SHIELD7;
GAMEOBJ pushka_BUILD;
GAMEOBJ pushka_BUILD3;
GAMEOBJ pushka_BUILD4;
GAMEOBJ pushka_BUILD5;
GAMEOBJ pushka_BUILD6;
GAMEOBJ mortira_BUILD;
GAMEOBJ mortira_BUILD3;
GAMEOBJ mortira_BUILD4;
GAMEOBJ mortira_BUILD5;
GAMEOBJ KUZ03;
GAMEOBJ AKA01;
GAMEOBJ AKA02;
GAMEOBJ AKA03;
GAMEOBJ AKA04;
GAMEOBJ AKA08;
GAMEOBJ AKA23;
GAMEOBJ AKA24;
GAMEOBJ AKA10;
GAMEOBJ AKA30;
GAMEOBJ AKA07;
GAMEOBJ AKA20;
GAMEOBJ KUZ02;
GAMEOBJ AKA09;
GAMEOBJ AKA05;
GAMEOBJ AKA28;
GAMEOBJ KUZ05;

// ----- Registered by the original Russia.dll, used by the water-map AI -----
// Tower (registered but not TryUnit'd by the Turkey-lineage land AI)
GAMEOBJ bashnia2;
// Yahta yacht (counted via GetUnitsByUsage for the fleet proxy)
GAMEOBJ yahta;
// 18th-century cavalry / support units (registered by the original DLL)
GAMEOBJ dragun18;
GAMEOBJ dragun18_dip;
GAMEOBJ gusar;
GAMEOBJ sveshenik;
GAMEOBJ strelec_algir;
// Gun-team unit: the original registers ONE of the two variants per game,
// picked with GetRandomIndex() (r & 6) == 2 ? kozak : koval.
GAMEOBJ tiagoli;
// Second 18th c. musketeer: original registers either Mushketer(RU) or
// Grenader(RU), picked with (r & 0x18) == 8.
GAMEOBJ strelec18;

// Upgrades registered by the original DLL that the water-map AI uses
GAMEOBJ KUZ04;
GAMEOBJ AKA35;
GAMEOBJ AKA36;
GAMEOBJ AKA32;
GAMEOBJ AKA11;
GAMEOBJ AKA12;
GAMEOBJ AKA13;
GAMEOBJ AKA14;
GAMEOBJ AKA15;
GAMEOBJ AKA31;
GAMEOBJ AKA33;
GAMEOBJ AKA34;
GAMEOBJ KUZ06;
GAMEOBJ bashnia_PAUSE;
GAMEOBJ bashnia_PAUSE3;
GAMEOBJ bashnia_PAUSE4;
GAMEOBJ bashnia_PAUSE5;
GAMEOBJ bashnia_PAUSE6;
GAMEOBJ bashnia_PAUSE7;
GAMEOBJ barabanshik_SHIELD;
GAMEOBJ dragun18_ATTACK;
GAMEOBJ dragun18_ATTACK3;
GAMEOBJ dragun18_ATTACK4;
GAMEOBJ dragun18_ATTACK5;
GAMEOBJ dragun18_ATTACK6;
GAMEOBJ dragun18_ATTACK7;
GAMEOBJ dragun18_SHIELD;
GAMEOBJ dragun18_SHIELD3;
GAMEOBJ dragun18_SHIELD4;
GAMEOBJ dragun18_SHIELD5;
GAMEOBJ dragun18_SHIELD6;
GAMEOBJ dragun18_SHIELD7;

