#pragma once

// Ukraine AI GAMEOBJ handles. Var names keep the Algeria-lineage role
// names (see the role map in dllmain.cpp) so the handlers read 1:1
// against the Ukraine.dll decompilation.
struct GAMEOBJ
{
	unsigned short Index;
	unsigned short Serial;
	int Type;
};

GAMEOBJ strelec;			// Kozacki_Strelec(UA): 17th c. musketeer
GAMEOBJ krestian;			// Kres_ukranian(UA): peasant
GAMEOBJ grenader;			// Grenader_DIP(UA): mercenary grenadier
GAMEOBJ pikiner;			// Kozak_pikiner(UA): pikeman
GAMEOBJ pushka;				// Pushka(UA)
GAMEOBJ mortira;			// Mortira(UA)
GAMEOBJ mortira_b;			// Mortira_b(UA)
GAMEOBJ lodka;				// Lodka(UA): fishing boat
GAMEOBJ center;				// CenterUR(UA): town hall
GAMEOBJ melnica;			// Melnica_rus(UA): mill
GAMEOBJ barack;				// Strelcovaia_Izba(UA): 17th c. barracks
GAMEOBJ sclad;				// Sclad3(UA): storehouse
GAMEOBJ akademia;			// akademia_UA(UA)
GAMEOBJ konushnia;			// konushnia_ua(UA): stable
GAMEOBJ diplomatic;			// Dip_korpus_UA(UA): diplomatic corps
GAMEOBJ art_depo;			// artileri_depo_UA(UA)
GAMEOBJ rinok;				// Rinok(UA): market
GAMEOBJ kuznia;				// Kuznica_ua(UA): blacksmith
GAMEOBJ cerkov;				// Cerkov_UA(UA): church
GAMEOBJ port;				// Poru(UA)
GAMEOBJ galera;				// GALERA(UA)
GAMEOBJ peres_kor;			// PERES_KOR(UA): ferry

GAMEOBJ melnica_GETRES;		// Melnica_rus(UA)GETRES
GAMEOBJ melnica_GETRES2;	// Melnica_rus(UA)GETRES2
GAMEOBJ KUZ01;				// KUZ01UA
GAMEOBJ strelec_ATTACK;		// Kozacki_Strelec(UA)ATTACK
GAMEOBJ strelec_ATTACK3;
GAMEOBJ strelec_ATTACK4;
GAMEOBJ strelec_ATTACK5;
GAMEOBJ strelec_ATTACK6;
GAMEOBJ strelec_SHIELD;		// Kozacki_Strelec(UA)SHIELD
GAMEOBJ strelec_SHIELD3;
GAMEOBJ strelec_SHIELD4;
GAMEOBJ strelec_SHIELD5;
GAMEOBJ strelec_SHIELD6;
GAMEOBJ pikiner_ATTACK;		// Kozak_pikiner(UA)ATTACK
GAMEOBJ pikiner_ATTACK3;
GAMEOBJ pikiner_ATTACK4;
GAMEOBJ pikiner_ATTACK5;
GAMEOBJ pikiner_ATTACK6;
GAMEOBJ pikiner_ATTACK7;
GAMEOBJ pikiner_SHIELD;		// Kozak_pikiner(UA)SHIELD
GAMEOBJ pikiner_SHIELD3;
GAMEOBJ pikiner_SHIELD4;
GAMEOBJ pikiner_SHIELD5;
GAMEOBJ pikiner_SHIELD6;
GAMEOBJ pikiner_SHIELD7;
GAMEOBJ pushka_BUILD;		// Pushka(UA)BUILD
GAMEOBJ pushka_BUILD3;
GAMEOBJ pushka_BUILD4;
GAMEOBJ pushka_BUILD5;
GAMEOBJ pushka_BUILD6;
GAMEOBJ mortira_BUILD;		// Mortira(UA)BUILD
GAMEOBJ mortira_BUILD3;
GAMEOBJ mortira_BUILD4;
GAMEOBJ mortira_BUILD5;
GAMEOBJ KUZ03;				// KUZ03UA
GAMEOBJ AKA01;				// AKA01UA (academy grid)
GAMEOBJ AKA02;
GAMEOBJ AKA03;
GAMEOBJ AKA04;
GAMEOBJ AKA08;
GAMEOBJ AKA24;
GAMEOBJ AKA33;
GAMEOBJ AKA12;
GAMEOBJ AKA13;
GAMEOBJ AKA14;
GAMEOBJ AKA15;
GAMEOBJ AKA10;
GAMEOBJ AKA07;
GAMEOBJ KUZ02;				// KUZ02UA
GAMEOBJ AKA09;
GAMEOBJ AKA05;
GAMEOBJ AKA28;
GAMEOBJ KUZ05;				// KUZ05UA
GAMEOBJ AKA23;
GAMEOBJ AKA31;
GAMEOBJ AKA30;
GAMEOBJ AKA20;
