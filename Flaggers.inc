/*
Laggers filter by Rogue 2018/2/21
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Callbacks:
-=-=-=-=-=
OnPlayerLagout(playerid, lagtype, ping, Float:packetloss);
-=-=-=-=-
lagtype 1 = ping/packets lag.
lagtype 2 = packet-loss sync.
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Inner functions:
-=-=-=-=-=-=-=-=
IsPlayerLagging(playerid);
IsPlayerDesynced(playerid);
-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Defeinitions to use:
-=-=-=-=-=-=-=-=-=-=
#define FL_MAX_WAIT_TIME		[number]			( max wait time for the c-sync redo )
#define FL_MAX_LAG_PING 	    [number]   			( max ping to set a warning         )
#define FL_MAX_PACKETLOSS 	    [number] 			( max packet loss to set a warning  )
#define FL_MAX_WARNINGS 	    [number]   			( max warnings to call the callback )
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/

#if defined _FLaggersIncluded_
	#endinput
#endif

#define _FLaggersIncluded_
#define INVALID_PLAYER_STATS -1

#include <a_samp>

#if !defined FL_MAX_LAG_PING
	#define FL_MAX_LAG_PING 500
#endif

#if !defined FL_MAX_PACKETLOSS
	#define FL_MAX_PACKETLOSS 0.5
#endif

#if !defined FL_MAX_WAIT_TIME
	#define FL_MAX_WAIT_TIME 2
#endif

#if !defined FL_MAX_WARNINGS
	#define FL_MAX_WARNINGS 3
#endif

forward FL_CheckStats(playerid);
forward FL_Spawned(playerid);

#if defined OnPlayerLagout
	forward OnPlayerLagout(playerid, lagtype, ping, Float:packetloss);
#endif

#if defined FL_OnPlayerConnect
	forward FL_OnPlayerConnect(playerid);
#endif

#if defined FL_OnPlayerUpdate
	forward FL_OnPlayerUpdate(playerid);
#endif

#if defined FL_OnPlayerDisconnect
	forward FL_OnPlayerDisconnect(playerid, reason);
#endif

#if defined FL_OnPlayerSpawn
	forward FL_OnPlayerSpawn(playerid);
#endif

#if defined FL_OnPlayerDeath
	forward FL_OnPlayerDeath(playerid, killerid, reason);
#endif

enum FLAGGERS_PLAYER_INFO
{
	bool:FLdesynced,
	bool:FLlagger,
	bool:FLdcheck,
	bool:FLspawn,
	bool:FLTSet,

	Float:FLPackets,

	FLTicks,
	FLwarns,
	FLAmmo,
	FLPing,

	FLTimer

};

new 
	FLaggers_Player[MAX_PLAYERS][FLAGGERS_PLAYER_INFO];

stock Float:FL_GetPlayerPacketloss(playerid) // Made by Fusez
{
    new nstats[400+1], nstats_loss[20], start, end;
    GetPlayerNetworkStats(playerid, nstats, sizeof(nstats));

    start = strfind(nstats,"packetloss",true);
    end = strfind(nstats,"%",true,start);

    strmid(nstats_loss, nstats, start+12, end, sizeof(nstats_loss));
    return floatstr(nstats_loss);
}

public OnPlayerConnect(playerid)
{
	FLaggers_Player[playerid][FLspawn] = false;
	FLaggers_Player[playerid][FLTSet]  = false;
	FLaggers_Player[playerid][FLPackets] = INVALID_PLAYER_STATS;
	FLaggers_Player[playerid][FLPing] = INVALID_PLAYER_STATS;

	FLaggers_Player[playerid][FLTimer] = SetTimerEx("FL_CheckStats", 5000, true, "i", playerid);

		#if defined FL_OnPlayerConnect
	return FL_OnPlayerConnect(playerid);
		#else
	return 1;
		#endif
}

public OnPlayerDisconnect(playerid, reason)
{
	FLaggers_Player[playerid][FLdesynced] = false;
	FLaggers_Player[playerid][FLlagger]   = false;
	FLaggers_Player[playerid][FLdcheck]   = false;
	FLaggers_Player[playerid][FLTSet]     = false;
	FLaggers_Player[playerid][FLspawn]    = false;

	FLaggers_Player[playerid][FLwarns]    = 0;

	KillTimer(FLaggers_Player[playerid][FLTimer]);

		#if defined FL_OnPlayerDisconnect
	return FL_OnPlayerDisconnect(playerid, reason);
		#else
	return 1;
		#endif
}

public OnPlayerUpdate(playerid)
{
	//Desync filter:
	if(gettime() > FLaggers_Player[playerid][FLTicks])
	{
		new
		    FLaggersWep,
			FlaggersAmmo;

		FLaggers_Player[playerid][FLTicks] = gettime()+FL_MAX_WAIT_TIME;
	    GetPlayerWeaponData(playerid, 0, FLaggersWep, FlaggersAmmo);
		
		if(FlaggersAmmo != FLaggers_Player[playerid][FLAmmo] && FLaggers_Player[playerid][FLspawn])
		{
			FLaggers_Player[playerid][FLdesynced] = true;
			#if defined OnPlayerLagout
			OnPlayerLagout(playerid, 2, GetPlayerPing(playerid), FL_GetPlayerPacketloss(playerid));
			#endif
		}
		else FLaggers_Player[playerid][FLdesynced] = false;

		SetPlayerAmmo(playerid, FLaggersWep, (FlaggersAmmo == 0) ? 1 : 0);
		FLaggers_Player[playerid][FLAmmo] = FlaggersAmmo;
	}
		#if defined FL_OnPlayerUpdate
	return FL_OnPlayerUpdate(playerid);
		#else
	return 1;
		#endif
}

public OnPlayerSpawn(playerid)
{
	FLaggers_Player[playerid][FLspawn] = false;
	if(!FLaggers_Player[playerid][FLTSet]) FLaggers_Player[playerid][FLTSet] = true, SetTimerEx("FL_Spawned", 5000, false, "i", playerid);
		#if defined FL_OnPlayerSpawn
	return FL_OnPlayerSpawn(playerid);
		#else
	return 1;
		#endif
}

public OnPlayerDeath(playerid, killerid, reason)
{
	FLaggers_Player[playerid][FLspawn] = false;
		#if defined FL_FL_OnPlayerDeath
	return FL_OnPlayerDeath(playerid, killerid, reason);
		#else
	return 1;
		#endif
}

public FL_Spawned(playerid) return FLaggers_Player[playerid][FLspawn] = true, FLaggers_Player[playerid][FLTSet] = false;

public FL_CheckStats(playerid)
{
	new FL_PING = GetPlayerPing(playerid);
	new Float:FL_PACKETS = FL_GetPlayerPacketloss(playerid);
	if(FLaggers_Player[playerid][FLwarns] < FL_MAX_WARNINGS && FLaggers_Player[playerid][FLspawn])
	{
		//Ping filter:
		if(FL_PING >= FL_MAX_LAG_PING && FLaggers_Player[playerid][FLPing] == INVALID_PLAYER_STATS)
		{
			FLaggers_Player[playerid][FLPing] = FL_PING;
		}
		else if(FL_PING > FLaggers_Player[playerid][FLPing] && FLaggers_Player[playerid][FLPing] != INVALID_PLAYER_STATS)
		{

			FLaggers_Player[playerid][FLwarns]++, 
			FLaggers_Player[playerid][FLPing] = FL_PING;
		}
		
		else if(FLaggers_Player[playerid][FLPing] != INVALID_PLAYER_STATS) 
		{
			if(FLaggers_Player[playerid][FLwarns] > 0 ) FLaggers_Player[playerid][FLwarns]--;
			FLaggers_Player[playerid][FLPing] = INVALID_PLAYER_STATS;
		}

		//Packet-loss filter:
		if(FL_PACKETS >= FL_MAX_PACKETLOSS && FLaggers_Player[playerid][FLPackets] == INVALID_PLAYER_STATS)
		{ 
			FLaggers_Player[playerid][FLPackets] = FL_PACKETS;
		}

		else if(FL_PACKETS > FLaggers_Player[playerid][FLPackets] && FLaggers_Player[playerid][FLPackets] != INVALID_PLAYER_STATS)
		{
			FLaggers_Player[playerid][FLwarns]++, 
			FLaggers_Player[playerid][FLPackets] = FL_PACKETS;
		}

		else if(FLaggers_Player[playerid][FLPackets] != INVALID_PLAYER_STATS)
		{
			if(FLaggers_Player[playerid][FLwarns] > 0 ) FLaggers_Player[playerid][FLwarns]--;
			FLaggers_Player[playerid][FLPackets] = INVALID_PLAYER_STATS;
		}
	}
	//Final-Stage filter:
	else if(FLaggers_Player[playerid][FLspawn] && FLaggers_Player[playerid][FLwarns] >= FL_MAX_WARNINGS)
	{
		FLaggers_Player[playerid][FLwarns] = 0;
		#if defined OnPlayerLagout
		OnPlayerLagout(playerid, 1, FL_PING, FL_PACKETS);
		#endif
	}
	return 1;
}

//Inner functions:
stock IsPlayerLagging(playerid)
{
	if(FLaggers_Player[playerid][FLwarns] > 1) return 1;
	else return 0;
}

stock IsPlayerDesynced(playerid) return FLaggers_Player[playerid][FLdesynced];

#if defined _ALS_OnPlayerDisconnect
  #undef OnPlayerDisconnect
#else
    #define _ALS_OnPlayerDisconnect
#endif

#define OnPlayerDisconnect FL_OnPlayerDisconnect

#if defined _ALS_OnPlayerConnect
  #undef OnPlayerConnect
#else
    #define _ALS_OnPlayerConnect
#endif

#define OnPlayerConnect FL_OnPlayerConnect

#if defined _ALS_OnPlayerUpdate
  #undef OnPlayerUpdate
#else
    #define _ALS_OnPlayerUpdate
#endif

#define OnPlayerUpdate FL_OnPlayerUpdate

#if defined _ALS_OnPlayerSpawn
  #undef OnPlayerSpawn
#else
    #define _ALS_OnPlayerSpawn
#endif

#define OnPlayerSpawn FL_OnPlayerSpawn

#if defined _ALS_OnPlayerDeath
  #undef OnPlayerDeath
#else
    #define _ALS_OnPlayerDeath
#endif

#define OnPlayerDeath FL_OnPlayerDeath
