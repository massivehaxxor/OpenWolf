/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

// cl_main.c  -- client main loop

#include "../idLib/precompiled.h"
#include "client.h"
#include <limits.h>

#ifdef ET_MYSQL
#include "../database/database.h"
#endif

#if defined (GUIDMASTER_SUPPORT)
#define MASTERGUIDSERVER0 "localhost/auth1"
#define MASTERGUIDSERVER1 "localhost/auth2"
#ifdef _WIN32
	#include <windows.h>
#endif
#endif

#include "../snd_system/snd_local.h"

#include "../sys/sys_loadlib.h"
#include "../sys/sys_local.h"

convar_t         *cl_wavefilerecord;

#include "../qcommon/crypto.h"

#ifdef USE_MUMBLE
#include "../snd_system/libmumblelink.h"
#endif

#ifdef USE_MUMBLE
convar_t         *cl_useMumble;
convar_t         *cl_mumbleScale;
#endif

#ifdef USE_VOIP
convar_t         *cl_voipUseVAD;
convar_t         *cl_voipVADThreshold;
convar_t         *cl_voipSend;
convar_t         *cl_voipSendTarget;
convar_t         *cl_voipGainDuringCapture;
convar_t         *cl_voipCaptureMult;
convar_t         *cl_voipShowMeter;
convar_t	       *cl_voipShowSender;
convar_t         *cl_voip;
#endif

convar_t         *cl_nodelta;
convar_t         *cl_debugMove;

convar_t         *cl_noprint;
convar_t         *cl_motd;
convar_t         *cl_autoupdate;	// DHM - Nerve

convar_t         *rcon_client_password;
convar_t         *rconAddress;

convar_t         *cl_timeout;
convar_t         *cl_maxpackets;
convar_t         *cl_packetdup;
convar_t         *cl_timeNudge;
convar_t         *cl_showTimeDelta;
convar_t         *cl_freezeDemo;

convar_t         *cl_shownet = NULL;	// NERVE - SMF - This is referenced in msg.c and we need to make sure it is NULL
convar_t         *cl_shownuments;	// DHM - Nerve
convar_t         *cl_visibleClients;	// DHM - Nerve
convar_t         *cl_showSend;
convar_t         *cl_showServerCommands;	// NERVE - SMF
convar_t         *cl_timedemo;

convar_t         *cl_aviFrameRate;
convar_t         *cl_forceavidemo;

convar_t         *cl_freelook;
convar_t         *cl_sensitivity;
convar_t         *cl_xbox360ControllerAvailable;

convar_t         *cl_mouseAccelOffset; 
convar_t         *cl_mouseAccel;
convar_t         *cl_mouseAccelStyle; 
convar_t         *cl_showMouseRate;

convar_t         *m_pitch;
convar_t         *m_yaw;
convar_t         *m_forward;
convar_t         *m_side;
convar_t         *m_filter;

convar_t	       *j_pitch;
convar_t	       *j_yaw;
convar_t	       *j_forward;
convar_t	       *j_side;
convar_t         *j_up;
convar_t	       *j_pitch_axis;
convar_t	       *j_yaw_axis;
convar_t	       *j_forward_axis;
convar_t	       *j_side_axis;
convar_t         *j_up_axis;

convar_t         *cl_activeAction;

convar_t         *cl_autorecord;

convar_t         *cl_motdString;

convar_t         *cl_allowDownload;
convar_t         *cl_wwwDownload;
convar_t         *cl_conXOffset;
convar_t         *cl_inGameVideo;

convar_t         *cl_serverStatusResendTime;
convar_t         *cl_trn;
convar_t         *cl_missionStats;
convar_t         *cl_waitForFire;

// NERVE - SMF - localization
convar_t         *cl_language;
convar_t         *cl_debugTranslation;

// -NERVE - SMF
// DHM - Nerve :: Auto-Update
convar_t         *cl_updateavailable;
convar_t         *cl_updatefiles;

convar_t         *cl_pubkeyID;

// DHM - Nerve

convar_t         *cl_authserver;

convar_t         *cl_profile;
convar_t         *cl_defaultProfile;

convar_t         *cl_demorecording;	// fretn
convar_t         *cl_demofilename;	// bani
convar_t         *cl_demooffset;	// bani

convar_t         *cl_waverecording;	//bani
convar_t         *cl_wavefilename;	//bani
convar_t         *cl_waveoffset;	//bani

convar_t         *cl_packetloss;	//bani
convar_t         *cl_packetdelay;	//bani
extern qboolean sv_cheats;		//bani

convar_t         *cl_consoleKeys;
convar_t         *cl_consoleFont;
convar_t         *cl_consoleFontSize;
convar_t         *cl_consoleFontKerning;
convar_t	       *cl_consolePrompt;

struct rsa_public_key public_key; 
struct rsa_private_key private_key; 

convar_t         *cl_gamename;
convar_t         *cl_altTab;

static convar_t  *cl_renderer = NULL;
static void    *rendererLib = NULL;

// XreaL BEGIN
convar_t         *cl_aviMotionJpeg;
// XreaL END

clientActive_t  cl;
clientConnection_t clc;
clientStatic_t  cls;
vm_t           *cgvm;

// Structure containing functions exported from refresh DLL
refexport_t     re;

ping_t          cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s
{
	char            string[BIG_INFO_STRING];
	netadr_t        address;
	int             time, startTime;
	qboolean        pending;
	qboolean        print;
	qboolean        retrieved;
} serverStatus_t;

serverStatus_t  cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int             serverStatusCount;

// DHM - Nerve :: Have we heard from the auto-update server this session?
qboolean        autoupdateChecked;
qboolean        autoupdateStarted;

// TTimo : moved from char* to array (was getting the char* from va(), broke on big downloads)
char            autoupdateFilename[MAX_QPATH];

// "updates" shifted from -7
#define AUTOUPDATE_DIR "ni]Zm^l"
#define AUTOUPDATE_DIR_SHIFT 7

extern void     SV_BotFrame(int time);
void            CL_CheckForResend(void);
void            CL_ShowIP_f(void);
void            CL_ServerStatus_f(void);
void            CL_ServerStatusResponse(netadr_t from, msg_t * msg);
void            CL_SaveTranslations_f(void);
void            CL_LoadTranslations_f(void);

// fretn
void            CL_WriteWaveClose(void);
void            CL_WavStopRecord_f(void);

void CL_PurgeCache( void ) {
	cls.doCachePurge = qtrue;
}

void CL_DoPurgeCache( void ) {
	if ( !cls.doCachePurge ) {
		return;
	}

	cls.doCachePurge = qfalse;

	if ( !com_cl_running ) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( !cls.rendererStarted ) {
		return;
	}

	re.purgeCache();
}

#ifdef USE_MUMBLE
static void CL_UpdateMumble(void) {
	vec3_t pos, forward, up;
	float scale = cl_mumbleScale->value;
	float tmp;
	
	if(!cl_useMumble->integer)
		return;

	// !!! FIXME: not sure if this is even close to correct.
	AngleVectors( cl.snap.ps.viewangles, forward, NULL, up);

	pos[0] = cl.snap.ps.origin[0] * scale;
	pos[1] = cl.snap.ps.origin[2] * scale;
	pos[2] = cl.snap.ps.origin[1] * scale;

	tmp = forward[1];
	forward[1] = forward[2];
	forward[2] = tmp;

	tmp = up[1];
	up[1] = up[2];
	up[2] = tmp;

	if(cl_useMumble->integer > 1) {
		fprintf(stderr, "%f %f %f, %f %f %f, %f %f %f\n",
			pos[0], pos[1], pos[2],
			forward[0], forward[1], forward[2],
			up[0], up[1], up[2]);
	}

	mumble_update_coordinates(pos, forward, up);
}
#endif

#ifdef USE_VOIP
static
void CL_UpdateVoipIgnore(const char *idstr, qboolean ignore)
{
	if ((*idstr >= '0') && (*idstr <= '9')) {
		const int id = atoi(idstr);
		if ((id >= 0) && (id < MAX_CLIENTS)) {
			clc.voipIgnore[id] = ignore;
			CL_AddReliableCommand(va("voip %s %d",
			                         ignore ? "ignore" : "unignore", id));
			Com_Printf("VoIP: %s ignoring player #%d\n",
			            ignore ? "Now" : "No longer", id);
			return;
		}
	}
	Com_Printf("VoIP: invalid player ID#\n");
}

static
void CL_UpdateVoipGain(const char *idstr, float gain)
{
	if ((*idstr >= '0') && (*idstr <= '9')) {
		const int id = atoi(idstr);
		if (gain < 0.0f)
			gain = 0.0f;
		if ((id >= 0) && (id < MAX_CLIENTS)) {
			clc.voipGain[id] = gain;
			Com_Printf("VoIP: player #%d gain now set to %f\n", id, gain);
		}
	}
}

void CL_Voip_f( void )
{
	const char *cmd = Cmd_Argv(1);
	const char *reason = NULL;

	if (cls.state != CA_ACTIVE)
		reason = "Not connected to a server";
	else if (!clc.speexInitialized)
		reason = "Speex not initialized";
	else if (!clc.voipEnabled)
		reason = "Server doesn't support VoIP";

	if (reason != NULL) {
		Com_Printf("VoIP: command ignored: %s\n", reason);
		return;
	}

	if (!*cmd ) {
		Com_Printf("Invalid VoIP command, please use one listed below:\n\n"
			"^3help: ^1Display a menu for the usage of following commands.\n\n"
			"^3ignore: ^1Disable the client from speaking.\n\n"
			"^3unignore: ^1Enable the client to speak again.\n\n"
			"^3muteall: ^1Stop all incoming voice chat from all clients.\n\n"
			"^3unmuteall: ^1Allow all incoming voice chat from all clients.\n\n"
			"^3gain: ^1Modify how loud a client is.\n");
	}
	if ( !strcmp(cmd, "help") ) {
		Com_Printf("^6Help menu:\n\n"
			"^3help: ^1Display this menu.\n"
			"^5Usage: /voip help\n\n"
			"^3ignore: ^1Disable the client from speaking.\n"
			"^5Usage: /voip ignore <client number>\n\n"
			"^3unignore: ^1Enable the client to speak again.\n"
			"^5Usage: /voip unignore <client number>\n\n"
			"^3muteall: ^1Stop all incoming voice chat from all clients.\n"
			"^5Usage: /voip muteall\n\n"
			"^3unmuteall: ^1Allow all incoming voice chat from all clients.\n"
			"^5Usage: /voip unmuteall\n\n""^3gain: ^1Modify how loud a client is\n"
			"^5Usage: /voip gain <client number> <gain>\n\n"
			"^7If you do not know the clients number, it should be displayed in the lower right-hand corner when the client speaks.\n");
	}

	if (strcmp(cmd, "ignore") == 0) {
		CL_UpdateVoipIgnore(Cmd_Argv(2), qtrue);
	} else if (strcmp(cmd, "unignore") == 0) {
		CL_UpdateVoipIgnore(Cmd_Argv(2), qfalse);
	} else if (strcmp(cmd, "gain") == 0) {
		int id = 0;

		if (Cmd_Argc() > 3) {
			CL_UpdateVoipGain(Cmd_Argv(2), atof(Cmd_Argv(3)));
		} else if (Q_strtoi(Cmd_Argv(2), &id)) {
			int id = atoi(Cmd_Argv(2));
			if (id >= 0 && id < MAX_CLIENTS) {
				Com_Printf("VoIP: current gain for player #%d "
					"is %f\n", id, clc.voipGain[id]);
			} else {
				Com_Printf("VoIP: invalid player ID#\n");
			}
		} else {
			Com_Printf("usage: voip gain <playerID#> [value]\n");
		}
	} else if (strcmp(cmd, "muteall") == 0) {
		Com_Printf("VoIP: muting incoming voice\n");
		CL_AddReliableCommand("voip muteall");
		clc.voipMuteAll = qtrue;
	} else if (strcmp(cmd, "unmuteall") == 0) {
		Com_Printf("VoIP: unmuting incoming voice\n");
		CL_AddReliableCommand("voip unmuteall");
		clc.voipMuteAll = qfalse;
	} else {
		Com_Printf("usage: voip [un]ignore <playerID#>\n"
		           "       voip [un]muteall\n"
		           "       voip gain <playerID#> [value]\n");
	}
}


static
void CL_VoipNewGeneration(void)
{
	// don't have a zero generation so new clients won't match, and don't
	//  wrap to negative so MSG_ReadLong() doesn't "fail."
	clc.voipOutgoingGeneration++;
	if (clc.voipOutgoingGeneration <= 0)
		clc.voipOutgoingGeneration = 1;
	clc.voipPower = 0.0f;
	clc.voipOutgoingSequence = 0;
}

/*
===============
CL_VoipParseTargets

sets clc.voipTargets according to cl_voipSendTarget
Generally we don't want who's listening to change during a transmission,
so this is only called when the key is first pressed
===============
*/
void CL_VoipParseTargets(void)
{
	const char *target = cl_voipSendTarget->string;
	char *end;
	int val;

	Com_Memset(clc.voipTargets, 0, sizeof(clc.voipTargets));
	clc.voipFlags &= ~VOIP_SPATIAL;

	while(target)
	{
		while(*target == ',' || *target == ' ')
			target++;

		if(!*target)
			break;
		
		if(isdigit(*target))
		{
			val = strtol(target, &end, 10);
			target = end;
		}
		else
		{
			if(!Q_stricmpn(target, "all", 3))
			{
				Com_Memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
				return;
			}
			if(!Q_stricmpn(target, "spatial", 7))
			{
				clc.voipFlags |= VOIP_SPATIAL;
				target += 7;
				continue;
			}
			else
			{
				if(!Q_stricmpn(target, "attacker", 8))
				{
					val = VM_Call(cgvm, CG_LAST_ATTACKER);
					target += 8;
				}
				else if(!Q_stricmpn(target, "crosshair", 9))
				{
					val = VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
					target += 9;
				}
				else
				{
					while(*target && *target != ',' && *target != ' ')
						target++;

					continue;
				}

				if(val < 0)
					continue;
			}
		}

		if(val < 0 || val >= MAX_CLIENTS)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: VoIP "
				   "target %d is not a valid client "
				   "number\n", val);

			continue;
		}

		clc.voipTargets[val / 8] |= 1 << (val % 8);
	}
}

/*
===============
CL_CaptureVoip

Record more audio from the hardware if required and encode it into Speex
 data for later transmission.
===============
*/
static
void CL_CaptureVoip(void)
{
	const float audioMult = cl_voipCaptureMult->value;
	const qboolean useVad = (qboolean)(cl_voipUseVAD->integer != 0);
	qboolean initialFrame = qfalse;
	qboolean finalFrame = qfalse;

#if USE_MUMBLE
	// if we're using Mumble, don't try to handle VoIP transmission ourselves.
	if (cl_useMumble->integer)
		return;
#endif

	if (!clc.speexInitialized)
		return;  // just in case this gets called at a bad time.

	if (clc.voipOutgoingDataSize > 0)
		return;  // packet is pending transmission, don't record more yet.

	if (cl_voipUseVAD->modified) {
		Cvar_Set("cl_voipSend", (useVad) ? "1" : "0");
		cl_voipUseVAD->modified = qfalse;
	}

	if ((useVad) && (!cl_voipSend->integer))
		Cvar_Set("cl_voipSend", "1");  // lots of things reset this.

	if (cl_voipSend->modified) {
		qboolean dontCapture = qfalse;
		if (cls.state != CA_ACTIVE)
			dontCapture = qtrue;  // not connected to a server.
		else if (!clc.voipEnabled)
			dontCapture = qtrue;  // server doesn't support VoIP.
		else if (clc.demoplaying)
			dontCapture = qtrue;  // playing back a demo.
		else if ( cl_voip->integer == 0 )
			dontCapture = qtrue;  // client has VoIP support disabled.
		else if ( audioMult == 0.0f )
			dontCapture = qtrue;  // basically silenced incoming audio.

		cl_voipSend->modified = qfalse;

		if(dontCapture)
		{
			Cvar_Set("cl_voipSend", "0");
			return;
		}

		if (cl_voipSend->integer) {
			initialFrame = qtrue;
		} else {
			finalFrame = qtrue;
		}
	}

	// try to get more audio data from the sound card...

	if (initialFrame) {
		S_MasterGain(Com_Clamp(0.0f, 1.0f, cl_voipGainDuringCapture->value));
		S_StartCapture();
		CL_VoipNewGeneration();
		CL_VoipParseTargets();
	}

	if ((cl_voipSend->integer) || (finalFrame)) { // user wants to capture audio?
		int samples = S_AvailableCaptureSamples();
		const int mult = (finalFrame) ? 1 : 4; // 4 == 80ms of audio.

		// enough data buffered in audio hardware to process yet?
		if (samples >= (clc.speexFrameSize * mult)) {
			// audio capture is always MONO16 (and that's what speex wants!).
			//  2048 will cover 12 uncompressed frames in narrowband mode.
			static int16_t sampbuffer[2048];
			float voipPower = 0.0f;
			int speexFrames = 0;
			int wpos = 0;
			int pos = 0;

			if (samples > (clc.speexFrameSize * 4))
				samples = (clc.speexFrameSize * 4);

			// !!! FIXME: maybe separate recording from encoding, so voipPower
			// !!! FIXME:  updates faster than 4Hz?

			samples -= samples % clc.speexFrameSize;
			S_Capture(samples, (byte *) sampbuffer);  // grab from audio card.

			// this will probably generate multiple speex packets each time.
			while (samples > 0) {
				int16_t *sampptr = &sampbuffer[pos];
				int i, bytes;

				// preprocess samples to remove noise...
				speex_preprocess_run(clc.speexPreprocessor, sampptr);

				// check the "power" of this packet...
				for (i = 0; i < clc.speexFrameSize; i++) {
					const float flsamp = (float) sampptr[i];
					const float s = fabs(flsamp);
					voipPower += s * s;
					sampptr[i] = (int16_t) ((flsamp) * audioMult);
				}

				// encode raw audio samples into Speex data...
				speex_bits_reset(&clc.speexEncoderBits);
				speex_encode_int(clc.speexEncoder, sampptr,
				                 &clc.speexEncoderBits);
				bytes = speex_bits_write(&clc.speexEncoderBits,
				                         (char *) &clc.voipOutgoingData[wpos+1],
				                         sizeof (clc.voipOutgoingData) - (wpos+1));
				assert((bytes > 0) && (bytes < 256));
				clc.voipOutgoingData[wpos] = (byte) bytes;
				wpos += bytes + 1;

				// look at the data for the next packet...
				pos += clc.speexFrameSize;
				samples -= clc.speexFrameSize;
				speexFrames++;
			}

			clc.voipPower = (voipPower / (32768.0f * 32768.0f *
			                 ((float) (clc.speexFrameSize * speexFrames)))) *
			                 100.0f;

			if ((useVad) && (clc.voipPower < cl_voipVADThreshold->value)) {
				CL_VoipNewGeneration();  // no "talk" for at least 1/4 second.
			} else {
				clc.voipOutgoingDataSize = wpos;
				clc.voipOutgoingDataFrames = speexFrames;

				Com_DPrintf("VoIP: Send %d frames, %d bytes, %f power\n",
				            speexFrames, wpos, clc.voipPower);

				#if 0
				static FILE *encio = NULL;
				if (encio == NULL) encio = fopen("voip-outgoing-encoded.bin", "wb");
				if (encio != NULL) { fwrite(clc.voipOutgoingData, wpos, 1, encio); fflush(encio); }
				static FILE *decio = NULL;
				if (decio == NULL) decio = fopen("voip-outgoing-decoded.bin", "wb");
				if (decio != NULL) { fwrite(sampbuffer, speexFrames * clc.speexFrameSize * 2, 1, decio); fflush(decio); }
				#endif
			}
		}
	}

	// User requested we stop recording, and we've now processed the last of
	//  any previously-buffered data. Pause the capture device, etc.
	if (finalFrame) {
		S_StopCapture();
		S_MasterGain(1.0f);
		clc.voipPower = 0.0f;  // force this value so it doesn't linger.
	}
}
#endif

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
=====================
CL_GetReliableCommand
=====================
*/
char * CL_GetReliableCommand( int index )
{
	char * cmd = clc.reliableCommands[ index & (MAX_RELIABLE_COMMANDS-1) ];
	return (char*)(cmd?cmd:"");
}

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(const char *cmd)
{
	int             index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if(clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS)
	{
		Com_Error(ERR_DROP, "Client command overflow");
	}
	clc.reliableSequence++;
	index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);

	clc.reliableCommands[ index ] = Q_strcpy_ringbuffer(	clc.reliableCommandBuffer,
															sizeof( clc.reliableCommandBuffer ),
															clc.reliableCommands[ (clc.reliableAcknowledge) & ( MAX_RELIABLE_COMMANDS - 1 ) ],
															clc.reliableCommands[ (clc.reliableSequence-1) & ( MAX_RELIABLE_COMMANDS - 1 ) ],
															cmd
														);

	if ( !clc.reliableCommands[ index ] )
	{
		Com_Error( ERR_DROP, "Client command buffer overflow" );
	}
}

/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand(void)
{
	int             r, index, l;

	// NOTE TTimo: what is the randomize for?
	r = clc.reliableSequence - (random() * 5);
	index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
	l = strlen(clc.reliableCommands[index]);
	if(l >= MAX_STRING_CHARS - 1)
	{
		l = MAX_STRING_CHARS - 2;
	}
	clc.reliableCommands[index][l] = '\n';
	clc.reliableCommands[index][l + 1] = '\0';
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(msg_t * msg, int headerBytes)
{
	int             len, swlen;

	// write the packet sequence
	len = clc.serverMessageSequence;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);
	FS_Write(msg->data + headerBytes, len, clc.demofile);
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f(void)
{
	int             len;

	if(!clc.demorecording)
	{
		Com_Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	FS_Write(&len, 4, clc.demofile);
	FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;

	clc.demorecording = qfalse;
	Cvar_Set("cl_demorecording", "0");	// fretn
	Cvar_Set("cl_demofilename", "");	// bani
	Cvar_Set("cl_demooffset", "0");	// bani
	Com_Printf("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename(int number, char *fileName)
{
	if(number < 0 || number > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "demo9999");	// fretn - removed .tga
		return;
	}

	Com_sprintf(fileName, MAX_OSPATH, "demo%04i", number);
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/

static char     demoName[MAX_QPATH];	// compiler bug workaround
void CL_Record_f(void)
{
	char            name[MAX_OSPATH];
	int             len;
	char           *s;

	if(Cmd_Argc() > 2)
	{
		Com_Printf("record <demoname>\n");
		return;
	}

	if(clc.demorecording)
	{
		Com_Printf("Already recording.\n");
		return;
	}

	if(cls.state != CA_ACTIVE)
	{
		Com_Printf("You must be in a level to record.\n");
		return;
	}


	// ATVI Wolfenstein Misc #479 - changing this to a warning
	// sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
	if ( NET_IsLocalAddress( clc.serverAddress ) && !Cvar_VariableValue( "g_synchronousClients" ) ) {
		Com_Printf (S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	}

	if(Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		Q_strncpyz(demoName, s, sizeof(demoName));
		Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, com_protocol->integer);
	}
	else
	{
		int             number;

		// scan for a free demo name
		for(number = 0; number <= 9999; number++)
		{
			CL_DemoFilename(number, demoName);
			Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, com_protocol->integer);

			len = FS_ReadFile(name, NULL);
			if(len <= 0)
			{
				break;			// file doesn't exist
			}
		}
	}

	CL_Record(name);
}

void CL_Record(const char *name)
{
	int             i;
	msg_t           buf;
	byte            bufData[MAX_MSGLEN];
	entityState_t  *ent;
	entityState_t   nullstate;
	char           *s;
	int             len;

	// open the demo file

	Com_Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if(!clc.demofile)
	{
		Com_Printf("ERROR: couldn't open.\n");
		return;
	}

	clc.demorecording = qtrue;
	Cvar_Set("cl_demorecording", "1");	// fretn
	Q_strncpyz(clc.demoName, demoName, sizeof(clc.demoName));
	Cvar_Set("cl_demofilename", clc.demoName);	// bani
	Cvar_Set("cl_demooffset", "0");	// bani

	// don't start saving messages until a non-delta compressed message is received
	clc.demowaiting = qtrue;

	// write out the gamestate message
	MSG_Init(&buf, bufData, sizeof(bufData));
	MSG_Bitstream(&buf);

	// NOTE, MRE: all server->client messages now acknowledge
	MSG_WriteLong(&buf, clc.reliableSequence);

	MSG_WriteByte(&buf, svc_gamestate);
	MSG_WriteLong(&buf, clc.serverCommandSequence);

	// configstrings
	for(i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if(!cl.gameState.stringOffsets[i])
		{
			continue;
		}
		s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		MSG_WriteByte(&buf, svc_configstring);
		MSG_WriteShort(&buf, i);
		MSG_WriteBigString(&buf, s);
	}

	// baselines
	memset(&nullstate, 0, sizeof(nullstate));
	for(i = 0; i < MAX_GENTITIES; i++)
	{
		ent = &cl.entityBaselines[i];
		if(!ent->number)
		{
			continue;
		}
		MSG_WriteByte(&buf, svc_baseline);
		MSG_WriteDeltaEntity(&buf, &nullstate, ent, qtrue);
	}

	MSG_WriteByte(&buf, svc_EOF);

	// finished writing the gamestate stuff

	// write the client num
	MSG_WriteLong(&buf, clc.clientNum);
	// write the checksum feed
	MSG_WriteLong(&buf, clc.checksumFeed);

	// finished writing the client packet
	MSG_WriteByte(&buf, svc_EOF);

	// write it to the demo file
	len = LittleLong(clc.serverMessageSequence - 1);
	FS_Write(&len, 4, clc.demofile);

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf.data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/

void CL_DemoCompleted(void)
{
	if(cl_timedemo && cl_timedemo->integer)
	{
		int             time;

		time = Sys_Milliseconds() - clc.timeDemoStart;
		if(time > 0)
		{
			Com_Printf("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
					   time / 1000.0, clc.timeDemoFrames * 1000.0 / time);
		}
	}

	// fretn
	if(clc.waverecording)
	{
		CL_WriteWaveClose();
		clc.waverecording = qfalse;
	}

	CL_Disconnect(qtrue);
	CL_NextDemo();

}

/*
=================
CL_ReadDemoMessage
=================
*/

void CL_ReadDemoMessage(void)
{
	int             r;
	msg_t           buf;
	byte            bufData[MAX_MSGLEN];
	int             s;

	if(!clc.demofile)
	{
		CL_DemoCompleted();
		return;
	}

	// get the sequence number
	r = FS_Read(&s, 4, clc.demofile);
	if(r != 4)
	{
		CL_DemoCompleted();
		return;
	}

	clc.serverMessageSequence = LittleLong(s);

	// init the message
	MSG_Init(&buf, bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, clc.demofile);

	if(r != 4)
	{
		CL_DemoCompleted();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if(buf.cursize == -1)
	{
		CL_DemoCompleted();
		return;
	}
	if(buf.cursize > buf.maxsize)
	{
		Com_Error(ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
	}
	r = FS_Read(buf.data, buf.cursize, clc.demofile);
	if(r != buf.cursize)
	{
		Com_Printf("Demo file was truncated.\n");
		CL_DemoCompleted();
		return;
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CL_ParseServerMessage(&buf);
}

/*
====================

  Wave file saving functions

  FIXME: make this actually work

====================
*/

/*
==================
CL_DemoFilename
==================
*/
void CL_WavFilename(int number, char *fileName)
{
	if(number < 0 || number > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "wav9999");	// fretn - removed .tga
		return;
	}

	Com_sprintf(fileName, MAX_OSPATH, "wav%04i", number);
}

typedef struct wav_hdr_s
{
	unsigned int    ChunkID;	// big endian
	unsigned int    ChunkSize;	// little endian
	unsigned int    Format;		// big endian

	unsigned int    Subchunk1ID;	// big endian
	unsigned int    Subchunk1Size;	// little endian
	unsigned short  AudioFormat;	// little endian
	unsigned short  NumChannels;	// little endian
	unsigned int    SampleRate;	// little endian
	unsigned int    ByteRate;	// little endian
	unsigned short  BlockAlign;	// little endian
	unsigned short  BitsPerSample;	// little endian

	unsigned int    Subchunk2ID;	// big endian
	unsigned int    Subchunk2Size;	// little indian ;)

	unsigned int    NumSamples;
} wav_hdr_t;

wav_hdr_t       hdr;

static void CL_WriteWaveHeader(void)
{
	memset(&hdr, 0, sizeof(hdr));

	hdr.ChunkID = 0x46464952;	// "RIFF"
	hdr.ChunkSize = 0;			// total filesize - 8 bytes
	hdr.Format = 0x45564157;	// "WAVE"

	hdr.Subchunk1ID = 0x20746d66;	// "fmt "
	hdr.Subchunk1Size = 16;		// 16 = pcm
	hdr.AudioFormat = 1;		// 1 = linear quantization
	hdr.NumChannels = 2;		// 2 = stereo

	hdr.SampleRate = dma.speed;

	hdr.BitsPerSample = 16;		// 16bits

	// SampleRate * NumChannels * BitsPerSample/8
	hdr.ByteRate = hdr.SampleRate * hdr.NumChannels * (hdr.BitsPerSample / 8);

	// NumChannels * BitsPerSample/8
	hdr.BlockAlign = hdr.NumChannels * (hdr.BitsPerSample / 8);

	hdr.Subchunk2ID = 0x61746164;	// "data"

	hdr.Subchunk2Size = 0;		// NumSamples * NumChannels * BitsPerSample/8

	// ...
	FS_Write(&hdr.ChunkID, 44, clc.wavefile);
}

static char     wavName[MAX_QPATH];	// compiler bug workaround
void CL_WriteWaveOpen()
{
	// we will just save it as a 16bit stereo 22050kz pcm file

	char            name[MAX_OSPATH];
	int             len;
	char           *s;

	if(Cmd_Argc() > 2)
	{
		Com_Printf("wav_record <wavname>\n");
		return;
	}

	if(clc.waverecording)
	{
		Com_Printf("Already recording a wav file\n");
		return;
	}

	// yes ... no ? leave it up to them imo
	//if (cl_avidemo.integer)
	//  return;

	if(Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		Q_strncpyz(wavName, s, sizeof(wavName));
		Com_sprintf(name, sizeof(name), "wav/%s.wav", wavName);
	}
	else
	{
		int             number;

		// I STOLE THIS
		for(number = 0; number <= 9999; number++)
		{
			CL_WavFilename(number, wavName);
			Com_sprintf(name, sizeof(name), "wav/%s.wav", wavName);

			len = FS_FileExists(name);
			if(len <= 0)
			{
				break;			// file doesn't exist
			}
		}
	}

	Com_Printf("recording to %s.\n", name);
	clc.wavefile = FS_FOpenFileWrite(name);

	if(!clc.wavefile)
	{
		Com_Printf("ERROR: couldn't open %s for writing.\n", name);
		return;
	}

	CL_WriteWaveHeader();
	clc.wavetime = -1;

	clc.waverecording = qtrue;

	Cvar_Set("cl_waverecording", "1");
	Cvar_Set("cl_wavefilename", wavName);
	Cvar_Set("cl_waveoffset", "0");
}

void CL_WriteWaveClose()
{
	Com_Printf("Stopped recording\n");

	hdr.Subchunk2Size = hdr.NumSamples * hdr.NumChannels * (hdr.BitsPerSample / 8);
	hdr.ChunkSize = 36 + hdr.Subchunk2Size;

	FS_Seek(clc.wavefile, 4, FS_SEEK_SET);
	FS_Write(&hdr.ChunkSize, 4, clc.wavefile);
	FS_Seek(clc.wavefile, 40, FS_SEEK_SET);
	FS_Write(&hdr.Subchunk2Size, 4, clc.wavefile);

	// and we're outta here
	FS_FCloseFile(clc.wavefile);
	clc.wavefile = 0;
}

/*
====================
CL_CompleteDemoName
====================
*/
static void CL_CompleteDemoName( char *args, int argNum )
{
	if( argNum == 2 )
	{
		char demoExt[ 16 ];

		Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d", com_protocol->integer );
		Field_CompleteFilename( "demos", demoExt, qtrue );
	}
}



/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f(void)
{
	char            name[MAX_OSPATH], extension[32];
	char           *arg;
	int             prot_ver;

	if(Cmd_Argc() != 2)
	{
		Com_Printf("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");

	CL_Disconnect(qtrue);


//  CL_FlushMemory();   //----(SA)  MEM NOTE: in missionpack, this is moved to CL_DownloadsComplete


	// open the demo file
	arg = Cmd_Argv(1);
	prot_ver = com_protocol->integer - 1;
	while(prot_ver <= com_protocol->integer && !clc.demofile)
	{
		Com_sprintf(extension, sizeof(extension), ".dm_%d", prot_ver);
		if(!Q_stricmp(arg + strlen(arg) - strlen(extension), extension))
		{
			Com_sprintf(name, sizeof(name), "demos/%s", arg);
		}
		else
		{
			Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", arg, prot_ver);
		}
		FS_FOpenFileRead(name, &clc.demofile, qtrue);
		prot_ver++;
	}
	if(!clc.demofile)
	{
		Com_Error(ERR_DROP, "couldn't open %s", name);
		return;
	}
	Q_strncpyz(clc.demoName, Cmd_Argv(1), sizeof(clc.demoName));

	Con_Close();

	cls.state = CA_CONNECTED;
	clc.demoplaying = qtrue;

	if(Cvar_VariableValue("cl_wavefilerecord"))
	{
		CL_WriteWaveOpen();
	}

	Q_strncpyz(cls.servername, Cmd_Argv(1), sizeof(cls.servername));

	// read demo messages until connected
	while(cls.state >= CA_CONNECTED && cls.state < CA_PRIMED)
	{
		CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.firstDemoFrameSkipped = qfalse;
//  if (clc.waverecording) {
//      CL_WriteWaveClose();
//      clc.waverecording = qfalse;
//  }
}

/*
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void CL_StartDemoLoop( void ) {
	// start the demo loop again
	Cbuf_AddText ("d1\n");
	idKeyInput::SetCatcher( 0 );
}

/*
==================
CL_DemoState

Returns the current state of the demo system
==================
*/
demoState_t CL_DemoState( void ) {
	if( clc.demoplaying ) {
		return DS_PLAYBACK;
	} else if( clc.demorecording ) {
		return DS_RECORDING;
	} else {
		return DS_NONE;
	}
}

/*
==================
CL_DemoPos

Returns the current position of the demo
==================
*/
int CL_DemoPos( void ) {
	if( clc.demoplaying || clc.demorecording ) {
		return FS_FTell( clc.demofile );
	} else {
		return 0;
	}
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo(void)
{
	char            v[MAX_STRING_CHARS];

	Q_strncpyz(v, Cvar_VariableString("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS - 1] = 0;
	Com_DPrintf("CL_NextDemo: %s\n", v);
	if(!v[0])
	{
		return;
	}

	Cvar_Set("nextdemo", "");
	Cbuf_AddText(v);
	Cbuf_AddText("\n");
	Cbuf_Execute();
}

/*
==================
CL_DemoName

Returns the name of the demo
==================
*/
void CL_DemoName( char *buffer, int size ) {
	if( clc.demoplaying || clc.demorecording ) {
		Q_strncpyz( buffer, clc.demoName, size );
	} else if( size >= 1 ) {
		buffer[ 0 ] = '\0';
	}
}


//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void)
{

	// clear sounds
	S_DisableSounds();
	// download subsystem
	DL_Shutdown();
	// shutdown CGame
	CL_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI();

	// shutdown the renderer
	if(re.Shutdown)
	{
		re.Shutdown(qfalse);	// don't destroy window or context
	}

	if(re.purgeCache)
	{
		CL_DoPurgeCache();
	}

	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.rendererStarted = qfalse;
	cls.soundRegistered = qfalse;

	// Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
	if(clc.demorecording)
	{
		CL_StopRecord_f();
	}

	if(clc.waverecording)
	{
		CL_WavStopRecord_f();
	}
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void)
{

	// shutdown all the client stuff
	CL_ShutdownAll();

	// if not running a server clear the whole hunk
	if(!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
		// clear collision map data
		CM_ClearMap();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}

	Con_Close();
	cls.keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if ( cls.state >= CA_CONNECTED && !Q_stricmp( cls.servername, "localhost" ) ) {
		cls.state = CA_CONNECTED;       // so the connect screen is drawn
		memset( cls.updateInfoString, 0, sizeof( cls.updateInfoString ) );
		memset( clc.serverMessage, 0, sizeof( clc.serverMessage ) );
		memset( &cl.gameState, 0, sizeof( cl.gameState ) );
		clc.lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	} else {
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set( "nextmap", "" );
		CL_Disconnect( qtrue );
		Q_strncpyz( cls.servername, "localhost", sizeof( cls.servername ) );
		cls.state = CA_CHALLENGING;     // so the connect screen is drawn
		cls.keyCatchers = 0;
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( cls.servername, &clc.serverAddress, NA_UNSPEC );
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState(void)
{
	Com_Memset(&cl, 0, sizeof(cl));
}

/*
=====================
CL_ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void CL_ClearStaticDownload(void)
{
	assert(!cls.bWWWDlDisconnected);	// reset before calling
	cls.downloadRestart = qfalse;
	cls.downloadTempName[0] = '\0';
	cls.downloadName[0] = '\0';
	cls.originalDownloadName[0] = '\0';
}

/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(qboolean showMainMenu)
{
	if(!com_cl_running || !com_cl_running->integer)
	{
		return;
	}

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if(clc.demorecording)
	{
		CL_StopRecord_f();
	}

	if(!cls.bWWWDlDisconnected)
	{
		if(clc.download)
		{
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set("cl_downloadName", "");

		autoupdateStarted = qfalse;
		autoupdateFilename[0] = '\0';
	}

#ifdef USE_MUMBLE
	if (cl_useMumble->integer && mumble_islinked()) {
		Com_Printf("Mumble: Unlinking from Mumble application\n");
		mumble_unlink();
	}
#endif

#ifdef USE_VOIP
	if (cl_voipSend->integer) {
		int tmp = cl_voipUseVAD->integer;
		cl_voipUseVAD->integer = 0;  // disable this for a moment.
		clc.voipOutgoingDataSize = 0;  // dump any pending VoIP transmission.
		Cvar_Set("cl_voipSend", "0");
		CL_CaptureVoip();  // clean up any state...
		cl_voipUseVAD->integer = tmp;
	}

	if (clc.speexInitialized) {
		int i;
		speex_bits_destroy(&clc.speexEncoderBits);
		speex_encoder_destroy(clc.speexEncoder);
		speex_preprocess_state_destroy(clc.speexPreprocessor);
		for (i = 0; i < MAX_CLIENTS; i++) {
			speex_bits_destroy(&clc.speexDecoderBits[i]);
			speex_decoder_destroy(clc.speexDecoder[i]);
		}
	}
	Cmd_RemoveCommand ("voip");
#endif	
	
	if(clc.demofile)
	{
		FS_FCloseFile(clc.demofile);
		clc.demofile = 0;
	}

	if(uivm && showMainMenu)
	{
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE);
	}

	SCR_StopCinematic();
	S_ClearSoundBuffer();	//----(SA)    modified
#if 1
	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if(cls.state >= CA_CONNECTED)
	{
		CL_AddReliableCommand("disconnect");
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}
#endif
	CL_ClearState();

	// wipe the client connection
	Com_Memset(&clc, 0, sizeof(clc));

	if(!cls.bWWWDlDisconnected)
	{
		CL_ClearStaticDownload();
	}

	// allow cheats locally
	Cvar_Set("sv_cheats", "1");

	// not connected to a pure server anymore
	cl_connectedToPureServer = qfalse;

#ifdef USE_VOIP
	// not connected to voip server anymore.
	clc.voipEnabled = qfalse;
#endif	
	
	// XreaL BEGIN	
	// stop recording any video
	if(CL_VideoRecording())
	{
		// finish rendering current frame
		//SCR_UpdateScreen();
		CL_CloseAVI();
	}
	// XreaL END

	// show_bug.cgi?id=589
	// don't try a restart if uivm is NULL, as we might be in the middle of a restart already
	if(uivm && cls.state > CA_DISCONNECTED)
	{
		// restart the UI
		cls.state = CA_DISCONNECTED;

		// shutdown the UI
		CL_ShutdownUI();

		// init the UI
		CL_InitUI();
	}
	else
	{
		cls.state = CA_DISCONNECTED;
	}
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer(const char *string)
{
	char           *cmd;

	cmd = Cmd_Argv(0);

	// ignore key up commands
	if(cmd[0] == '-')
	{
		return;
	}

	if(clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+')
	{
		Com_Printf("Unknown command \"%s\"\n", cmd);
		return;
	}

	if(Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(string);
	}
	else
	{
		CL_AddReliableCommand(cmd);
	}
}


#if defined (USE_HTTP)

/*
===================
CL_Highscore_response

gets highscores results from the website
===================
*/
static int CL_Highscore_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "st_postresults", buffer );
		VM_Call(uivm, UI_REPORT_HIGHSCORE_RESPONSE );
	}
	return 1;
}

/*
===================
CL_Highscore_f

asks for highscores from the website
===================
*/
static void CL_Highscore_f( void ) {

	if ( Cmd_Argc() != 7 ) {
		Com_Printf("Usage: highscore <score> <skill> <kills> <time> <game> <real time>\n");
		return;
	}

	HTTP_PostUrl(	va("http://%s/user/report_score",AUTHORIZE_SERVER_NAME), CL_Highscore_response, 0,
					"c[slot]=%d&h[score]=%d&h[skill]=%d&h[kills]=%d&h[time]=%d&h[game_id]=%d&h[real_time]=%d",
					Cvar_VariableIntegerValue("slot"),
					atoi(Cmd_Argv(1)),
					atoi(Cmd_Argv(2)),
					atoi(Cmd_Argv(3)),
					atoi(Cmd_Argv(4)),
					atoi(Cmd_Argv(5)),
					atoi(Cmd_Argv(6)) );
}

/*
===================
CL_Login_response

website's response to a users attempt to login
===================
*/
static int CL_Login_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "cl_servermessage", Info_ValueForKey( buffer, "message" ) );

		switch( atoi( Info_ValueForKey( buffer, "status" ) ) )
		{
			case 1:		VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_OK );			break;
			case -1:	VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_NOTVERIFIED );	break;
			default:	VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_BAD );			break;
		}
	}

	//	VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_UNAVAILABLE);

	return length;
}

/*
==================
CL_Login_f
==================
*/
static void CL_Login_f( void )
{
	if ( Cmd_Argc() != 3) {
		Com_Printf( "usage: login user password\n");
		return;	
	}

	HTTP_PostUrl( va("http://%s/user/login", AUTHORIZE_SERVER_NAME), CL_Login_response, 0, "user[login]=%s&user[password]=%s&version=%d", Cmd_Argv(1), Cmd_Argv(2), 31 );
}

/*
==================
CL_ForgotPassword_f
==================
*/
void CL_ForgotPassword_f(void)
{
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: forgotpassword email\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/forgot_password", AUTHORIZE_SERVER_NAME), 0, 0, "user[email]=%s", Cmd_Argv(1) );
}

/*
==================
CL_CreateCharacter_response
==================
*/
static int CL_CreateCharacter_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "cl_servermessage", buffer );
		VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_CREATECHARACTER );
	}

	return 1;
}


/*
==================
CL_CreateCharacter_f
==================
*/
void CL_CreateCharacter_f(void)
{
	if ( Cmd_Argc() != 4 ) {
		Com_Printf( "usage: createcharacter <slot> <name> <model>\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/create_character", AUTHORIZE_SERVER_NAME), CL_CreateCharacter_response, 0, "char[slot]=%s&char[name]=%s&char[model]=%s", Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3) );
}

/*
==================
CL_DeleteCharacter_response
==================
*/
static int CL_DeleteCharacter_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		if ( buffer && buffer[0] == '1' ) {
			VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_DELETECHARACTER );
		} else {
			VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_BAD);
		}
	}

	return 1;
}

/*
==================
CL_DeleteCharacter_f
==================
*/
void CL_DeleteCharacter_f(void)
{
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: deletecharacter <slot>\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/delete_character", AUTHORIZE_SERVER_NAME), CL_DeleteCharacter_response, 0, "slot=%s", Cmd_Argv(1) );
}

/*
==================
CL_GetAccount_response
==================
*/
static int CL_GetAccount_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "cl_servermessage", buffer );
		VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_ACCOUNTINFO );
	}

	return 1;
}

/*
==================				 `
CL_GetAccount_f
==================
*/
void CL_GetAccount_f(void)
{
	if ( Cmd_Argc() != 1 ) {
		Com_Printf( "usage: getaccount\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/characters", AUTHORIZE_SERVER_NAME), CL_GetAccount_response, 0, 0 );
}

/*
==================
CL_GlobalHighScores_f
==================
*/
void CL_GlobalHighScores_f( void )
{
	if ( Cmd_Argc() != 3) {
		Com_Printf( "usage: globalhighscores [version] [char slot]\n");
		return;	
	}

	HTTP_PostUrl( va("http://%s/user/scores/version/%d/slot/%d",
		AUTHORIZE_SERVER_NAME, 
		atoi(Cmd_Argv(1)), 
		atoi(Cmd_Argv(2))), 
		0, 
		0, 
		0 );
}

#endif

/*
==================
CL_OpenUrl_f
==================
*/
void CL_OpenUrl_f( void )
{
	const char *url;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: openurl <url>\n" );
		return;
	}

	url = Cmd_Argv( 1 );
	
	{
		/*
			FixMe: URL sanity checks.

			Random sanity checks. Scott: if you've got some magic URL
			parsing and validating functions USE THEM HERE, this code
			is a placeholder!!!
		*/
		int i;
		const char *u;

		const char *allowPrefixes[] = { "http://", "https://", "" };
		const char *allowDomains[2] = { "localhost", 0 };

#if 0 //defined (USE_HTTP)
		allowDomains[1] = AUTHORIZE_SERVER_NAME;
#endif

		u = url;
		for( i = 0; i < lengthof( allowPrefixes ); i++ )
		{
			const char *p = allowPrefixes[i];
			size_t len = strlen( p );
			if( Q_strncmp( u, p, len ) == 0 )
			{
				u += len;
				break;
			}
		}

		if( i == lengthof( allowPrefixes ) )
		{
			/*
				This really won't ever hit because of the "" at the end
				of the allowedPrefixes array. As I said above, placeholder
				code: fix it later!
			*/
			Com_Printf( "Invalid URL prefix.\n" );
			return;
		}

		for( i = 0; i < lengthof( allowDomains ); i++ )
		{
			size_t len;
			const char *d = allowDomains[i];
			if ( !d )
				break;

			len = strlen( d );
			if( Q_strncmp( u, d, len ) == 0 )
			{
				u += len;
				break;
			}
		}

		if( i == lengthof( allowDomains ) )
		{
			Com_Printf( "Invalid domain.\n" );
			return;
		}
		/* my kingdom for a regex */
		for (i=0; i < strlen(url); i++)
		{
			if ( !(
				(url[i] >= 'a' && url[i] <= 'z') || // lower case alpha 
				(url[i] >= 'A' && url[i] <= 'Z') || // upper case alpha
				(url[i] >= '0' && url[i] <= '9') || //numeric
				(url[i] == '/') || (url[i] == ':' ) || // / and : chars
				(url[i] == '.' ) || (url[i] == '&') || // . and & chars
				(url[i] == ';' )						// ; char
				) ) {
				Com_Printf("Invalid URL\n");
				return;
			}
		}
	}

	if( !Sys_OpenUrl( url ) )
		Com_Printf( "System error opening URL\n" );
}


/*
===================
CL_RequestMotd

===================
*/
void CL_RequestMotd( void ) {
	char		info[MAX_INFO_STRING];

	if ( !cl_motd->integer ) {
		return;
	}
	Com_DPrintf( "Resolving %s\n", MASTER_SERVER_NAME );

	switch( NET_StringToAdr( MASTER_SERVER_NAME, &cls.updateServer,
	                         NA_UNSPEC ) )
	{
		case 0:
			Com_Printf( "Couldn't resolve master address\n" );
			return;

		case 2:
			cls.updateServer.port = BigShort( PORT_MASTER );
		default:
			break;
	}

	Com_DPrintf( "%s resolved to %s\n", MASTER_SERVER_NAME,
	            NET_AdrToStringwPort( cls.updateServer ) );

	info[0] = 0;

	Com_sprintf( cls.updateChallenge, sizeof( cls.updateChallenge ),
			"%i", ((rand() << 16) ^ rand()) ^ Com_Milliseconds());

	Info_SetValueForKey( info, "challenge", cls.updateChallenge );
	Info_SetValueForKey( info, "renderer", cls.glconfig.renderer_string );
	Info_SetValueForKey( info, "version", com_version->string );

	NET_OutOfBandPrint( NS_CLIENT, cls.updateServer, "getmotd%s", info );
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

#if defined (GUIDMASTER_SUPPORT)
char *CL_GenHWInfo(void)
{
#ifdef _WIN32
	HW_PROFILE_INFO HWInfo;
	if (GetCurrentHwProfileA(&HWInfo) != NULL)
	{
		return HWInfo.szHwProfileGuid;
	}
	else
	{
		return NULL;
	}
#else
	return va("%lx", gethostid());
#endif
}

void CL_GetAndRegGUID(void)
{
	netadr_t addr;
	if (!NET_StringToAdr(MASTERGUIDSERVER0, &addr, NA_IP))
	{
		if (!NET_StringToAdr(MASTERGUIDSERVER1, &addr, NA_IP))
		{
			Com_Printf("^3Can't connect to GUID server! GUIDKey generation is impossible.\n");
		}
	}
	else
	{
		NET_StringToAdr(MASTERGUIDSERVER0, &addr, NA_IP);
	}
	NET_OutOfBandPrint(NS_CLIENT, addr, "getguid %s", CL_GenHWInfo());
}

void CL_CmdGetGuid_f(void)
{
	CL_GetAndRegGUID();
	Com_Printf("Request has been sent.\n");
}

#endif


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f(void)
{
	if(cls.state != CA_ACTIVE || clc.demoplaying)
	{
		Com_Printf("Not connected to a server.\n");
		return;
	}

	// don't forward the first argument
	if(Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(Cmd_Args());
	}
}

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f(void)
{
	int             argc = Cmd_Argc();

	if(argc > 2)
	{
		char            buffer[1024];
		int             i;

		strcpy(buffer, Cmd_Argv(1));
		strcat(buffer, "=");

		for(i = 2; i < argc; i++)
		{
			strcat(buffer, Cmd_Argv(i));
			strcat(buffer, " ");
		}

		Q_putenv(buffer);
	}
	else if(argc == 2)
	{
		char           *env = getenv(Cmd_Argv(1));

		if(env)
		{
			Com_Printf("%s=%s\n", Cmd_Argv(1), env);
		}
		else
		{
			Com_Printf("%s undefined\n", Cmd_Argv(1));
		}
	}
}


/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f(void)
{
	SCR_StopCinematic();
	Cvar_Set("savegame_loading", "0");
	Cvar_Set("g_reloading", "0");
	if(cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC)
	{
		Com_Error(ERR_DISCONNECT, "Disconnected from server");
	}
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f(void)
{
	if(!strlen(cls.servername) || !strcmp(cls.servername, "localhost"))
	{
		Com_Printf("Can't reconnect to localhost.\n");
		return;
	}
	Cbuf_AddText(va("connect %s\n", cls.servername));
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f( void ) {
	char			*server;
	const char		*serverString;
	int				argc = Cmd_Argc();
	netadrtype_t	family = NA_UNSPEC;

	if ( argc != 2 && argc != 3 ) {
		Com_Printf( "usage: connect [-4|-6] server\n");
		return;	
	}
	
	if(argc == 2)
		server = Cmd_Argv(1);
	else
	{
		if(!strcmp(Cmd_Argv(1), "-4"))
			family = NA_IP;
		else if(!strcmp(Cmd_Argv(1), "-6"))
			family = NA_IP6;
		else
			Com_Printf( "warning: only -4 or -6 as address type understood.\n");
		
		server = Cmd_Argv(2);
	}

	S_StopAllSounds();      // NERVE - SMF

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set( "r_uiFullScreen", "0" );
	Cvar_Set( "ui_connecting", "1" );

	// fire a message off to the motd server
	CL_RequestMotd();

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
		// if running a local server, kill it
		SV_Shutdown( "Server quit\n" );
	}

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	SV_Frame( 0 );

	CL_Disconnect( qtrue );
	Con_Close();

	Q_strncpyz( cls.servername, server, sizeof( cls.servername ) );

	if ( !NET_StringToAdr( cls.servername, &clc.serverAddress, family ) ) {
		Com_Printf( "Bad server address\n" );
		cls.state = CA_DISCONNECTED;
		Cvar_Set( "ui_connecting", "0" );
		return;
	}
	if ( clc.serverAddress.port == 0 ) {
		clc.serverAddress.port = BigShort( PORT_SERVER );
	}

	serverString = NET_AdrToStringwPort(clc.serverAddress);

	Com_Printf( "%s resolved to %s\n", cls.servername, serverString);

	// if we aren't playing on a lan, we needto authenticate
	// with the cd key
	if ( NET_IsLocalAddress( clc.serverAddress ) ) {
		cls.state = CA_CHALLENGING;
	} else {
		cls.state = CA_CONNECTING;
	}

	Cvar_Set( "cl_avidemo", "0" );

	// show_bug.cgi?id=507
	// prepare to catch a connection process that would turn bad
	Cvar_Set( "com_errorDiagnoseIP", NET_AdrToString( clc.serverAddress ) );
	// ATVI Wolfenstein Misc #439
	// we need to setup a correct default for this, otherwise the first val we set might reappear
	Cvar_Set( "com_errorMessage", "" );

	cls.keyCatchers = 0;
	clc.connectTime = -99999;   // CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set( "cl_currentServerAddress", server );
	Cvar_Set( "cl_currentServerIP", serverString );

	// Gordon: um, couldnt this be handled
	// NERVE - SMF - reset some cvars
	Cvar_Set( "mp_playerType", "0" );
	Cvar_Set( "mp_currentPlayerType", "0" );
	Cvar_Set( "mp_weapon", "0" );
	Cvar_Set( "mp_team", "0" );
	Cvar_Set( "mp_currentTeam", "0" );

	Cvar_Set( "ui_limboOptions", "0" );
	Cvar_Set( "ui_limboPrevOptions", "0" );
	Cvar_Set( "ui_limboObjective", "0" );
	// -NERVE - SMF

}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f( void ) {
	char message[1024];
	netadr_t to;

	if ( !rcon_client_password->string ) {
		Com_Printf( "You must set 'rconPassword' before\n"
					"issuing an rcon command.\n" );
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	strcat( message, "rcon " );

	strcat( message, rcon_client_password->string );
	strcat( message, " " );

	// ATVI Wolfenstein Misc #284
	strcat( message, Cmd_Cmd() + 5 );

	if ( cls.state >= CA_CONNECTED ) {
		to = clc.netchan.remoteAddress;
	} else {
		if ( !strlen( rconAddress->string ) ) {
			Com_Printf( "You must either be connected,\n"
						"or set the 'rconAddress' cvar\n"
						"to issue rcon commands\n" );

			return;
		}
		NET_StringToAdr( rconAddress->string, &to, NA_UNSPEC );
		if ( to.port == 0 ) {
			to.port = BigShort( PORT_SERVER );
		}
	}

	NET_SendPacket( NS_CLIENT, strlen( message ) + 1, message, to );
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums(void)
{
	const char     *pChecksums;
	char            cMsg[MAX_INFO_VALUE];
	int             i;

	// if we are pure we need to send back a command with our referenced pk3 checksums
	pChecksums = FS_ReferencedPakPureChecksums();

	// "cp"
	Com_sprintf(cMsg, sizeof(cMsg), "Va ");
	Q_strcat(cMsg, sizeof(cMsg), va("%d ", cl.serverId));
	Q_strcat(cMsg, sizeof(cMsg), pChecksums);
	for(i = 0; i < 2; i++)
	{
		cMsg[i] += 13 + (i * 2);
	}
	CL_AddReliableCommand(cMsg);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer(void)
{
	CL_AddReliableCommand(va("vdr"));
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/

#ifdef _WIN32
extern void     Sys_In_Restart_f(void);	// fretn
#endif

void CL_Vid_Restart_f(void)
{
// XreaL BEGIN
	// settings may have changed so stop recording now
	if(CL_VideoRecording())
	{
		Cvar_Set("cl_avidemo", "0");
		CL_CloseAVI();
	}
// XreaL END

	// RF, don't show percent bar, since the memory usage will just sit at the same level anyway
	com_expectedhunkusage = -1;

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI();
	// shutdown the CGame
	CL_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart(clc.checksumFeed);

	S_BeginRegistration();		// all sound handles are now invalid

	cls.rendererStarted = qfalse;
	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.soundRegistered = qfalse;
	autoupdateChecked = qfalse;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");

	// if not running a server clear the whole hunk
	if(!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// startup all the client stuff
	CL_StartHunkUsers();

#ifdef _WIN32
	Sys_In_Restart_f();			// fretn
#endif
	// start the cgame if connected
	if(cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.cgameStarted = qtrue;
		CL_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
}

/*
=================
CL_UI_Restart_f

Restart the ui subsystem
=================
*/
void CL_UI_Restart_f(void)
{								// NERVE - SMF
	// shutdown the UI
	CL_ShutdownUI();

	autoupdateChecked = qfalse;

	// init the UI
	CL_InitUI();
}

/*
=================
CL_Snd_Reload_f

Reloads sounddata from disk, retains soundhandles.
=================
*/
void CL_Snd_Reload_f(void)
{
	// Dushan - FIX ME
	//S_Reload();
}


/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f(void)
{
	S_Shutdown();
	S_Init();

	CL_Vid_Restart_f();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f(void)
{
	Com_Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f(void)
{
	Com_Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f(void)
{
	int             i;
	int             ofs;

	if(cls.state != CA_ACTIVE)
	{
		Com_Printf("Not connected to a server.\n");
		return;
	}

	for(i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		ofs = cl.gameState.stringOffsets[i];
		if(!ofs)
		{
			continue;
		}
		Com_Printf("%4i: %s\n", i, cl.gameState.stringData + ofs);
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f(void)
{
	Com_Printf("--------- Client Information ---------\n");
	Com_Printf("state: %i\n", cls.state);
	Com_Printf("Server: %s\n", cls.servername);
	Com_Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO));
	Com_Printf("--------------------------------------\n");
}

/*
==============
CL_WavRecord_f
==============
*/

void CL_WavRecord_f(void)
{
	if(clc.wavefile)
	{
		Com_Printf("Already recording a wav file\n");
		return;
	}

	CL_WriteWaveOpen();
}

/*
==============
CL_WavStopRecord_f
==============
*/

void CL_WavStopRecord_f(void)
{
	if(!clc.wavefile)
	{
		Com_Printf("Not recording a wav file\n");
		return;
	}

	CL_WriteWaveClose();
	Cvar_Set("cl_waverecording", "0");
	Cvar_Set("cl_wavefilename", "");
	Cvar_Set("cl_waveoffset", "0");
	clc.waverecording = qfalse;
}


// XreaL BEGIN =======================================================

/*
===============
CL_Video_f

video
video [filename]
===============
*/
void CL_Video_f(void)
{
	char            filename[MAX_OSPATH];
	int             i, last;

	if(!clc.demoplaying)
	{
		Com_Printf("The video command can only be used when playing back demos\n");
		return;
	}

	if(Cmd_Argc() == 2)
	{
		// explicit filename
		Com_sprintf(filename, MAX_OSPATH, "videos/%s.avi", Cmd_Argv(1));
	}
	else
	{
		// scan for a free filename
		for(i = 0; i <= 9999; i++)
		{
			int             a, b, c, d;

			last = i;

			a = last / 1000;
			last -= a * 1000;
			b = last / 100;
			last -= b * 100;
			c = last / 10;
			last -= c * 10;
			d = last;

			Com_sprintf(filename, MAX_OSPATH, "videos/video%d%d%d%d.avi", a, b, c, d);

			if(!FS_FileExists(filename))
				break;			// file doesn't exist
		}

		if(i > 9999)
		{
			Com_Printf(S_COLOR_RED "ERROR: no free file names to create video\n");
			return;
		}
	}

	CL_OpenAVIForWriting(filename);
}

/*
===============
CL_StopVideo_f
===============
*/
void CL_StopVideo_f(void)
{
	CL_CloseAVI();
}



// XreaL END =========================================================

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete(void)
{
	char           *fs_write_path;
	char           *fn;

	// DHM - Nerve :: Auto-update (not finished yet)
	if(autoupdateStarted)
	{

		if(strlen(autoupdateFilename) > 4)
		{
			fs_write_path = Cvar_VariableString("fs_homepath");
			fn = FS_BuildOSPath(fs_write_path, FS_ShiftStr(AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT), autoupdateFilename);

			// will either exit with a successful process spawn, or will Com_Error ERR_DROP
			// so we need to clear the disconnected download data if needed
			if(cls.bWWWDlDisconnected)
			{
				cls.bWWWDlDisconnected = qfalse;
				CL_ClearStaticDownload();
			}
			Sys_StartProcess(fn, qtrue);
		}

		// NOTE - TTimo: that code is never supposed to be reached?

		autoupdateStarted = qfalse;

		if(!cls.bWWWDlDisconnected)
		{
			CL_Disconnect(qtrue);
		}
		// we can reset that now
		cls.bWWWDlDisconnected = qfalse;
		CL_ClearStaticDownload();

		return;
	}

	// if we downloaded files we need to restart the file system
	if(cls.downloadRestart)
	{
		cls.downloadRestart = qfalse;

		FS_Restart(clc.checksumFeed);	// We possibly downloaded a pak, restart the file system to load it

		if(!cls.bWWWDlDisconnected)
		{
			// inform the server so we get new gamestate info
			CL_AddReliableCommand("donedl");
		}
		// we can reset that now
		cls.bWWWDlDisconnected = qfalse;
		CL_ClearStaticDownload();

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// TTimo: I wonder if that happens - it should not but I suspect it could happen if a download fails in the middle or is aborted
	assert(!cls.bWWWDlDisconnected);

	// let the client game init and load data
	cls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if(cls.state != CA_LOADING)
	{
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CL_FlushMemory();

	// initialize the CGame
	cls.cgameStarted = qtrue;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void CL_BeginDownload(const char *localName, const char *remoteName)
{

	Com_DPrintf("***** CL_BeginDownload *****\n"
				"Localname: %s\n" "Remotename: %s\n" "****************************\n", localName, remoteName);

	Q_strncpyz(cls.downloadName, localName, sizeof(cls.downloadName));
	Com_sprintf(cls.downloadTempName, sizeof(cls.downloadTempName), "%s.tmp", localName);

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0;		// Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand(va("download %s", remoteName));
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void)
{
	char           *s;
	char           *remoteName, *localName;

	// We are looking to start a download here
	if(*clc.downloadList)
	{
		s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if(*s == '@')
		{
			s++;
		}
		remoteName = s;

		if((s = strchr(s, '@')) == NULL)
		{
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;
		if((s = strchr(s, '@')) != NULL)
		{
			*s++ = 0;
		}
		else
		{
			s = localName + strlen(localName);	// point at the nul byte

		}
		CL_BeginDownload(localName, remoteName);

		cls.downloadRestart = qtrue;

		// move over the rest
		memmove(clc.downloadList, s, strlen(s) + 1);

		return;
	}

	CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void)
{
#ifndef PRE_RELEASE_DEMO
	char            missingfiles[1024];
	char           *dir = FS_ShiftStr(AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT);

	// TTimo
	// init some of the www dl data
	clc.bWWWDl = qfalse;
	clc.bWWWDlAborting = qfalse;
	cls.bWWWDlDisconnected = qfalse;
	CL_ClearStaticDownload();

	if(autoupdateStarted && NET_CompareAdr(cls.autoupdateServer, clc.serverAddress))
	{
		if(strlen(cl_updatefiles->string) > 4)
		{
			Q_strncpyz(autoupdateFilename, cl_updatefiles->string, sizeof(autoupdateFilename));
			Q_strncpyz(clc.downloadList, va("@%s/%s@%s/%s", dir, cl_updatefiles->string, dir, cl_updatefiles->string),
					   MAX_INFO_STRING);
			cls.state = CA_CONNECTED;
			CL_NextDownload();
			return;
		}
	}
	else
	{
		// whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
		if(FS_ComparePaks(missingfiles, sizeof(missingfiles), qfalse))
		{
			Cvar_Set("com_missingFiles", missingfiles);
		}
		else
		{
			Cvar_Set("com_missingFiles", "");
		}

		// reset the redirect checksum tracking
		clc.redirectedList[0] = '\0';

		if(cl_allowDownload->integer && FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), qtrue))
		{
			// this gets printed to UI, i18n
			Com_Printf(CL_TranslateStringBuf("Need paks: %s\n"), clc.downloadList);

			if(*clc.downloadList)
			{
				// if autodownloading is not enabled on the server
				cls.state = CA_CONNECTED;
				CL_NextDownload();
				return;
			}
		}
	}
#endif

	CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend(void)
{
	int             port, i;
	char            info[MAX_INFO_STRING];
	char            data[MAX_INFO_STRING];
	char            pkt[1024 + 1];	// EVEN BALANCE - T.RAY
	int             pktlen;		// EVEN BALANCE - T.RAY

	// don't send anything if playing back a demo
	if(clc.demoplaying)
	{
		return;
	}

	// resend if we haven't gotten a reply yet
	if(cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING)
	{
		return;
	}

	if(cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT)
	{
		return;
	}

	clc.connectTime = cls.realtime;	// for retransmit requests
	clc.connectPacketCount++;

	switch (cls.state)
	{
		case CA_CONNECTING:
			// EVEN BALANCE - T.RAY
			strcpy(pkt, "getchallenge");
			pktlen = strlen(pkt);
			NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "%s", pkt);
			break;

		case CA_CHALLENGING:
			// sending back the challenge
			port = Cvar_VariableValue("net_qport");

			Q_strncpyz( info, Cvar_InfoString( CVAR_USERINFO ), sizeof( info ) );
			Info_SetValueForKey( info, "protocol", va("%i", com_protocol->integer ) );
			Info_SetValueForKey( info, "qport", va("%i", port ) );
			Info_SetValueForKey( info, "challenge", va("%i", clc.challenge ) );

			strcpy(data, "connect ");

			data[8] = '\"';		// NERVE - SMF - spaces in name bugfix

			for(i = 0; i < strlen(info); i++)
			{
				data[9 + i] = info[i];	// + (clc.challenge)&0x3;
			}
			data[9 + i] = '\"';	// NERVE - SMF - spaces in name bugfix
			data[10 + i] = 0;

			// EVEN BALANCE - T.RAY
			pktlen = i + 10;
			memcpy(pkt, &data[0], pktlen);

			NET_OutOfBandData(NS_CLIENT, clc.serverAddress, (byte*) pkt, pktlen);
			// the most current userinfo has been sent, so watch for any
			// newer changes to userinfo variables
			cvar_modifiedFlags &= ~CVAR_USERINFO;
			break;

		default:
			Com_Error(ERR_FATAL, "CL_CheckForResend: bad cls.state");
	}
}

/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket(netadr_t from)
{
	const char     *message;

	if(cls.state < CA_AUTHORIZING)
	{
		return;
	}

	// if not from our server, ignore it
	if(!NET_CompareAdr(from, clc.netchan.remoteAddress))
	{
		return;
	}

	// if we have received packets within three seconds, ignore (it might be a malicious spoof)
	// NOTE TTimo:
	// there used to be a  clc.lastPacketTime = cls.realtime; line in CL_PacketEvent before calling CL_ConnectionLessPacket
	// therefore .. packets never got through this check, clients never disconnected
	// switched the clc.lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
	// you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
	if(cls.realtime - clc.lastPacketTime < 3000)
	{
		return;
	}

	// if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
	if(!cls.bWWWDlDisconnected)
	{
		// drop the connection
		message = "Server disconnected for unknown reason";
		Com_Printf("%s", message);
		Cvar_Set("com_errorMessage", message);
		CL_Disconnect(qtrue);
	}
	else
	{
		CL_Disconnect(qfalse);
		Cvar_Set("ui_connecting", "1");
		Cvar_Set("ui_dl_running", "1");
	}
}


char *str_replace ( const char *string, const char *substr, const char *replacement ){
  char *tok = NULL;
  char *newstr = NULL;
  char *oldstr = NULL;
  /* if either substr or replacement is NULL, duplicate string a let caller handle it */
  if ( substr == NULL || replacement == NULL ) return strdup (string);
  newstr = strdup (string);
  while ( (tok = strstr ( newstr, substr ))){
    oldstr = newstr;
    newstr = (char*)malloc ( strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );
    /*failed to alloc mem, free old string and return NULL */
    if ( newstr == NULL ){
      free (oldstr);
      return NULL;
    }
    memcpy ( newstr, oldstr, tok - oldstr );
    memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
    memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
    memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );
    free (oldstr);
  }
  return newstr;
}

/*
===================
CL_MotdPacket
===================
*/
void CL_MotdPacket( netadr_t from, const char *info ) {
	const char *v;
	char *w;

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, cls.updateServer ) ) {
		Com_DPrintf( "MOTD packet from unexpected source\n" );
		return;
	}

	Com_DPrintf( "MOTD packet: %s\n", info );

	while ( *info != '\\' ) {
		info++;
	}

	// check challenge
	v = Info_ValueForKey( info, "challenge" );

	if ( strcmp( v, cls.updateChallenge ) ) {
		Com_DPrintf( "MOTD packet mismatched challenge: "
		             "'%s' != '%s'\n", v, cls.updateChallenge );
		return;
	}

	v = Info_ValueForKey( info, "motd" );
	w = str_replace( v, "|", "\n" );

	Q_strncpyz( cls.updateInfoString, info, sizeof( cls.updateInfoString ) );
	Cvar_Set( "cl_newsString", w );
	free(w);
}

/*
===================
CL_PrintPackets
an OOB message from server, with potential markups
print OOB are the only messages we handle markups in
[err_dialog]: used to indicate that the connection should be aborted
  no further information, just do an error diagnostic screen afterwards
[err_prot]: HACK. This is a protocol error. The client uses a custom
  protocol error message (client sided) in the diagnostic window.
  The space for the error message on the connection screen is limited
  to 256 chars.
===================
*/
void CL_PrintPacket(netadr_t from, msg_t * msg)
{
	char           *s;

	s = MSG_ReadBigString(msg);
	if(!Q_stricmpn(s, "[err_dialog]", 12))
	{
		Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
		// Cvar_Set("com_errorMessage", clc.serverMessage );
		Com_Error(ERR_DROP, "%s", clc.serverMessage);
	}
	else if(!Q_stricmpn(s, "[err_prot]", 10))
	{
		Q_strncpyz(clc.serverMessage, s + 10, sizeof(clc.serverMessage));
		// Cvar_Set("com_errorMessage", CL_TranslateStringBuf( PROTOCOL_MISMATCH_ERROR_LONG ) );
		Com_Error(ERR_DROP, "%s", CL_TranslateStringBuf(PROTOCOL_MISMATCH_ERROR_LONG));
	}
	else if(!Q_stricmpn(s, "[err_update]", 12))
	{
		Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
		Com_Error(ERR_AUTOUPDATE, "%s", clc.serverMessage);
	}
	else if(!Q_stricmpn(s, "ET://", 5))
	{							// fretn
		Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
		Cvar_Set("com_errorMessage", clc.serverMessage);
		Com_Error(ERR_DROP, "%s", clc.serverMessage);
	}
	else
	{
		Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
	}
	Com_Printf("%s", clc.serverMessage);
}

/*
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo( serverInfo_t *server, netadr_t *address ) {
	server->adr = *address;
	server->clients = 0;
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType = 0;
	server->netType = 0;
	server->allowAnonymous = 0;
}

#define MAX_SERVERSPERPACKET    256

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket( const netadr_t* from, msg_t *msg, qboolean extended ) {
	int i, count, total;
	netadr_t addresses[MAX_SERVERSPERPACKET];
	int numservers;
	byte*           buffptr;
	byte*           buffend;

	Com_Printf( "CL_ServersResponsePacket\n" );

	if ( cls.numglobalservers == -1 ) {
		// state to detect lack of servers or lack of response
		cls.numglobalservers = 0;
		cls.numGlobalServerAddresses = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr    = msg->data;
	buffend    = buffptr + msg->cursize;

	// advance to initial token
	do
	{
		if(*buffptr == '\\' || (extended && *buffptr == '/'))
			break;
		
		buffptr++;
	} while (buffptr < buffend);

	while (buffptr + 1 < buffend)
	{
		// IPv4 address
		if (*buffptr == '\\')
		{
			buffptr++;

			if (buffend - buffptr < sizeof(addresses[numservers].ip) + sizeof(addresses[numservers].port) + 1)
				break;

			for(i = 0; i < sizeof(addresses[numservers].ip); i++)
				addresses[numservers].ip[i] = *buffptr++;

			addresses[numservers].type = NA_IP;
		}
		// IPv6 address, if it's an extended response
		else if (extended && *buffptr == '/')
		{
			buffptr++;

			if (buffend - buffptr < sizeof(addresses[numservers].ip6) + sizeof(addresses[numservers].port) + 1)
				break;
			
			for(i = 0; i < sizeof(addresses[numservers].ip6); i++)
				addresses[numservers].ip6[i] = *buffptr++;
			
			addresses[numservers].type = NA_IP6;
			addresses[numservers].scope_id = from->scope_id;
		}
		else
			// syntax error!
			break;

		// parse out port
		addresses[numservers].port = ( *buffptr++ ) << 8;
		addresses[numservers].port += *buffptr++;
		addresses[numservers].port = BigShort( addresses[numservers].port );

		// syntax check
		if (*buffptr != '\\' && *buffptr != '/')
			break;
	
		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET)
			break;
	}

	count = cls.numglobalservers;

	for (i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++) {
		// build net address
		serverInfo_t *server = &cls.globalServers[count];

		CL_InitServerInfo( server, &addresses[i] );
		// advance to next slot
		count++;
	}

	// if getting the global list
	if ( count >= MAX_GLOBAL_SERVERS && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS )
	{
		// if we couldn't store the servers in the main list anymore
		for (; i < numservers && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS; i++)
		{
			// just store the addresses in an additional list
			cls.globalServerAddresses[cls.numGlobalServerAddresses++] = addresses[i];
		}
	}

	cls.numglobalservers = count;
	total = count + cls.numGlobalServerAddresses;

	Com_Printf( "%d servers parsed (total %d)\n", numservers, total );
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char    *s;
	char    *c;

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );    // skip the -1

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	c = Cmd_Argv( 0 );

	Com_DPrintf ("CL packet %s: %s\n", NET_AdrToStringwPort(from), c);

	// challenge from the server we are connecting to
	if ( !Q_stricmp( c, "challengeResponse" ) ) {
		if ( cls.state != CA_CONNECTING ) {
			Com_Printf( "Unwanted challenge response received.  Ignored.\n" );
		} else {
			// start sending challenge repsonse instead of challenge request packets
			clc.challenge = atoi( Cmd_Argv( 1 ) );
			if ( Cmd_Argc() > 2 ) {
				clc.onlyVisibleClients = atoi( Cmd_Argv( 2 ) );         // DHM - Nerve
			} else {
				clc.onlyVisibleClients = 0;
			}
			cls.state = CA_CHALLENGING;
			clc.connectPacketCount = 0;
			clc.connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.serverAddress = from;
			Com_DPrintf( "challenge: %d\n", clc.challenge );
		}
		return;
	}

	// server connection
	if ( !Q_stricmp( c, "connectResponse" ) ) {
		if ( cls.state >= CA_CONNECTED ) {
			Com_Printf( "Dup connect received.  Ignored.\n" );
			return;
		}
		if ( cls.state != CA_CHALLENGING ) {
			Com_Printf( "connectResponse packet while not connecting.  Ignored.\n" );
			return;
		}
		if ( !NET_CompareAdr( from, clc.serverAddress ) ) {
			Com_Printf( "connectResponse from a different address.  Ignored.\n" );
			Com_Printf( "%s should have been %s\n", NET_AdrToString( from ),
						NET_AdrToStringwPort( clc.serverAddress ) );
			return;
		}

		// DHM - Nerve :: If we have completed a connection to the Auto-Update server...
		if ( autoupdateChecked && NET_CompareAdr( cls.autoupdateServer, clc.serverAddress ) ) {
			// Mark the client as being in the process of getting an update
			if ( cl_updateavailable->integer ) {
				autoupdateStarted = qtrue;
			}
		}

		Netchan_Setup( NS_CLIENT, &clc.netchan, from, Cvar_VariableValue( "net_qport" ) );
		cls.state = CA_CONNECTED;
		clc.lastPacketSentTime = -9999;     // send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if ( !Q_stricmp( c, "infoResponse" ) ) {
		CL_ServerInfoPacket( from, msg );
		return;
	}

	// server responding to a get playerlist
	if ( !Q_stricmp( c, "statusResponse" ) ) {
		CL_ServerStatusResponse( from, msg );
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if ( !Q_stricmp( c, "disconnect" ) ) {
		CL_DisconnectPacket( from );
		return;
	}

	// echo request from server
	if ( !Q_stricmp( c, "echo" ) ) {
		NET_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv( 1 ) );
		return;
	}

	// cd check
	if ( !Q_stricmp( c, "keyAuthorize" ) ) {
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD from id
	if ( !Q_stricmp( c, "motd" ) ) {
		CL_MotdPacket( from, s );
		return;
	}

	// echo request from server
	if ( !Q_stricmp( c, "print" ) ) {
		CL_PrintPacket( from, msg );
		return;
	}

	// DHM - Nerve :: Auto-update server response message
	if ( !Q_stricmp( c, "updateResponse" ) ) {
		CL_UpdateInfoPacket( from );
		return;
	}
	// DHM - Nerve
#if defined (GUIDMASTER_SUPPORT)
	if (!Q_stricmp(c, "guidresponse")) {
		CL_GenGuid(msg);
	}
#endif

	// NERVE - SMF - bugfix, make this compare first n chars so it doesnt bail if token is parsed incorrectly
	// echo request from server
	if ( !Q_strncmp( c, "getserversResponse", 18 ) ) {
		CL_ServersResponsePacket( &from, msg, qfalse );
		return;
	}

	// list of servers sent back by a master server (extended)
	if ( !Q_strncmp(c, "getserversExtResponse", 21) ) {
		CL_ServersResponsePacket( &from, msg, qtrue );
		return;
	}

	Com_DPrintf( "Unknown connectionless packet command.\n" );
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent( netadr_t from, msg_t *msg ) {
	int headerBytes;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		CL_ConnectionlessPacket( from, msg );
		return;
	}

	clc.lastPacketTime = cls.realtime;

	if ( cls.state < CA_CONNECTED ) {
		return;     // can't be a valid sequenced packet
	}

	if ( msg->cursize < 4 ) {
		Com_Printf( "%s: Runt packet\n",NET_AdrToStringwPort( from ) );
		return;
	}

	//
	// packet from server
	//
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		Com_DPrintf( "%s:sequenced packet without connection\n"
			, NET_AdrToStringwPort( from ) );
		// FIXME: send a client disconnect?
		return;
	}

	if ( !CL_Netchan_Process( &clc.netchan, msg ) ) {
		return;     // out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.serverMessageSequence = LittleLong( *(int *)msg->data );

	clc.lastPacketTime = cls.realtime;
	CL_ParseServerMessage( msg );

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//

	if ( clc.demorecording && !clc.demowaiting ) {
		CL_WriteDemoMessage( msg, headerBytes );
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout(void)
{
	//
	// check timeout
	//
	if((!cl_paused->integer || !sv_paused->integer)
	   && cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC && cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000)
	{
		if(++cl.timeoutcount > 5)
		{						// timeoutcount saves debugger
			Cvar_Set("com_errorMessage", "Server connection timed out.");
			CL_Disconnect(qtrue);
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo(void)
{
	// don't add reliable commands when not yet connected
	if(cls.state < CA_CHALLENGING)
	{
		return;
	}
	// don't overflow the reliable command buffer when paused
	if(cl_paused->integer)
	{
		return;
	}
	// send a reliable userinfo update if needed
	if(cvar_modifiedFlags & CVAR_USERINFO)
	{
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString(CVAR_USERINFO)));
	}
}

/*
==================
CL_WWWDownload
==================
*/
void CL_WWWDownload(void)
{
	char           *to_ospath;
	dlStatus_t      ret;
	static qboolean bAbort = qfalse;

	if(clc.bWWWDlAborting)
	{
		if(!bAbort)
		{
			Com_DPrintf("CL_WWWDownload: WWWDlAborting\n");
			bAbort = qtrue;
		}
		return;
	}
	if(bAbort)
	{
		Com_DPrintf("CL_WWWDownload: WWWDlAborting done\n");
		bAbort = qfalse;
	}

	ret = DL_DownloadLoop();

	if(ret == DL_CONTINUE)
	{
		return;
	}

	if(ret == DL_DONE)
	{
		// taken from CL_ParseDownload
		// we work with OS paths
		clc.download = 0;
		to_ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), cls.originalDownloadName, "");
		to_ospath[strlen(to_ospath) - 1] = '\0';
		if(rename(cls.downloadTempName, to_ospath))
		{
			FS_CopyFile(cls.downloadTempName, to_ospath);
			remove(cls.downloadTempName);
		}
		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set("cl_downloadName", "");
		if(cls.bWWWDlDisconnected)
		{
			// for an auto-update in disconnected mode, we'll be spawning the setup in CL_DownloadsComplete
			if(!autoupdateStarted)
			{
				// reconnect to the server, which might send us to a new disconnected download
				Cbuf_ExecuteText(EXEC_APPEND, "reconnect\n");
			}
		}
		else
		{
			CL_AddReliableCommand("wwwdl done");
			// tracking potential web redirects leading us to wrong checksum - only works in connected mode
			if(strlen(clc.redirectedList) + strlen(cls.originalDownloadName) + 1 >= sizeof(clc.redirectedList))
			{
				// just to be safe
				Com_Printf("ERROR: redirectedList overflow (%s)\n", clc.redirectedList);
			}
			else
			{
				strcat(clc.redirectedList, "@");
				strcat(clc.redirectedList, cls.originalDownloadName);
			}
		}
	}
	else
	{
		if(cls.bWWWDlDisconnected)
		{
			// in a connected download, we'd tell the server about failure and wait for a reply
			// but in this case we can't get anything from server
			// if we just reconnect it's likely we'll get the same disconnected download message, and error out again
			// this may happen for a regular dl or an auto update
			const char     *error = va("Download failure while getting '%s'\n", cls.downloadName);	// get the msg before clearing structs

			cls.bWWWDlDisconnected = qfalse;	// need clearing structs before ERR_DROP, or it goes into endless reload
			CL_ClearStaticDownload();
			Com_Error(ERR_DROP, "%s", error);
		}
		else
		{
			// see CL_ParseDownload, same abort strategy
			Com_Printf("Download failure while getting '%s'\n", cls.downloadName);
			CL_AddReliableCommand("wwwdl fail");
			clc.bWWWDlAborting = qtrue;
		}
		return;
	}

	clc.bWWWDl = qfalse;
	CL_NextDownload();
}

/*
==================
CL_WWWBadChecksum

FS code calls this when doing FS_ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
qboolean CL_WWWBadChecksum(const char *pakname)
{
	if(strstr(clc.redirectedList, va("@%s", pakname)))
	{
		Com_Printf("WARNING: file %s obtained through download redirect has wrong checksum\n", pakname);
		Com_Printf("         this likely means the server configuration is broken\n");
		if(strlen(clc.badChecksumList) + strlen(pakname) + 1 >= sizeof(clc.badChecksumList))
		{
			Com_Printf("ERROR: badChecksumList overflowed (%s)\n", clc.badChecksumList);
			return qfalse;
		}
		strcat(clc.badChecksumList, "@");
		strcat(clc.badChecksumList, pakname);
		Com_DPrintf("bad checksums: %s\n", clc.badChecksumList);
		return qtrue;
	}
	return qfalse;
}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int msec)
{

	if(!com_cl_running->integer)
	{
		return;
	}

	if(cls.state == CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_UI) && !com_sv_running->integer)
	{
		// if disconnected, bring up the menu
		//S_StopAllSounds();
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
	}

	// if recording an avi, lock to a fixed fps
	if(CL_VideoRecording() && cl_aviFrameRate->integer && msec)
	{
		// XreaL BEGIN
		if(!CL_VideoRecording())
		{
			CL_Video_f();
		}

		// save the current screen
		if(cls.state == CA_ACTIVE || cl_forceavidemo->integer)
		{
			CL_TakeVideoFrame();

			// fixed time for next frame'
			msec = (int)ceil((1000.0f / cl_aviFrameRate->value) * com_timescale->value);
			if(msec == 0)
			{
				msec = 1;
			}
		}
		// XreaL END
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	if(cl_timegraph->integer)
	{
		SCR_DebugGraph(cls.realFrametime * 0.25, 0);
	}

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// wwwdl download may survive a server disconnect
	if((cls.state == CA_CONNECTED && clc.bWWWDl) || cls.bWWWDlDisconnected)
	{
		CL_WWWDownload();
	}

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();


	// update the screen
	SCR_UpdateScreen();

	// update the sound
	S_Update();

#ifdef USE_VOIP
	CL_CaptureVoip();
#endif	

#ifdef USE_MUMBLE
	CL_UpdateMumble();
#endif
	
	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}


//============================================================================
// Ridah, startup-caching system
typedef struct
{
	char            name[MAX_QPATH];
	int             hits;
	int             lastSetIndex;
} cacheItem_t;
typedef enum
{
	CACHE_SOUNDS,
	CACHE_MODELS,
	CACHE_IMAGES,

	CACHE_NUMGROUPS
} cacheGroup_t;
static cacheItem_t cacheGroups[CACHE_NUMGROUPS] = {
	{{'s', 'o', 'u', 'n', 'd', 0}, CACHE_SOUNDS},
	{{'m', 'o', 'd', 'e', 'l', 0}, CACHE_MODELS},
	{{'i', 'm', 'a', 'g', 'e', 0}, CACHE_IMAGES},
};

#define MAX_CACHE_ITEMS     4096
#define CACHE_HIT_RATIO     0.75	// if hit on this percentage of maps, it'll get cached

static int      cacheIndex;
static cacheItem_t cacheItems[CACHE_NUMGROUPS][MAX_CACHE_ITEMS];

static void CL_Cache_StartGather_f(void)
{
	cacheIndex = 0;
	memset(cacheItems, 0, sizeof(cacheItems));

	Cvar_Set("cl_cacheGathering", "1");
}

static void CL_Cache_UsedFile_f(void)
{
	char            groupStr[MAX_QPATH];
	char            itemStr[MAX_QPATH];
	int             i, group;
	cacheItem_t    *item;

	if(Cmd_Argc() < 2)
	{
		Com_Error(ERR_DROP, "usedfile without enough parameters\n");
		return;
	}

	strcpy(groupStr, Cmd_Argv(1));

	strcpy(itemStr, Cmd_Argv(2));
	for(i = 3; i < Cmd_Argc(); i++)
	{
		strcat(itemStr, " ");
		strcat(itemStr, Cmd_Argv(i));
	}
	Q_strlwr(itemStr);

	// find the cache group
	for(i = 0; i < CACHE_NUMGROUPS; i++)
	{
		if(!Q_strncmp(groupStr, cacheGroups[i].name, MAX_QPATH))
		{
			break;
		}
	}
	if(i == CACHE_NUMGROUPS)
	{
		Com_Error(ERR_DROP, "usedfile without a valid cache group\n");
		return;
	}

	// see if it's already there
	group = i;
	for(i = 0, item = cacheItems[group]; i < MAX_CACHE_ITEMS; i++, item++)
	{
		if(!item->name[0])
		{
			// didn't find it, so add it here
			Q_strncpyz(item->name, itemStr, MAX_QPATH);
			if(cacheIndex > 9999)
			{					// hack, but yeh
				item->hits = cacheIndex;
			}
			else
			{
				item->hits++;
			}
			item->lastSetIndex = cacheIndex;
			break;
		}
		if(item->name[0] == itemStr[0] && !Q_strncmp(item->name, itemStr, MAX_QPATH))
		{
			if(item->lastSetIndex != cacheIndex)
			{
				item->hits++;
				item->lastSetIndex = cacheIndex;
			}
			break;
		}
	}
}

static void CL_Cache_SetIndex_f(void)
{
	if(Cmd_Argc() < 2)
	{
		Com_Error(ERR_DROP, "setindex needs an index\n");
		return;
	}

	cacheIndex = atoi(Cmd_Argv(1));
}

static void CL_Cache_MapChange_f(void)
{
	cacheIndex++;
}

static void CL_Cache_EndGather_f(void)
{
	// save the frequently used files to the cache list file
	int             i, j, handle, cachePass;
	char            filename[MAX_QPATH];

	cachePass = (int)floor((float)cacheIndex * CACHE_HIT_RATIO);

	for(i = 0; i < CACHE_NUMGROUPS; i++)
	{
		Q_strncpyz(filename, cacheGroups[i].name, MAX_QPATH);
		Q_strcat(filename, MAX_QPATH, ".cache");

		handle = FS_FOpenFileWrite(filename);

		for(j = 0; j < MAX_CACHE_ITEMS; j++)
		{
			// if it's a valid filename, and it's been hit enough times, cache it
			if(cacheItems[i][j].hits >= cachePass && strstr(cacheItems[i][j].name, "/"))
			{
				FS_Write(cacheItems[i][j].name, strlen(cacheItems[i][j].name), handle);
				FS_Write("\n", 1, handle);
			}
		}

		FS_FCloseFile(handle);
	}

	Cvar_Set("cl_cacheGathering", "0");
}

// done.
//============================================================================

/*
================
CL_SetRecommended_f
================
*/
void CL_SetRecommended_f(void)
{
	Com_SetRecommended();
}

/*
================
CL_RefPrintf

DLL glue
================
*/
void QDECL CL_RefPrintf(int print_level, const char *fmt, ...)
{
	va_list         argptr;
	char            msg[MAXPRINTMSG];

	va_start(argptr, fmt);
	idStr::vsnPrintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if(print_level == PRINT_ALL)
	{
		Com_Printf("%s", msg);
	}
	else if(print_level == PRINT_WARNING)
	{
		Com_Printf(S_COLOR_YELLOW "%s", msg);	// yellow
	}
	else if(print_level == PRINT_DEVELOPER)
	{
		Com_DPrintf(S_COLOR_RED "%s", msg);	// red
	}
}

/*
============
CL_InitRenderer
============
*/
qboolean CL_InitRenderer(void)
{
	fileHandle_t f;
	// this sets up the renderer and calls R_Init
	if( !re.BeginRegistration(&cls.glconfig, &cls.glconfig2) ) {
		return qfalse;
	}

	// load character sets
	cls.charSetShader = re.RegisterShader("gfx/2d/consolechars");

	cls.useLegacyConsoleFont = qtrue;

    // Register console font specified by cl_consoleFont, if any
    // filehandle is unused but forces FS_FOpenFileRead() to heed purecheck because it does not when filehandle is NULL 
    if ( cl_consoleFont->string[0] ) {
		if( FS_FOpenFileByMode( cl_consoleFont->string, &f, FS_READ ) >= 0 )  {
			re.RegisterFont( cl_consoleFont->string, cl_consoleFontSize->integer, &cls.consoleFont);
			cls.useLegacyConsoleFont = qfalse;
		}
		FS_FCloseFile( f );
	}

	cls.whiteShader = re.RegisterShader("white");

// JPW NERVE

	cls.consoleShader = re.RegisterShader( "console-16bit" ); // JPW NERVE shader works with 16bit
	cls.consoleShader2 = re.RegisterShader( "console2-16bit" ); // JPW NERVE same

	g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
	g_consoleField.widthInChars = g_console_field_width;

#if defined (USE_HTTP)
	HTTP_PostUrl( "http://localhost/user/log", 0, 0, 
				"message="
				"[%s] %s(%s)\n"
				, Cvar_VariableString( "sys_osstring" )
				, cls.glconfig.renderer_string
				, cls.glconfig.version_string );
#endif
	return qtrue;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers(void)
{
	if(!com_cl_running)
	{
		return;
	}

	if(!com_cl_running->integer)
	{
		return;
	}

	if( rendererLib == NULL )
	{
		// initialize the renderer interface
		char renderers[MAX_QPATH];
		char *from, *to;
		
		Q_strncpyz( renderers, cl_renderer->string, sizeof(renderers) );
		from = renderers;
		while( from ) {
			to = strchr(from, ',');
			if( to )
				*to++ = '\0';
			CL_InitRef( from );
			if( CL_InitRenderer() ) {
				cls.rendererStarted = qtrue;
				break;
			}
			CL_ShutdownRef();
			from = to;
		}
	} else if(!cls.rendererStarted) {
		if( CL_InitRenderer() )
			cls.rendererStarted = qtrue;
	}

	if(!cls.soundStarted)
	{
		cls.soundStarted = qtrue;
		S_Init();
	}

	if(!cls.soundRegistered)
	{
		cls.soundRegistered = qtrue;
		S_BeginRegistration();
	}

	if(!cls.uiStarted)
	{
		cls.uiStarted = qtrue;
		CL_InitUI();
	}
}

// DHM - Nerve
void CL_CheckAutoUpdate( void ) {
#ifndef PRE_RELEASE_DEMO

	if ( !cl_autoupdate->integer ) {
		return;
	}

	// Only check once per session
	if ( autoupdateChecked ) {
		return;
	}

	srand( Com_Milliseconds() );

	// Resolve update server
	if ( !NET_StringToAdr( cls.autoupdateServerNames[0], &cls.autoupdateServer, NA_IP  ) ) {
		Com_DPrintf( "Failed to resolve any Auto-update servers.\n" );

		cls.autoUpdateServerChecked[0] = qtrue;

		autoupdateChecked = qtrue;
		return;
	}

	cls.autoupdatServerIndex = 0;

	cls.autoupdatServerFirstIndex = cls.autoupdatServerIndex;

	cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = qtrue;

	cls.autoupdateServer.port = BigShort( PORT_SERVER );
	Com_DPrintf( "autoupdate server at: %i.%i.%i.%i:%i\n", cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
				 cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
				 BigShort( cls.autoupdateServer.port ) );

	NET_OutOfBandPrint( NS_CLIENT, cls.autoupdateServer, "getUpdateInfo \"%s\" \"%s\"\n", Q3_VERSION, ARCH_STRING );

#endif // !PRE_RELEASE_DEMO

	CL_RequestMotd();

	autoupdateChecked = qtrue;
}

qboolean CL_NextUpdateServer( void ) {
	char        *servername;

#ifdef PRE_RELEASE_DEMO
	return qfalse;
#endif // PRE_RELEASE_DEMO

	if ( !cl_autoupdate->integer ) {
		return qfalse;
	}

#if 0 //def _DEBUG
	Com_Printf( S_COLOR_MAGENTA "Autoupdate hardcoded OFF in debug build\n" );
	return qfalse;
#endif

	while ( cls.autoUpdateServerChecked[cls.autoupdatServerFirstIndex] ) {
		cls.autoupdatServerIndex++;

		if ( cls.autoupdatServerIndex > MAX_AUTOUPDATE_SERVERS ) {
			cls.autoupdatServerIndex = 0;
		}

		if ( cls.autoupdatServerIndex == cls.autoupdatServerFirstIndex ) {
			// went through all of them already
			return qfalse;
		}
	}

	servername = cls.autoupdateServerNames[cls.autoupdatServerIndex];

	Com_DPrintf( "Resolving AutoUpdate Server... " );
	if ( !NET_StringToAdr( servername, &cls.autoupdateServer, NA_IP ) ) {
		Com_DPrintf( "Couldn't resolve address, trying next one..." );

		cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = qtrue;

		return CL_NextUpdateServer();
	}

	cls.autoUpdateServerChecked[cls.autoupdatServerIndex] = qtrue;

	cls.autoupdateServer.port = BigShort( PORT_SERVER );
	Com_DPrintf( "%i.%i.%i.%i:%i\n", cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
				 cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
				 BigShort( cls.autoupdateServer.port ) );

	return qtrue;
}

void CL_GetAutoUpdate(void)
{

	// Don't try and get an update if we haven't checked for one
	if(!autoupdateChecked)
	{
		return;
	}

	// Make sure there's a valid update file to request
	if(strlen(cl_updatefiles->string) < 5)
	{
		return;
	}

	Com_DPrintf("Connecting to auto-update server...\n");

	S_StopAllSounds();			// NERVE - SMF

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// toggle on all the download related cvars
	Cvar_Set("cl_allowDownload", "1");	// general flag
	Cvar_Set("cl_wwwDownload", "1");	// ftp/http support

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	if(com_sv_running->integer)
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	CL_Disconnect(qtrue);
	Con_Close();

	Q_strncpyz(cls.servername, "Auto-Updater", sizeof(cls.servername));

	if(cls.autoupdateServer.type == NA_BAD)
	{
		Com_Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		Cvar_Set("ui_connecting", "0");
		return;
	}

	// Copy auto-update server address to Server connect address
	memcpy(&clc.serverAddress, &cls.autoupdateServer, sizeof(netadr_t));

	Com_DPrintf("%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
				clc.serverAddress.ip[0], clc.serverAddress.ip[1],
				clc.serverAddress.ip[2], clc.serverAddress.ip[3], BigShort(clc.serverAddress.port));

	cls.state = CA_CONNECTING;

	cls.keyCatchers = 0;
	clc.connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", "Auto-Updater");
}

// DHM - Nerve

/*
============
CL_RefMalloc
============
*/
#ifdef ZONE_DEBUG
void           *CL_RefMallocDebug(int size, char *label, char *file, int line)
{
	return Z_TagMallocDebug(size, TAG_RENDERER, label, file, line);
}
#else
void           *CL_RefMalloc(int size)
{
	return Z_TagMalloc(size, TAG_RENDERER);
}
#endif

/*
============
CL_RefTagFree
============
*/
void CL_RefTagFree(void)
{
	Z_FreeTags(TAG_RENDERER);
	return;
}

int CL_ScaledMilliseconds(void)
{
	return Sys_Milliseconds() * com_timescale->value;
}

#if defined(REF_HARD_LINKED)
extern refexport_t* GetRefAPI(int apiVersion, refimport_t * rimp);
#endif

/*
============
CL_InitRef

RB: changed to load the renderer from a .dll
============
*/
void CL_InitRef(const char *renderer)
{
	refimport_t     ri;
	refexport_t    *ret;

#if !defined(REF_HARD_LINKED)
	GetRefAPI_t		GetRefAPI;
	char            dllName[MAX_OSPATH];
#endif

	Com_Printf("----- Initializing Renderer ----\n");

#if !defined(REF_HARD_LINKED)

	Com_sprintf(dllName, sizeof(dllName), DLL_PREFIX "renderer%s" ARCH_STRING DLL_EXT, renderer);

	Com_Printf("Loading \"%s\"...\n", dllName);
	if((rendererLib = Sys_LoadLibrary(dllName)) == 0)
	{
		Com_Printf("failed:\n\"%s\"\n", Sys_LibraryError());

		Com_sprintf(dllName, sizeof(dllName), DLL_PREFIX "rendererGL" ARCH_STRING DLL_EXT);
		Com_Printf("Loading \"%s\"...", dllName);
		if((rendererLib = Sys_LoadLibrary(dllName)) == 0)
		{
			Com_Error(ERR_FATAL, "failed:\n\"%s\"\n", Sys_LibraryError());
		}
	}

	Com_Printf("done\n");

	GetRefAPI = (GetRefAPI_t)Sys_LoadFunction(rendererLib, "GetRefAPI");
	if(!GetRefAPI)
	{
		Com_Error(ERR_FATAL, "Can't load symbol GetRefAPI: '%s'",  Sys_LibraryError());
	}

#endif

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	
	ri.Printf = CL_RefPrintf;
	ri.Error = Com_Error;
	
	ri.Milliseconds = CL_ScaledMilliseconds;
	ri.RealTime = Com_RealTime;

#ifdef ZONE_DEBUG
	ri.Z_MallocDebug = CL_RefMallocDebug;
#else
	ri.Z_Malloc = CL_RefMalloc;
#endif
	ri.Free = Z_Free;
	ri.Tag_Free = CL_RefTagFree;
	ri.Hunk_Clear = Hunk_ClearToMark;
#ifdef HUNK_DEBUG
	ri.Hunk_AllocDebug = Hunk_AllocDebug;
#else
	ri.Hunk_Alloc = Hunk_Alloc;
#endif
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;

	ri.CM_PointContents = CM_PointContents;
	ri.CM_DrawDebugSurface = CM_DrawDebugSurface;
	
	ri.FS_ReadFile = FS_ReadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_WriteFile = FS_WriteFile;
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_ListFiles = FS_ListFiles;
	ri.FS_FileIsInPAK = FS_FileIsInPAK;
	ri.FS_FileExists = FS_FileExists;
	
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_CheckRange = Cvar_CheckRange;

	ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;

	// cinematic stuff

	ri.CIN_UploadCinematic = CIN_UploadCinematic;
	ri.CIN_PlayCinematic = CIN_PlayCinematic;
	ri.CIN_RunCinematic = CIN_RunCinematic;

	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

	// XreaL BEGIN
	//ri.Sys_GetSystemHandles = Sys_GetSystemHandles;

	ri.CL_VideoRecording = CL_VideoRecording;
	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;
	// XreaL END

	ri.IN_Init = IN_Init;
	ri.IN_Shutdown = IN_Shutdown;
	ri.IN_Restart = IN_Restart;

	ri.FS_FCloseFile = FS_FCloseFile;

	// Dushan
	ri.ftol = Q_ftol;
	ri.Con_GetText = Con_GetText;
#if defined (USE_HTTP)
	ri.HTTP_PostBug = HTTP_PostBug;
#endif

	ri.Sys_GLimpSafeInit = Sys_GLimpSafeInit;
	ri.Sys_GLimpInit = Sys_GLimpInit;

	Com_Printf("Calling GetRefAPI...\n");
	ret = GetRefAPI(REF_API_VERSION, &ri);

	Com_Printf("-------------------------------\n");

	if(!ret)
	{
		Com_Error(ERR_FATAL, "Couldn't initialize refresh");
	}

	re = *ret;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");
}

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef(void)
{
	if(!re.Shutdown)
	{
		return;
	}
	re.Shutdown(qtrue);
	memset(&re, 0, sizeof(re));

	if(rendererLib)
	{
		Sys_UnloadDll(rendererLib);
		rendererLib = NULL;
	}
}



// RF, trap manual client damage commands so users can't issue them manually
void CL_ClientDamageCommand(void)
{
	// do nothing
}

#if !defined( __MACOS__ )
void CL_SaveTranslations_f(void)
{
	CL_SaveTransTable("scripts/translation.lang", qfalse);
}

void CL_SaveNewTranslations_f(void)
{
	char            fileName[512];

	if(Cmd_Argc() != 2)
	{
		Com_Printf("usage: SaveNewTranslations <filename>\n");
		return;
	}

	strcpy(fileName, va("translations/%s.lang", Cmd_Argv(1)));

	CL_SaveTransTable(fileName, qtrue);
}

void CL_LoadTranslations_f(void)
{
	CL_ReloadTranslation();
}

// -NERVE - SMF
#endif

//===========================================================================================

static void CL_UpdateGUID(void)
{
	fileHandle_t f;
	int          len;

	len = FS_SV_FOpenFileRead(BASEGAME "/" GUIDKEY_FILE, &f);
	FS_FCloseFile(f);

	if (len < GUIDKEY_SIZE)
	{
#if defined (GUIDMASTER_SUPPORT)
		CL_GetAndRegGUID();
#else
		Com_Printf("^3Support for GUID server disabled. Can't create guid!\n");
#endif
	}
	else
	{
		char *guid = Com_MD5FileOWCompat(GUIDKEY_FILE);
		if (guid)
		{
			Cvar_Set("cl_guid", guid);
		}
	}
}


/*
============
CL_GenerateGUIDKey
============
*/
static void CL_GenerateGUIDKey(char *guid) {
    fileHandle_t  f;

	f = FS_SV_FOpenFileWrite(BASEGAME "/" GUIDKEY_FILE);
	if (!f)
	{
		Com_Printf(S_COLOR_CYAN "ERROR: could not open %s file for write\n",
		           GUIDKEY_FILE);
		return;
	}
	FS_Write(guid, strlen(guid), f);
	FS_FCloseFile(f);
	Com_Printf(S_COLOR_CYAN "New OWKey %s file generated\n", GUIDKEY_FILE);
}

/*
===============
CL_GenerateRSAKey

Check if the RSAKey file contains a valid RSA keypair
If not then generate a new keypair
===============
*/
static void CL_GenerateRSAKey(void)
{
	int				len;
	fileHandle_t	f;
	void			*buf;
	struct			nettle_buffer key_buffer;
	int				key_buffer_len = 0;

	rsa_public_key_init( &public_key );
	rsa_private_key_init( &private_key );

	len = FS_SV_FOpenFileRead( RSAKEY_FILE, &f );
	if ( !f || len < 1 )
	{
		Com_Printf( "OpenWolf RSA public-key file not found, regenerating\n" );
		goto new_key;
	}
	buf = Z_TagMalloc( len, TAG_CRYPTO );
	FS_Read( buf, len, f );
	FS_FCloseFile( f );

	if ( !rsa_keypair_from_sexp( &public_key, &private_key, 0 , len, (const uint8_t*)buf ) )
	{
		Com_Printf( "Invalid RSA keypair in RSAKey, regenerating\n" );
		Z_Free( buf );
		goto new_key;
	}

	Z_Free( buf );
	Com_Printf( "OpenWolf RSA public-key found.\n" );
	return;

new_key:
	mpz_set_ui(public_key.e, RSA_PUBLIC_EXPONENT);
	if ( !rsa_generate_keypair( &public_key, &private_key, NULL, qnettle_random, NULL, NULL, RSA_KEY_LENGTH, 0 ) )
		goto keygen_error;

	qnettle_buffer_init(&key_buffer, &key_buffer_len);
	if ( !rsa_keypair_to_sexp( &key_buffer, NULL, &public_key, &private_key ) )
		goto keygen_error;

	f = FS_SV_FOpenFileWrite( RSAKEY_FILE );
	if( !f )
	{
		Com_Printf( "OpenWolf RSA public-key could not open %s for write, RSA support will be disabled\n", RSAKEY_FILE );
		Cvar_Set( "cl_pubkeyID", "0" );
		Crypto_Shutdown();
		return;
	}

	FS_Write( key_buffer.contents, key_buffer.size, f );
	nettle_buffer_clear( &key_buffer );
	FS_FCloseFile( f );
	Com_Printf( "OpenWolf RSA public-key generated\n" );
	return;

keygen_error:
	Com_Printf( "Error generating RSA keypair, RSA support will be disabled\n" );
	Cvar_Set( "cl_pubkeyID", "0" );
	Crypto_Shutdown();
} 

void CL_GenGuid(msg_t *msg)
{
	char *s;
	char *guid;
	Com_Printf("CL_GenGuidResponse\n");

	MSG_BeginReadingOOB(msg);
	MSG_ReadLong(msg);

	s    = MSG_ReadStringLine(msg);
	guid = Cmd_Argv(1);
	CL_GenerateGUIDKey(guid);
}

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	Com_Printf("----- Client Initialization -----\n");

	Con_Init();

	CL_ClearState();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput();

	CL_OW_IRCSetup();

	//
	// register our variables
	//
	cl_renderer = Cvar_Get("cl_renderer", "GL3,GL", CVAR_ARCHIVE|CVAR_LATCH, "test");

	cl_noprint = Cvar_Get("cl_noprint", "0", 0, "test");
	cl_motd = Cvar_Get("cl_motd", "1", 0, "test");
	cl_autoupdate = Cvar_Get("cl_autoupdate", "1", CVAR_ARCHIVE, "test");

	cl_timeout = Cvar_Get("cl_timeout", "200", 0, "test");

	cl_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP, "test");

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP, "test");
	cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP, "test");
	cl_shownuments = Cvar_Get("cl_shownuments", "0", CVAR_TEMP, "test");
	cl_visibleClients = Cvar_Get("cl_visibleClients", "0", CVAR_TEMP, "test");
	cl_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0, "test");
	cl_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP, "test");
	cl_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP, "test");
	cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP, "test");
	rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP, "test");
	cl_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP, "test");
	cl_autorecord = Cvar_Get("cl_autorecord", "0", CVAR_TEMP, "test");

	cl_timedemo = Cvar_Get("timedemo", "0", 0, "test");
	cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0, "test");
	cl_aviFrameRate = Cvar_Get("cl_aviFrameRate", "25", CVAR_ARCHIVE, "test");
	cl_aviMotionJpeg = Cvar_Get("cl_aviMotionJpeg", "1", CVAR_ARCHIVE, "test");

	rconAddress = Cvar_Get("rconAddress", "", 0, "test");

	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", CVAR_ARCHIVE, "test");
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "140", CVAR_ARCHIVE, "test");
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0, "test");

	cl_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE, "test");
	cl_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE, "test");

	cl_run = Cvar_Get("cl_run", "1", CVAR_ARCHIVE, "test");
	cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE, "test");
	cl_mouseAccel = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE, "test");
	cl_freelook = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE, "test");

	cl_xbox360ControllerAvailable = Cvar_Get("in_xbox360ControllerAvailable", "0", CVAR_ROM, "test");

	// 0: legacy mouse acceleration 
	// 1: new implementation 

	cl_mouseAccelStyle = Cvar_Get( "cl_mouseAccelStyle", "0", CVAR_ARCHIVE, "test" ); 
	// offset for the power function (for style 1, ignored otherwise) 
	// this should be set to the max rate value 
	cl_mouseAccelOffset = Cvar_Get( "cl_mouseAccelOffset", "5", CVAR_ARCHIVE, "test" ); 


	cl_showMouseRate = Cvar_Get("cl_showmouserate", "0", 0, "test");

	cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE, "test");
	cl_wwwDownload = Cvar_Get("cl_wwwDownload", "1", CVAR_USERINFO | CVAR_ARCHIVE, "test");

	cl_profile = Cvar_Get("cl_profile", "", CVAR_ROM, "test");
	cl_defaultProfile = Cvar_Get("cl_defaultProfile", "", CVAR_ROM, "test");

#if defined (USE_HTTP)
	cl_authserver = Cvar_Get( "cl_authserver", "localhost", CVAR_INIT, "test" );
#endif

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// -NERVE - SMF - disabled autoswitch by default
	Cvar_Get("cg_autoswitch", "0", CVAR_ARCHIVE, "test");

	// Rafael - particle switch
	Cvar_Get("cg_wolfparticles", "1", CVAR_ARCHIVE, "test");
	// done

	cl_conXOffset = Cvar_Get("cl_conXOffset", "3", 0, "test");
	cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE, "test");

	cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0, "test");

	// RF
	cl_recoilPitch = Cvar_Get("cg_recoilPitch", "0", CVAR_ROM, "test");

	cl_bypassMouseInput = Cvar_Get("cl_bypassMouseInput", "0", 0, "test");	//CVAR_ROM );          // NERVE - SMF

	cl_doubletapdelay = Cvar_Get("cl_doubletapdelay", "100", CVAR_ARCHIVE, "test");	// Arnout: double tap

#if defined (USE_HTTP)
	// Initialize ui_logged_in to -1(not logged in)
	// -2 - login in progress
	// 0 - invalid login
	// 1 - logged in
	Cvar_Get ("ui_logged_in", "-1", CVAR_ROM, "test");
	//force ui_logged in to -1 so we can't reset it on the command line
	Cvar_Set( "ui_logged_in", "-1" );
#endif

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE, "test");
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE, "test");
	m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE, "test");
	m_side = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE, "test");
	m_filter = Cvar_Get("m_filter", "0", CVAR_ARCHIVE, "test");

	j_pitch = Cvar_Get ("j_pitch", "0.022", CVAR_ARCHIVE, "test");
	j_yaw = Cvar_Get ("j_yaw", "-0.022", CVAR_ARCHIVE, "test");
	j_forward = Cvar_Get ("j_forward", "-0.25", CVAR_ARCHIVE, "test");
	j_side = Cvar_Get ("j_side", "0.25", CVAR_ARCHIVE, "test");
	j_up = Cvar_Get ("j_up", "1", CVAR_ARCHIVE, "test");
	j_pitch_axis = Cvar_Get ("j_pitch_axis", "3", CVAR_ARCHIVE, "test");
	j_yaw_axis = Cvar_Get ("j_yaw_axis", "4", CVAR_ARCHIVE, "test");
	j_forward_axis = Cvar_Get ("j_forward_axis", "1", CVAR_ARCHIVE, "test");
	j_side_axis = Cvar_Get ("j_side_axis", "0", CVAR_ARCHIVE, "test");
	j_up_axis = Cvar_Get ("j_up_axis", "2", CVAR_ARCHIVE, "test");

	Cvar_CheckRange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_forward_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_side_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	Cvar_CheckRange(j_up_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);

	cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM, "test");

	// ~ and `, as keys and characters
	cl_consoleKeys = Cvar_Get("cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE, "test");

	cl_consoleFont = Cvar_Get ("cl_consoleFont", "", CVAR_ARCHIVE | CVAR_LATCH, "test");
	cl_consoleFontSize = Cvar_Get ("cl_consoleFontSize", "16", CVAR_ARCHIVE | CVAR_LATCH, "test");
	cl_consoleFontKerning = Cvar_Get ("cl_consoleFontKerning", "0", CVAR_ARCHIVE, "test");
	cl_consolePrompt = Cvar_Get ("cl_consolePrompt", "^3->", CVAR_ARCHIVE, "test");

	cl_gamename = Cvar_Get("cl_gamename", GAMENAME_FOR_MASTER, CVAR_TEMP, "test");
	cl_altTab = Cvar_Get ("cl_altTab", "1", CVAR_ARCHIVE, "test");

	//bani - make these cvars visible to cgame
	cl_demorecording = Cvar_Get("cl_demorecording", "0", CVAR_ROM, "test");
	cl_demofilename = Cvar_Get("cl_demofilename", "", CVAR_ROM, "test");
	cl_demooffset = Cvar_Get("cl_demooffset", "0", CVAR_ROM, "test");
	cl_waverecording = Cvar_Get("cl_waverecording", "0", CVAR_ROM, "test");
	cl_wavefilename = Cvar_Get("cl_wavefilename", "", CVAR_ROM, "test");
	cl_waveoffset = Cvar_Get("cl_waveoffset", "0", CVAR_ROM, "test");

	//bani
	cl_packetloss = Cvar_Get("cl_packetloss", "0", CVAR_CHEAT, "test");
	cl_packetdelay = Cvar_Get("cl_packetdelay", "0", CVAR_CHEAT, "test");

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE, "test");
#if 0 // Leftover from RTCW singleplayer days. All this is set in the gamecode now.
	// NERVE - SMF
	Cvar_Get("cg_drawCompass", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawNotifyText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_quickMessageAlt", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_popupLimboMenu", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_descriptiveText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawTeamOverlay", "2", CVAR_ARCHIVE);
//  Cvar_Get( "cg_uselessNostalgia", "0", CVAR_ARCHIVE ); // JPW NERVE
	Cvar_Get("cg_drawGun", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_cursorHints", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_voiceSpriteTime", "6000", CVAR_ARCHIVE);
//  Cvar_Get( "cg_teamChatsOnly", "0", CVAR_ARCHIVE );
//  Cvar_Get( "cg_noVoiceChats", "0", CVAR_ARCHIVE );
//  Cvar_Get( "cg_noVoiceText", "0", CVAR_ARCHIVE );
	Cvar_Get("cg_crosshairSize", "48", CVAR_ARCHIVE);
	Cvar_Get("cg_drawCrosshair", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomDefaultSniper", "20", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomstepsniper", "2", CVAR_ARCHIVE);

//  Cvar_Get( "mp_playerType", "0", 0 );
//  Cvar_Get( "mp_currentPlayerType", "0", 0 );
//  Cvar_Get( "mp_weapon", "0", 0 );
//  Cvar_Get( "mp_team", "0", 0 );
//  Cvar_Get( "mp_currentTeam", "0", 0 );
	// -NERVE - SMF
#endif
	// userinfo
	Cvar_Get("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE, "test");
	Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE, "test");	// Dushan - changed from 5000
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE, "test");
//  Cvar_Get ("model", "american", CVAR_USERINFO | CVAR_ARCHIVE );  // temp until we have an skeletal american model
//  Arnout - no need // Cvar_Get ("model", "multi", CVAR_USERINFO | CVAR_ARCHIVE );
//  Arnout - no need // Cvar_Get ("head", "default", CVAR_USERINFO | CVAR_ARCHIVE );
//  Arnout - no need // Cvar_Get ("color", "4", CVAR_USERINFO | CVAR_ARCHIVE );
//  Arnout - no need // Cvar_Get ("handicap", "0", CVAR_USERINFO | CVAR_ARCHIVE );
//  Cvar_Get ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE, "test");

	cl_pubkeyID = Cvar_Get ("cl_pubkeyID", "1", CVAR_ARCHIVE | CVAR_USERINFO, "test"); 

	Cvar_Get("password", "", CVAR_USERINFO, "test");
	Cvar_Get("cg_predictItems", "1", CVAR_ARCHIVE, "test");

#ifdef USE_MUMBLE
	cl_useMumble = Cvar_Get ("cl_useMumble", "0", CVAR_ARCHIVE | CVAR_LATCH, "test");
	cl_mumbleScale = Cvar_Get ("cl_mumbleScale", "0.0254", CVAR_ARCHIVE, "test");
#endif

#ifdef USE_VOIP
	cl_voipSend = Cvar_Get ("cl_voipSend", "0", 0, "test");
	cl_voipSendTarget = Cvar_Get ("cl_voipSendTarget", "all", 0, "test");
	cl_voipGainDuringCapture = Cvar_Get ("cl_voipGainDuringCapture", "0.2", CVAR_ARCHIVE, "test");
	cl_voipCaptureMult = Cvar_Get ("cl_voipCaptureMult", "2.0", CVAR_ARCHIVE, "test");
	cl_voipUseVAD = Cvar_Get ("cl_voipUseVAD", "0", CVAR_ARCHIVE, "test");
	cl_voipVADThreshold = Cvar_Get ("cl_voipVADThreshold", "0.25", CVAR_ARCHIVE, "test");
	cl_voipShowMeter = Cvar_Get ("cl_voipShowMeter", "1", CVAR_ARCHIVE, "test");
	cl_voipShowSender = Cvar_Get ("cl_voipShowSender", "1", CVAR_ARCHIVE, "test");

	// This is a protocol version number.
	cl_voip = Cvar_Get ("cl_voip", "1", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_LATCH, "test");
	Cvar_CheckRange( cl_voip, 0, 1, qtrue );

	// If your data rate is too low, you'll get Connection Interrupted warnings
	//  when VoIP packets arrive, even if you have a broadband connection.
	//  This might work on rates lower than 25000, but for safety's sake, we'll
	//  just demand it. Who doesn't have at least a DSL line now, anyhow? If
	//  you don't, you don't need VoIP.  :)
	if ((cl_voip->integer) && (Cvar_VariableIntegerValue("rate") < 25000)) {
		Com_Printf(S_COLOR_YELLOW "Your network rate is too slow for VoIP.\n");
		Com_Printf("Set 'Data Rate' to 'LAN/Cable/xDSL' in 'Setup/System/Network' and restart.\n");
		Com_Printf("Until then, VoIP is disabled.\n");
		Cvar_Set("cl_voip", "0");
	}
#endif	
	
//----(SA) added
	Cvar_Get("cg_autoactivate", "1", CVAR_ARCHIVE, "test");
//----(SA) end

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE, "test");

	Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE, "test");

	cl_missionStats = Cvar_Get("g_missionStats", "0", CVAR_ROM, "test");
	cl_waitForFire = Cvar_Get("cl_waitForFire", "0", CVAR_ROM, "test");

	// NERVE - SMF - localization
	cl_language = Cvar_Get("cl_language", "0", CVAR_ARCHIVE, "test");
	cl_debugTranslation = Cvar_Get("cl_debugTranslation", "0", 0, "test");
	// -NERVE - SMF

	// DHM - Nerve :: Auto-update
	cl_updateavailable = Cvar_Get("cl_updateavailable", "0", CVAR_ROM, "test");
	cl_updatefiles = Cvar_Get("cl_updatefiles", "", CVAR_ROM, "test");

	Q_strncpyz(cls.autoupdateServerNames[0], AUTOUPDATE_SERVER1_NAME, MAX_QPATH);
	Q_strncpyz(cls.autoupdateServerNames[1], AUTOUPDATE_SERVER2_NAME, MAX_QPATH);
	Q_strncpyz(cls.autoupdateServerNames[2], AUTOUPDATE_SERVER3_NAME, MAX_QPATH);
	Q_strncpyz(cls.autoupdateServerNames[3], AUTOUPDATE_SERVER4_NAME, MAX_QPATH);
	Q_strncpyz(cls.autoupdateServerNames[4], AUTOUPDATE_SERVER5_NAME, MAX_QPATH);
	// DHM - Nerve

	//
	// register our commands
	//
	Cmd_AddCommand("cmd", CL_ForwardToServer_f, "^1Send a command to server remote console.");
#if defined (GUIDMASTER_SUPPORT)
	Cmd_AddCommand("getguid", CL_CmdGetGuid_f, "^1Send request for generating to GUID server.");
#endif
	Cmd_AddCommand("configstrings", CL_Configstrings_f, "^1List the current config strings in effec.t");
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f, "^1Display name, rate, number of snaps, player model, rail color, and handicap.");
	Cmd_AddCommand("snd_reload", CL_Snd_Reload_f, "^1Reload sound system.");
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f, "^1Reinitialize sound.");
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f, "^1Reinitialize video.");
	Cmd_AddCommand("ui_restart", CL_UI_Restart_f, "^11Reinitialize User Interface (UI).");
	Cmd_AddCommand("disconnect", CL_Disconnect_f, "^1Disconnects you from server (local included).");
	Cmd_AddCommand("record", CL_Record_f, "^1Records a demo.");
	Cmd_AddCommand("demo", CL_PlayDemo_f, "^1Play demo.");
	Cmd_SetCommandCompletionFunc( "demo", CL_CompleteDemoName );
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f, "^1Play the OpenWolf movie RoQ files (cinematic intro.RoQ).");
	Cmd_AddCommand("stoprecord", CL_StopRecord_f, "^1Stop recording a demo.");
	Cmd_AddCommand("connect", CL_Connect_f, "^1Connect to server (connect xx.xxx.xx.xx) or (connect serverURL.com).");
	Cmd_AddCommand("reconnect", CL_Reconnect_f, "^1Re-initialize the connection to the last server you were connected to.");
	Cmd_AddCommand("localservers", CL_LocalServers_f, "^1List servers on LAN or local sub net only.");
	Cmd_AddCommand("globalservers", CL_GlobalServers_f, "^1List public servers on the internet.");
#if defined (USE_HTTP)
	Cmd_AddCommand ("login", CL_Login_f, "^1Login to URL.");
	Cmd_AddCommand ("highscore", CL_Highscore_f, "^1Gets highscores results from the website.");
	Cmd_AddCommand ("forgotpassword", CL_ForgotPassword_f, "^1Send forgot password request to URL.");
	Cmd_AddCommand ("createcharacter", CL_CreateCharacter_f, "^1Create characer.");
	Cmd_AddCommand ("deletecharacter", CL_DeleteCharacter_f, "^1Remove created character.");
	Cmd_AddCommand ("getaccount", CL_GetAccount_f, "^1Get account from URL.");
	Cmd_AddCommand ("globalhighscores", CL_GlobalHighScores_f, "^1Get highscores from URL.");
#endif
	Cmd_AddCommand("openurl", CL_OpenUrl_f, "^1Open URL.");
	Cmd_AddCommand("rcon", CL_Rcon_f, "^1Start a remote console to a server.");
	Cmd_AddCommand("setenv", CL_Setenv_f, "^1Sets environment variables.");
	Cmd_AddCommand("ping", CL_Ping_f, "^1Manually ping a server (ping '<sv_hostname>' or by the IP address).");
	Cmd_AddCommand("serverstatus", CL_ServerStatus_f, "^1Display the current status of the connected server as well as connected users and their slot number. if you specify an IP address it will display the status of a remote server.");
	Cmd_AddCommand("showip", CL_ShowIP_f, "^1Display your current TCP/IP address.");
	Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f, "^1Variable holds a list of all the pk3 files the client found.");
	Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f, "^1Variable holds a list of all the pk3 files the client loaded data from.");

	Cmd_AddCommand ("irc_connect", CL_OW_InitIRC, "^1Connect at IRC server and chatroom.");
	Cmd_AddCommand ("irc_quit", CL_OW_IRCInitiateShutdown, "^1Disconnect from IRC server.");
	Cmd_AddCommand ("irc_say", CL_OW_IRCSay, "^1Talk at IRC chatroom.");

	// Ridah, startup-caching system
	Cmd_AddCommand("cache_startgather", CL_Cache_StartGather_f, "^1TODO - Add description.");
	Cmd_AddCommand("cache_usedfile", CL_Cache_UsedFile_f, "^1TODO - Add description.");
	Cmd_AddCommand("cache_setindex", CL_Cache_SetIndex_f, "^1TODO - Add description.");
	Cmd_AddCommand("cache_mapchange", CL_Cache_MapChange_f, "^1TODO - Add description.");
	Cmd_AddCommand("cache_endgather", CL_Cache_EndGather_f, "^1TODO - Add description.");

	Cmd_AddCommand("updatehunkusage", CL_UpdateLevelHunkUsage, "^1Use to update the hunkusage.dat file.");
	Cmd_AddCommand("updatescreen", SCR_UpdateScreen, "^1This is called every frame, and can also be called explicitly to flush text to the screen.");
	// done.

	Cmd_AddCommand("SaveTranslations", CL_SaveTranslations_f, "^1Save translations to file");
	Cmd_AddCommand("SaveNewTranslations", CL_SaveNewTranslations_f, "^1Save new translations to file");
	Cmd_AddCommand("LoadTranslations", CL_LoadTranslations_f, "^1Reload translations from file");

	Cmd_AddCommand("setRecommended", CL_SetRecommended_f, "^1Use to load the recommended hardware settings that match's current hardware.");
	Cmd_AddCommand("wav_record", CL_WavRecord_f, "^1Command to record .wav audio file.");
	Cmd_AddCommand("wav_stoprecord", CL_WavStopRecord_f, "^1Command to stop recording a .wav audio file.");
	Cmd_AddCommand("video", CL_Video_f, "^1Command for recording .AVI files from recorded demos.");
	Cmd_AddCommand("stopvideo", CL_StopVideo_f, "^1Command for stop recording .AVI files.");

	SCR_Init();

	Cbuf_Execute();

	Cvar_Set("cl_running", "1");

	Cvar_Get("cl_guid", "", CVAR_USERINFO | CVAR_ROM, "test");
	CL_UpdateGUID();

	if (cl_pubkeyID->integer) {
		CL_GenerateRSAKey();
	}

	// DHM - Nerve
	autoupdateChecked = qfalse;
	autoupdateStarted = qfalse;

	CL_InitTranslation(); // NERVE - SMF - localization

	Com_Printf("----- Client Initialization Complete -----\n");
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown(void) {
	static qboolean recursive = qfalse;

	// check whether the client is running at all.
	if(!(com_cl_running && com_cl_running->integer)) {
		return;
	}

	Com_Printf("----- CL_Shutdown -----\n");

	if(recursive) {
		printf("recursive shutdown\n");
		return;
	}
	recursive = qtrue;

	if(clc.waverecording) {							// fretn - write wav header when we quit
		CL_WavStopRecord_f();
	}

	CL_Disconnect(qtrue);

	S_Shutdown();
	DL_Shutdown();
	CL_ShutdownRef();

	CL_OW_IRCInitiateShutdown();

	CL_ShutdownUI();

	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("snd_reload");
	Cmd_RemoveCommand("snd_restart");
	Cmd_RemoveCommand("vid_restart");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("demo");
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("stoprecord");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("localservers");
	Cmd_RemoveCommand("globalservers");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("setenv");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("serverstatus");
	Cmd_RemoveCommand("showip");
	Cmd_RemoveCommand("model");

	// Ridah, startup-caching system
	Cmd_RemoveCommand("cache_startgather");
	Cmd_RemoveCommand("cache_usedfile");
	Cmd_RemoveCommand("cache_setindex");
	Cmd_RemoveCommand("cache_mapchange");
	Cmd_RemoveCommand("cache_endgather");

	Cmd_RemoveCommand("updatehunkusage");
	Cmd_RemoveCommand("wav_record");
	Cmd_RemoveCommand("wav_stoprecord");
	// done.

	CL_OW_IRCWaitShutdown( );

	Cvar_Set("cl_running", "0");

	recursive = qfalse;

	memset(&cls, 0, sizeof(cls));

	Com_Printf("-----------------------\n");
}


static void CL_SetServerInfo(serverInfo_t * server, const char *info, int ping)
{
	if(server)
	{
		if(info)
		{
			server->clients = atoi(Info_ValueForKey(info, "clients"));
			Q_strncpyz(server->hostName, Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH);
			server->load = atoi(Info_ValueForKey(info, "serverload"));
			Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH);
			server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
			Q_strncpyz(server->game, Info_ValueForKey(info, "game"), MAX_NAME_LENGTH);
			server->gameType = atoi(Info_ValueForKey(info, "gametype"));
			server->netType = atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = atoi(Info_ValueForKey(info, "maxping"));
			server->allowAnonymous = atoi(Info_ValueForKey(info, "sv_allowAnonymous"));
			server->friendlyFire = atoi(Info_ValueForKey(info, "friendlyFire"));	// NERVE - SMF
			server->maxlives = atoi(Info_ValueForKey(info, "maxlives"));	// NERVE - SMF
			server->needpass = atoi(Info_ValueForKey(info, "needpass"));	// NERVE - SMF
			server->punkbuster = atoi(Info_ValueForKey(info, "punkbuster"));	// DHM - Nerve
			Q_strncpyz(server->gameName, Info_ValueForKey(info, "gamename"), MAX_NAME_LENGTH);	// Arnout
			server->antilag = atoi(Info_ValueForKey(info, "g_antilag"));
			server->weaprestrict = atoi(Info_ValueForKey(info, "weaprestrict"));
			server->balancedteams = atoi(Info_ValueForKey(info, "balancedteams"));
		}
		server->ping = ping;
	}
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping)
{
	int             i;

	for(i = 0; i < MAX_OTHER_SERVERS; i++)
	{
		if(NET_CompareAdr(from, cls.localServers[i].adr))
		{
			CL_SetServerInfo(&cls.localServers[i], info, ping);
		}
	}

	for(i = 0; i < MAX_GLOBAL_SERVERS; i++)
	{
		if(NET_CompareAdr(from, cls.globalServers[i].adr))
		{
			CL_SetServerInfo(&cls.globalServers[i], info, ping);
		}
	}

	for(i = 0; i < MAX_OTHER_SERVERS; i++)
	{
		if(NET_CompareAdr(from, cls.favoriteServers[i].adr))
		{
			CL_SetServerInfo(&cls.favoriteServers[i], info, ping);
		}
	}

}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket( netadr_t from, msg_t *msg ) {
	int i, type;
	char info[MAX_INFO_STRING];
	char*   str;
	char    *infoString;
	int prot;
	char    *gameName;

	infoString = MSG_ReadString( msg );

	// if this isn't the correct protocol version, ignore it
	prot = atoi( Info_ValueForKey( infoString, "protocol" ) );
	if ( prot != com_protocol->integer ) {
		Com_DPrintf( "Different protocol info packet: %s\n", infoString );
		return;
	}

	// Arnout: if this isn't the correct game, ignore it
	gameName = Info_ValueForKey( infoString, "gamename" );
	if ( !gameName[0] || Q_stricmp( gameName, GAMENAME_STRING ) ) {
		Com_DPrintf( "Different game info packet: %s\n", infoString );
		return;
	}

	// iterate servers waiting for ping response
	for ( i = 0; i < MAX_PINGREQUESTS; i++ )
	{
		if ( cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr( from, cl_pinglist[i].adr ) ) {
			// calc ping time
			cl_pinglist[i].time = cls.realtime - cl_pinglist[i].start + 1;
			Com_DPrintf( "ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString( from ) );

			// save of info
			Q_strncpyz( cl_pinglist[i].info, infoString, sizeof( cl_pinglist[i].info ) );

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			switch ( from.type )
			{
			case NA_BROADCAST:
			case NA_IP:
				str = "udp";
				type = 1;
				break;
			case NA_IP6:
				type = 2;
				break;

			default:
				str = "???";
				type = 0;
				break;
			}
			Info_SetValueForKey( cl_pinglist[i].info, "nettype", va( "%d", type ) );
			CL_SetServerInfoByAddress( from, infoString, cl_pinglist[i].time );

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if ( cls.pingUpdateSource != AS_LOCAL ) {
		return;
	}

	for ( i = 0 ; i < MAX_OTHER_SERVERS ; i++ ) {
		// empty slot
		if ( cls.localServers[i].adr.port == 0 ) {
			break;
		}

		// avoid duplicate
		if ( NET_CompareAdr( from, cls.localServers[i].adr ) ) {
			return;
		}
	}

	if ( i == MAX_OTHER_SERVERS ) {
		Com_DPrintf( "MAX_OTHER_SERVERS hit, dropping infoResponse\n" );
		return;
	}

	// add this to the list
	cls.numlocalservers = i + 1;
	cls.localServers[i].adr = from;
	cls.localServers[i].clients = 0;
	cls.localServers[i].hostName[0] = '\0';
	cls.localServers[i].load = -1;
	cls.localServers[i].mapName[0] = '\0';
	cls.localServers[i].maxClients = 0;
	cls.localServers[i].maxPing = 0;
	cls.localServers[i].minPing = 0;
	cls.localServers[i].ping = -1;
	cls.localServers[i].game[0] = '\0';
	cls.localServers[i].gameType = 0;
	cls.localServers[i].netType = from.type;
	cls.localServers[i].allowAnonymous = 0;
	cls.localServers[i].friendlyFire = 0;           // NERVE - SMF
	cls.localServers[i].maxlives = 0;               // NERVE - SMF
	cls.localServers[i].needpass = 0;
	cls.localServers[i].punkbuster = 0;             // DHM - Nerve
	cls.localServers[i].antilag = 0;
	cls.localServers[i].weaprestrict = 0;
	cls.localServers[i].balancedteams = 0;
	cls.localServers[i].gameName[0] = '\0';           // Arnout

	Q_strncpyz( info, MSG_ReadString( msg ), MAX_INFO_STRING );
	if ( strlen( info ) ) {
		if ( info[strlen( info ) - 1] != '\n' ) {
			strncat( info, "\n", sizeof( info ) );
		}
		Com_Printf( "%s: %s", NET_AdrToStringwPort( from ), info );
	}
}

/*
===================
CL_UpdateInfoPacket
===================
*/
void CL_UpdateInfoPacket( netadr_t from ) {

	if ( cls.autoupdateServer.type == NA_BAD ) {
		Com_DPrintf( "CL_UpdateInfoPacket:  Auto-Updater has bad address\n" );
		return;
	}

	Com_DPrintf( "Auto-Updater resolved to %i.%i.%i.%i:%i\n",
				 cls.autoupdateServer.ip[0], cls.autoupdateServer.ip[1],
				 cls.autoupdateServer.ip[2], cls.autoupdateServer.ip[3],
				 BigShort( cls.autoupdateServer.port ) );

	if ( !NET_CompareAdr( from, cls.autoupdateServer ) ) {
		Com_DPrintf( "CL_UpdateInfoPacket:  Received packet from %i.%i.%i.%i:%i\n",
					 from.ip[0], from.ip[1], from.ip[2], from.ip[3],
					 BigShort( from.port ) );
		return;
	}

	Cvar_Set( "cl_updateavailable", Cmd_Argv( 1 ) );

	if ( !Q_stricmp( cl_updateavailable->string, "1" ) ) {
		Cvar_Set( "cl_updatefiles", Cmd_Argv( 2 ) );
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_AUTOUPDATE );
	}
}

// DHM - Nerve

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t *CL_GetServerStatus( netadr_t from ) {
	serverStatus_t *serverStatus;
	int i, oldest, oldestTime;

	serverStatus = NULL;
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			return &cl_serverStatusList[i];
		}
	}
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( cl_serverStatusList[i].retrieved ) {
			return &cl_serverStatusList[i];
		}
	}
	oldest = -1;
	oldestTime = 0;
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( oldest == -1 || cl_serverStatusList[i].startTime < oldestTime ) {
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}
	if ( oldest != -1 ) {
		return &cl_serverStatusList[oldest];
	}
	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount & ( MAX_SERVERSTATUSREQUESTS - 1 )];
}

/*
===================
CL_ServerStatus
===================
*/
int CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen ) {
	int i;
	netadr_t to;
	serverStatus_t *serverStatus;

	// if no server address then reset all server status requests
	if ( !serverAddress ) {
		for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = qtrue;
		}
		return qfalse;
	}
	// get the address
	if ( !NET_StringToAdr( serverAddress, &to, NA_UNSPEC) ) {
		return qfalse;
	}
	serverStatus = CL_GetServerStatus( to );
	// if no server status string then reset the server status request for this address
	if ( !serverStatusString ) {
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	// if this server status request has the same address
	if ( NET_CompareAdr( to, serverStatus->address ) ) {
		// if we recieved an response for this server status request
		if ( !serverStatus->pending ) {
			Q_strncpyz( serverStatusString, serverStatus->string, maxLen );
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		// resend the request regularly
		else if ( serverStatus->startTime < Sys_Milliseconds() - cl_serverStatusResendTime->integer ) {
			serverStatus->print = qfalse;
			serverStatus->pending = qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time = 0;
			serverStatus->startTime = Sys_Milliseconds();
			NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
			return qfalse;
		}
	}
	// if retrieved
	else if ( serverStatus->retrieved ) {
		serverStatus->address = to;
		serverStatus->print = qfalse;
		serverStatus->pending = qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = Sys_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
		return qfalse;
	}
	return qfalse;
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse(netadr_t from, msg_t * msg)
{
	char           *s;
	char            info[MAX_INFO_STRING];
	int             i, l, score, ping;
	int             len;
	serverStatus_t *serverStatus;

	serverStatus = NULL;
	for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if(NET_CompareAdr(from, cl_serverStatusList[i].address))
		{
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}
	// if we didn't request this server status
	if(!serverStatus)
	{
		return;
	}

	s = MSG_ReadStringLine(msg);

	len = 0;
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "%s", s);

	if(serverStatus->print)
	{
		Com_Printf("Server settings:\n");
		// print cvars
		while(*s)
		{
			for(i = 0; i < 2 && *s; i++)
			{
				if(*s == '\\')
				{
					s++;
				}
				l = 0;
				while(*s)
				{
					info[l++] = *s;
					if(l >= MAX_INFO_STRING - 1)
					{
						break;
					}
					s++;
					if(*s == '\\')
					{
						break;
					}
				}
				info[l] = '\0';
				if(i)
				{
					Com_Printf("%s\n", info);
				}
				else
				{
					Com_Printf("%-24s", info);
				}
			}
		}
	}

	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	if(serverStatus->print)
	{
		Com_Printf("\nPlayers:\n");
		Com_Printf("num: score: ping: name:\n");
	}
	for(i = 0, s = MSG_ReadStringLine(msg); *s; s = MSG_ReadStringLine(msg), i++)
	{

		len = strlen(serverStatus->string);
		Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\%s", s);

		if(serverStatus->print)
		{
			score = ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if(s)
			{
				s = strchr(s + 1, ' ');
			}
			if(s)
			{
				s++;
			}
			else
			{
				s = "unknown";
			}
			Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
		}
	}
	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	serverStatus->time = Sys_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = qfalse;
	if(serverStatus->print)
	{
		serverStatus->retrieved = qtrue;
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f( void ) {
	char        *message;
	int i, j;
	netadr_t to;

	Com_Printf( "Scanning for servers on the local network...\n" );

	// reset the list, waiting for response
	cls.numlocalservers = 0;
	cls.pingUpdateSource = AS_LOCAL;

	for ( i = 0; i < MAX_OTHER_SERVERS; i++ ) {
		qboolean b = cls.localServers[i].visible;
		Com_Memset( &cls.localServers[i], 0, sizeof( cls.localServers[i] ) );
		cls.localServers[i].visible = b;
	}
	Com_Memset( &to, 0, sizeof( to ) );

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for ( i = 0 ; i < 2 ; i++ ) {
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for ( j = 0 ; j < NUM_SERVER_PORTS ; j++ ) {
			to.port = BigShort( (short)( PORT_SERVER + j ) );

			to.type = NA_BROADCAST;
			NET_SendPacket( NS_CLIENT, strlen( message ), message, to );

			to.type = NA_MULTICAST6;
			NET_SendPacket( NS_CLIENT, strlen( message ), message, to );
		}
	}
}

/*
==================
CL_GlobalServers_f
==================
*/
void CL_GlobalServers_f( void ) {
    netadr_t    to;
    int         count, i, masterNum;
    char        command[1024], *masteraddress;
    int protocol = atoi( Cmd_Argv(2) ); // Do this right away, otherwise weird things happen when you use the ingame "Get New Servers" button.

    if ((count = Cmd_Argc()) < 3 || (masterNum = atoi(Cmd_Argv(1))) < 0 || masterNum > MAX_MASTER_SERVERS - 1)
    {
        Com_Printf("usage: globalservers <master# 0-%d> <protocol> [keywords]\n", MAX_MASTER_SERVERS - 1);
        return;
    }

    sprintf(command, "sv_master%d", masterNum + 1);
    masteraddress = Cvar_VariableString(command);

    if(!*masteraddress)
    {
        Com_Printf( "CL_GlobalServers_f: Error: No master server address given.\n");
        return;
    }

    // reset the list, waiting for response
    // -1 is used to distinguish a "no response"

    i = NET_StringToAdr(masteraddress, &to, NA_UNSPEC);

    if(!i)
    {
        Com_Printf( "CL_GlobalServers_f: Error: could not resolve address of master %s\n", masteraddress);
        return;
    }
    else if(i == 2)
        to.port = BigShort(PORT_MASTER);

    Com_Printf("Requesting servers from master %s...\n", masteraddress);

    cls.numglobalservers = -1;
    cls.pingUpdateSource = AS_GLOBAL;

    // Use the extended query for IPv6 masters
    if (to.type == NA_IP6 || to.type == NA_MULTICAST6)
    {
        int v4enabled = Cvar_VariableIntegerValue("net_enabled") & NET_ENABLEV4;

        if(v4enabled)
        {
            Com_sprintf(command, sizeof(command), "getserversExt %s %s",
                cl_gamename->string, Cmd_Argv(2));
        }
        else
        {
            Com_sprintf(command, sizeof(command), "getserversExt %s %s ipv6",
                cl_gamename->string, Cmd_Argv(2));
        }
    }
    else
        Com_sprintf(command, sizeof(command), "getservers %d", protocol);

    for (i=3; i < count; i++)
    {
        Q_strcat(command, sizeof(command), " ");
        Q_strcat(command, sizeof(command), Cmd_Argv(i));
    }

    NET_OutOfBandPrint( NS_SERVER, to, "%s", command );
}


/*
==================
CL_GetPing
==================
*/
void CL_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	const char  *str;
	int time;
	int maxPing;

	if ( n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port ) {
		// empty slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	str = NET_AdrToStringwPort( cl_pinglist[n].adr );
	Q_strncpyz( buf, str, buflen );

	time = cl_pinglist[n].time;
	if ( !time ) {
		// check for timeout
		time = cls.realtime - cl_pinglist[n].start;
		maxPing = Cvar_VariableIntegerValue( "cl_maxPing" );
		if ( maxPing < 100 ) {
			maxPing = 100;
		}
		if ( time < maxPing ) {
			// not timed out yet
			time = 0;
		}
	}

	CL_SetServerInfoByAddress( cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time );

	*pingtime = time;
}

/*
==================
CL_UpdateServerInfo
==================
*/
void CL_UpdateServerInfo(int n)
{
	if(n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
	{
		return;
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo(int n, char *buf, int buflen)
{
	if(n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
	{
		// empty slot
		if(buflen)
		{
			buf[0] = '\0';
		}
		return;
	}

	Q_strncpyz(buf, cl_pinglist[n].info, buflen);
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing(int n)
{
	if(n < 0 || n >= MAX_PINGREQUESTS)
	{
		return;
	}

	cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount(void)
{
	int             i;
	int             count;
	ping_t         *pingptr;

	count = 0;
	pingptr = cl_pinglist;

	for(i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		if(pingptr->adr.port)
		{
			count++;
		}
	}

	return (count);
}

/*
==================
CL_GetFreePing
==================
*/
ping_t         *CL_GetFreePing(void)
{
	ping_t         *pingptr;
	ping_t         *best;
	int             oldest;
	int             i;
	int             time;

	pingptr = cl_pinglist;
	for(i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// find free ping slot
		if(pingptr->adr.port)
		{
			if(!pingptr->time)
			{
				if(cls.realtime - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if(pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return (pingptr);
	}

	// use oldest entry
	pingptr = cl_pinglist;
	best = cl_pinglist;
	oldest = INT_MIN;
	for(i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// scan for oldest
		time = cls.realtime - pingptr->start;
		if(time > oldest)
		{
			oldest = time;
			best = pingptr;
		}
	}

	return (best);
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f( void ) {
	netadr_t to;
	ping_t*     pingptr;
	char*       server;
	int			argc;
	netadrtype_t	family = NA_UNSPEC;

	argc = Cmd_Argc();

	if ( argc != 2 && argc != 3 ) {
		Com_Printf( "usage: ping [-4|-6] server\n");
		return;	
	}
	
	if(argc == 2)
	server = Cmd_Argv( 1 );
	else
	{
		if(!strcmp(Cmd_Argv(1), "-4"))
			family = NA_IP;
		else if(!strcmp(Cmd_Argv(1), "-6"))
			family = NA_IP6;
		else
			Com_Printf( "warning: only -4 or -6 as address type understood.\n");

		server = Cmd_Argv(2);
	}

	Com_Memset( &to, 0, sizeof(netadr_t) );

	if ( !NET_StringToAdr( server, &to, family ) ) {
		return;
	}

	pingptr = CL_GetFreePing();

	memcpy( &pingptr->adr, &to, sizeof( netadr_t ) );
	pingptr->start = cls.realtime;
	pingptr->time  = 0;

	CL_SetServerInfoByAddress( pingptr->adr, NULL, 0 );

	NET_OutOfBandPrint( NS_CLIENT, to, "getinfo xxx" );
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
qboolean CL_UpdateVisiblePings_f(int source)
{
	int             slots, i;
	char            buff[MAX_STRING_CHARS];
	int             pingTime;
	int             max;
	qboolean        status = qfalse;

	if(source < 0 || source > AS_FAVORITES)
	{
		return qfalse;
	}

	cls.pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if(slots < MAX_PINGREQUESTS)
	{
		serverInfo_t   *server = NULL;

		max = (source == AS_GLOBAL) ? MAX_GLOBAL_SERVERS : MAX_OTHER_SERVERS;
		switch (source)
		{
			case AS_LOCAL:
				server = &cls.localServers[0];
				max = cls.numlocalservers;
				break;
			case AS_GLOBAL:
				server = &cls.globalServers[0];
				max = cls.numglobalservers;
				break;
			case AS_FAVORITES:
				server = &cls.favoriteServers[0];
				max = cls.numfavoriteservers;
				break;
		}
		for(i = 0; i < max; i++)
		{
			if(server[i].visible)
			{
				if(server[i].ping == -1)
				{
					int             j;

					if(slots >= MAX_PINGREQUESTS)
					{
						break;
					}
					for(j = 0; j < MAX_PINGREQUESTS; j++)
					{
						if(!cl_pinglist[j].adr.port)
						{
							continue;
						}
						if(NET_CompareAdr(cl_pinglist[j].adr, server[i].adr))
						{
							// already on the list
							break;
						}
					}
					if(j >= MAX_PINGREQUESTS)
					{
						status = qtrue;
						for(j = 0; j < MAX_PINGREQUESTS; j++)
						{
							if(!cl_pinglist[j].adr.port)
							{
								break;
							}
						}
						if(j < MAX_PINGREQUESTS)
						{

							memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
							cl_pinglist[j].start = cls.realtime;
							cl_pinglist[j].time = 0;
							NET_OutOfBandPrint(NS_CLIENT, cl_pinglist[j].adr, "getinfo xxx");
							slots++;
						}
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if(server[i].ping == 0)
				{
					// if we are updating global servers
					if(source == AS_GLOBAL)
					{
						//
						if(cls.numGlobalServerAddresses > 0)
						{
							// overwrite this server with one from the additional global servers
							cls.numGlobalServerAddresses--;
							CL_InitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
							// NOTE: the server[i].visible flag stays untouched
						}
					}
				}
			}
		}
	}

	if(slots)
	{
		status = qtrue;
	}
	for(i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if(!cl_pinglist[i].adr.port)
		{
			continue;
		}
		CL_GetPing(i, buff, MAX_STRING_CHARS, &pingTime);
		if(pingTime != 0)
		{
			CL_ClearPing(i);
			status = qtrue;
		}
	}

	return status;
}

/*
==================
CL_ServerStatus_f
==================
*/
void CL_ServerStatus_f( void ) {
	netadr_t	to, *toptr = NULL;
	char        *server;
	serverStatus_t *serverStatus;
	int			argc;
	netadrtype_t	family = NA_UNSPEC;

	argc = Cmd_Argc();

	if ( argc != 2 && argc != 3 )
	{
		if (cls.state != CA_ACTIVE || clc.demoplaying)
		{
			Com_Printf ("Not connected to a server.\n");
			Com_Printf( "usage: serverstatus [-4|-6] server\n");
			return;
		}

		toptr = &clc.serverAddress;
	}
	
	if(!toptr)
	{
		Com_Memset( &to, 0, sizeof( netadr_t ) );

		if(argc == 2)
			server = Cmd_Argv( 1 );
		else
		{
			if(!strcmp(Cmd_Argv(1), "-4"))
				family = NA_IP;
			else if(!strcmp(Cmd_Argv(1), "-6"))
				family = NA_IP6;
			else
				Com_Printf( "warning: only -4 or -6 as address type understood.\n");
		
			server = Cmd_Argv(2);
	}

		toptr = &to;
		if ( !NET_StringToAdr( server, toptr, family ) )
			return;
	}

	NET_OutOfBandPrint( NS_CLIENT, *toptr, "getstatus" );

	serverStatus = CL_GetServerStatus( *toptr );
	serverStatus->address = *toptr;
	serverStatus->print = qtrue;
	serverStatus->pending = qtrue;
}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void)
{
	Sys_ShowIP();
}


// NERVE - SMF
/*
=======================
CL_AddToLimboChat

=======================
*/
void CL_AddToLimboChat(const char *str)
{
	int             len;
	char           *p, *ls;
	int             lastcolor;
	int             chatHeight;
	int             i;

	chatHeight = LIMBOCHAT_HEIGHT;
	cl.limboChatPos = LIMBOCHAT_HEIGHT - 1;
	len = 0;

	// copy old strings
	for(i = cl.limboChatPos; i > 0; i--)
	{
		strcpy(cl.limboChatMsgs[i], cl.limboChatMsgs[i - 1]);
	}

	// copy new string
	p = cl.limboChatMsgs[0];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while(*str)
	{
		if(len > LIMBOCHAT_WIDTH - 1)
		{
			break;
		}

		if(Q_IsColorString(str))
		{
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if(*str == ' ')
		{
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;
}

/*
=======================
CL_GetLimboString

=======================
*/
qboolean CL_GetLimboString(int index, char *buf)
{
	if(index >= LIMBOCHAT_HEIGHT)
	{
		return qfalse;
	}

	strncpy(buf, cl.limboChatMsgs[index], 140);
	return qtrue;
}

// -NERVE - SMF



// NERVE - SMF - Localization code
#define FILE_HASH_SIZE      1024
#define MAX_VA_STRING       32000
#define MAX_TRANS_STRING    4096

typedef struct trans_s
{
	char            original[MAX_TRANS_STRING];
	char            translated[MAX_LANGUAGES][MAX_TRANS_STRING];
	struct trans_s *next;
	float           x_offset;
	float           y_offset;
	qboolean        fromFile;
} trans_t;

static trans_t *transTable[FILE_HASH_SIZE];

/*
=======================
AllocTrans
=======================
*/
static trans_t *AllocTrans(char *original, char *translated[MAX_LANGUAGES])
{
	trans_t        *t;
	int             i;

	t = (trans_t*)malloc(sizeof(trans_t));
	memset(t, 0, sizeof(trans_t));

	if(original)
	{
		strncpy(t->original, original, MAX_TRANS_STRING);
	}

	if(translated)
	{
		for(i = 0; i < MAX_LANGUAGES; i++)
			strncpy(t->translated[i], translated[i], MAX_TRANS_STRING);
	}

	return t;
}

/*
=======================
generateHashValue
=======================
*/
static long generateHashValue(const char *fname)
{
	int             i;
	long            hash;
	char            letter;

	hash = 0;
	i = 0;
	while(fname[i] != '\0')
	{
		letter = idStr::ToLower(fname[i]);
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

/*
=======================
LookupTrans
=======================
*/
static trans_t *LookupTrans(char *original, char *translated[MAX_LANGUAGES], qboolean isLoading)
{
	trans_t        *t, *newt, *prev = NULL;
	long            hash;

	hash = generateHashValue(original);

	for(t = transTable[hash]; t; prev = t, t = t->next)
	{
		if(!Q_stricmp(original, t->original))
		{
			if(isLoading)
			{
				Com_DPrintf(S_COLOR_YELLOW "WARNING: Duplicate string found: \"%s\"\n", original);
			}
			return t;
		}
	}

	newt = AllocTrans(original, translated);

	if(prev)
	{
		prev->next = newt;
	}
	else
	{
		transTable[hash] = newt;
	}

	if(cl_debugTranslation->integer >= 1 && !isLoading)
	{
		Com_Printf("Missing translation: \'%s\'\n", original);
	}

	// see if we want to save out the translation table everytime a string is added
	if(cl_debugTranslation->integer == 2 && !isLoading)
	{
		CL_SaveTransTable("scripts/translation.cfg", qfalse);
	}

	return newt;
}

/*
=======================
CL_SaveTransTable
=======================
*/
void CL_SaveTransTable(const char *fileName, qboolean newOnly)
{
	int             bucketlen, bucketnum, maxbucketlen, avebucketlen;
	int             untransnum, transnum;
	const char     *buf;
	fileHandle_t    f;
	trans_t        *t;
	int             i, j, len;

	if(cl.corruptedTranslationFile)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Cannot save corrupted translation file. Please reload first.");
		return;
	}

	FS_FOpenFileByMode(fileName, &f, FS_WRITE);

	bucketnum = 0;
	maxbucketlen = 0;
	avebucketlen = 0;
	transnum = 0;
	untransnum = 0;

	// write out version, if one
	if(strlen(cl.translationVersion))
	{
		buf = va("#version\t\t\"%s\"\n", cl.translationVersion);
	}
	else
	{
		buf = va("#version\t\t\"1.0 01/01/01\"\n");
	}

	len = strlen(buf);
	FS_Write(buf, len, f);

	// write out translated strings
	for(j = 0; j < 2; j++)
	{

		for(i = 0; i < FILE_HASH_SIZE; i++)
		{
			t = transTable[i];

			if(!t || (newOnly && t->fromFile))
			{
				continue;
			}

			bucketlen = 0;

			for(; t; t = t->next)
			{
				bucketlen++;

				if(strlen(t->translated[0]))
				{
					if(j)
					{
						continue;
					}
					transnum++;
				}
				else
				{
					if(!j)
					{
						continue;
					}
					untransnum++;
				}

				buf = va("{\n\tenglish\t\t\"%s\"\n", t->original);
				len = strlen(buf);
				FS_Write(buf, len, f);

				buf = va("\tfrench\t\t\"%s\"\n", t->translated[LANGUAGE_FRENCH]);
				len = strlen(buf);
				FS_Write(buf, len, f);

				buf = va("\tgerman\t\t\"%s\"\n", t->translated[LANGUAGE_GERMAN]);
				len = strlen(buf);
				FS_Write(buf, len, f);

				buf = va("\titalian\t\t\"%s\"\n", t->translated[LANGUAGE_ITALIAN]);
				len = strlen(buf);
				FS_Write(buf, len, f);

				buf = va("\tspanish\t\t\"%s\"\n", t->translated[LANGUAGE_SPANISH]);
				len = strlen(buf);
				FS_Write(buf, len, f);

				buf = "}\n";
				len = strlen(buf);
				FS_Write(buf, len, f);
			}

			if(bucketlen > maxbucketlen)
			{
				maxbucketlen = bucketlen;
			}

			if(bucketlen)
			{
				bucketnum++;
				avebucketlen += bucketlen;
			}
		}
	}

	Com_Printf("Saved translation table.\nTotal = %i, Translated = %i, Untranslated = %i, aveblen = %2.2f, maxblen = %i\n",
			   transnum + untransnum, transnum, untransnum, (float)avebucketlen / bucketnum, maxbucketlen);

	FS_FCloseFile(f);
}

/*
=======================
CL_CheckTranslationString

NERVE - SMF - compare formatting characters
=======================
*/
qboolean CL_CheckTranslationString( char *original, char *translated ) {
	char format_org[128], format_trans[128];
	int len, i;

	memset( format_org, 0, 128 );
	memset( format_trans, 0, 128 );

	// generate formatting string for original
	len = strlen( original );

	for ( i = 0; i < len; i++ ) {
		if ( original[i] != '%' ) {
			continue;
		}

		strcat( format_org, va( "%c%c ", '%', original[i + 1] ) );
	}

	// generate formatting string for translated
	len = strlen( translated );
	if ( !len ) {
		return qtrue;
	}

	for ( i = 0; i < len; i++ ) {
		if ( translated[i] != '%' ) {
			continue;
		}

		strcat( format_trans, va( "%c%c ", '%', translated[i + 1] ) );
	}

	// compare
	len = strlen( format_org );

	if ( len != strlen( format_trans ) ) {
		return qfalse;
	}

	for ( i = 0; i < len; i++ ) {
		if ( format_org[i] != format_trans[i] ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
=======================
CL_LoadTransTable
=======================
*/
void CL_LoadTransTable(const char *fileName)
{
	char            translated[MAX_LANGUAGES][MAX_VA_STRING];
	char            original[MAX_VA_STRING];
	qboolean        aborted;
	char           *text;
	fileHandle_t    f;
	char           *text_p;
	char           *token;
	int             len, i;
	trans_t        *t;
	int             count;

	count = 0;
	aborted = qfalse;
	cl.corruptedTranslationFile = qfalse;

	len = FS_FOpenFileByMode(fileName, &f, FS_READ);
	if(len <= 0)
	{
		return;
	}

	// Gordon: shouldn't this be a z_malloc or something?
	text = (char*)malloc(len + 1);
	if(!text)
	{
		return;
	}

	FS_Read(text, len, f);
	text[len] = 0;
	FS_FCloseFile(f);

	// parse the text
	text_p = text;

	do
	{
		token = COM_Parse(&text_p);
		if(Q_stricmp("{", token))
		{
			// parse version number
			if(!Q_stricmp("#version", token))
			{
				token = COM_Parse(&text_p);
				strcpy(cl.translationVersion, token);
				continue;
			}

			break;
		}

		// english
		token = COM_Parse(&text_p);
		if(Q_stricmp("english", token))
		{
			aborted = qtrue;
			break;
		}

		token = COM_Parse(&text_p);
		strcpy(original, token);

		if(cl_debugTranslation->integer == 3)
		{
			Com_Printf("%i Loading: \"%s\"\n", count, original);
		}

		// french
		token = COM_Parse(&text_p);
		if(Q_stricmp("french", token))
		{
			aborted = qtrue;
			break;
		}

		token = COM_Parse(&text_p);
		strcpy(translated[LANGUAGE_FRENCH], token);
		if(!CL_CheckTranslationString(original, translated[LANGUAGE_FRENCH]))
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n");
			aborted = qtrue;
			break;
		}

		// german
		token = COM_Parse(&text_p);
		if(Q_stricmp("german", token))
		{
			aborted = qtrue;
			break;
		}

		token = COM_Parse(&text_p);
		strcpy(translated[LANGUAGE_GERMAN], token);
		if(!CL_CheckTranslationString(original, translated[LANGUAGE_GERMAN]))
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n");
			aborted = qtrue;
			break;
		}

		// italian
		token = COM_Parse(&text_p);
		if(Q_stricmp("italian", token))
		{
			aborted = qtrue;
			break;
		}

		token = COM_Parse(&text_p);
		strcpy(translated[LANGUAGE_ITALIAN], token);
		if(!CL_CheckTranslationString(original, translated[LANGUAGE_ITALIAN]))
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n");
			aborted = qtrue;
			break;
		}

		// spanish
		token = COM_Parse(&text_p);
		if(Q_stricmp("spanish", token))
		{
			aborted = qtrue;
			break;
		}

		token = COM_Parse(&text_p);
		strcpy(translated[LANGUAGE_SPANISH], token);
		if(!CL_CheckTranslationString(original, translated[LANGUAGE_SPANISH]))
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n");
			aborted = qtrue;
			break;
		}

		// do lookup
		t = LookupTrans(original, NULL, qtrue);

		if(t)
		{
			t->fromFile = qtrue;

			for(i = 0; i < MAX_LANGUAGES; i++)
				strncpy(t->translated[i], translated[i], MAX_TRANS_STRING);
		}

		token = COM_Parse(&text_p);

		// set offset if we have one
		if(!Q_stricmp("offset", token))
		{
			token = COM_Parse(&text_p);
			t->x_offset = atof(token);

			token = COM_Parse(&text_p);
			t->y_offset = atof(token);

			token = COM_Parse(&text_p);
		}

		if(Q_stricmp("}", token))
		{
			aborted = qtrue;
			break;
		}

		count++;
	} while(token);

	if(aborted)
	{
		int             i, line = 1;

		for(i = 0; i < len && (text + i) < text_p; i++)
		{
			if(text[i] == '\n')
			{
				line++;
			}
		}

		Com_Printf(S_COLOR_YELLOW "WARNING: Problem loading %s on line %i\n", fileName, line);
		cl.corruptedTranslationFile = qtrue;
	}
	else
	{
		Com_Printf("Loaded %i translation strings from %s\n", count, fileName);
	}

	// cleanup
	free(text);
}

/*
=======================
CL_ReloadTranslation
=======================
*/
void CL_ReloadTranslation()
{
	char          **fileList;
	int             numFiles, i;

	for(i = 0; i < FILE_HASH_SIZE; i++)
	{
		if(transTable[i])
		{
			free(transTable[i]);
		}
	}

	memset(transTable, 0, sizeof(trans_t *) * FILE_HASH_SIZE);
	CL_LoadTransTable("scripts/translation.lang");

	fileList = FS_ListFiles("translations", ".lang", &numFiles);

	for(i = 0; i < numFiles; i++)
	{
		CL_LoadTransTable(va("translations/%s", fileList[i]));
	}
}

/*
=======================
CL_InitTranslation
=======================
*/
void CL_InitTranslation()
{
	char          **fileList;
	int             numFiles, i;

	memset(transTable, 0, sizeof(trans_t *) * FILE_HASH_SIZE);
	CL_LoadTransTable("scripts/translation.lang");

	fileList = FS_ListFiles("translations", ".lang", &numFiles);

	for(i = 0; i < numFiles; i++)
	{
		CL_LoadTransTable(va("translations/%s", fileList[i]));
	}
}

/*
=======================
CL_TranslateString
=======================
*/
void CL_TranslateString(const char *string, char *dest_buffer)
{
	int             i, count, currentLanguage;
	trans_t        *t;
	qboolean        newline = qfalse;
	char           *buf;

	buf = dest_buffer;
	currentLanguage = cl_language->integer - 1;

	// early bail if we only want english or bad language type
	if(!string)
	{
		strcpy(buf, "(null)");
		return;
	}
	else if(currentLanguage < 0 || currentLanguage >= MAX_LANGUAGES || !strlen(string))
	{
		strcpy(buf, string);
		return;
	}

	// ignore newlines
	if(string[strlen(string) - 1] == '\n')
	{
		newline = qtrue;
	}

	for(i = 0, count = 0; string[i] != '\0'; i++)
	{
		if(string[i] != '\n')
		{
			buf[count++] = string[i];
		}
	}
	buf[count] = '\0';

	t = LookupTrans(buf, NULL, qfalse);

	if(t && strlen(t->translated[currentLanguage]))
	{
		int             offset = 0;

		if(cl_debugTranslation->integer >= 1)
		{
			buf[0] = '^';
			buf[1] = '1';
			buf[2] = '[';
			offset = 3;
		}

		strcpy(buf + offset, t->translated[currentLanguage]);

		if(cl_debugTranslation->integer >= 1)
		{
			int             len2 = strlen(buf);

			buf[len2] = ']';
			buf[len2 + 1] = '^';
			buf[len2 + 2] = '7';
			buf[len2 + 3] = '\0';
		}

		if(newline)
		{
			int             len2 = strlen(buf);

			buf[len2] = '\n';
			buf[len2 + 1] = '\0';
		}
	}
	else
	{
		int             offset = 0;

		if(cl_debugTranslation->integer >= 1)
		{
			buf[0] = '^';
			buf[1] = '1';
			buf[2] = '[';
			offset = 3;
		}

		strcpy(buf + offset, string);

		if(cl_debugTranslation->integer >= 1)
		{
			int             len2 = strlen(buf);
			qboolean        addnewline = qfalse;

			if(buf[len2 - 1] == '\n')
			{
				len2--;
				addnewline = qtrue;
			}

			buf[len2] = ']';
			buf[len2 + 1] = '^';
			buf[len2 + 2] = '7';
			buf[len2 + 3] = '\0';

			if(addnewline)
			{
				buf[len2 + 3] = '\n';
				buf[len2 + 4] = '\0';
			}
		}
	}
}

/*
=======================
CL_TranslateStringBuf
TTimo - handy, stores in a static buf, converts \n to chr(13)
=======================
*/
const char     *CL_TranslateStringBuf(const char *string)
{
	char           *p;
	int             i, l;
	static char     buf[MAX_VA_STRING];

	CL_TranslateString(string, buf);
	while((p = strstr(buf, "\\n")) != NULL)
	{
		*p = '\n';
		p++;
		// Com_Memcpy(p, p+1, strlen(p) ); b0rks on win32
		l = strlen(p);
		for(i = 0; i < l; i++)
		{
			*p = *(p + 1);
			p++;
		}
	}
	return buf;
}

/*
=======================
CL_OpenURLForCvar
=======================
*/
void CL_OpenURL(const char *url)
{
	if(!url || !strlen(url))
	{
	  Com_Printf("%s", CL_TranslateStringBuf("invalid/empty URL\n"));
		return;
	}
	Sys_OpenURL(url, qtrue);
}

// Gordon: TEST TEST TEST
/*
==================
BotImport_DrawPolygon
==================
*/
void BotImport_DrawPolygon(int color, int numpoints, float *points)
{
	re.DrawDebugPolygon(color, numpoints, points);
}
