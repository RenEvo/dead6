/*************************************************************************
 Crytek Source File.
 Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
 $Id$
 $DateTime$
 Description:  Network services interface
 -------------------------------------------------------------------------
 History:
 - 11/01/2006   : Dima Kukushkin, Created
 
*************************************************************************/


#ifndef __INETWORKSERVICE_H__
#define __INETWORKSERVICE_H__

#pragma once

struct IServerBrowser;
struct IServerListener;
struct INetworkChat;
struct IChatListener;
struct ITestInterface;
struct IServerReport;
struct IServerReportListener;
struct IStatsTrack;
struct IStatsTrackListener;
struct INatNeg;
struct INatNegListener;
struct IFileDownloader;
struct SFileDownloadParameters;
struct IPatchCheck;
struct INetworkProfile;
struct INetworkProfileListener;
struct INetNub;

struct IContextViewExtension;
class  CContextView;

struct IContextViewExtensionAdder
{
  virtual void  AddExtension(IContextViewExtension* ) = 0;
};

enum ENetworkServiceState
{
  // service is opened, but has not finished initializing
  eNSS_Initializing,
  // service is initialized ok
  eNSS_Ok,
  // service has failed initializing in some way
  eNSS_Failed,
  // service has been closed
  eNSS_Closed
};

struct INetworkService : public CMultiThreadRefCount
{
  virtual ENetworkServiceState GetState() = 0;
  virtual void              Close() = 0;
  virtual void              CreateContextViewExtensions(bool server, IContextViewExtensionAdder* adder) = 0;
  virtual IServerBrowser*   GetServerBrowser() = 0;
  virtual INetworkChat*     GetNetworkChat() = 0;
  virtual IServerReport*    GetServerReport() = 0;
  virtual IStatsTrack*      GetStatsTrack(IStatsTrackListener*const) = 0;
  virtual INatNeg*          GetNatNeg(INatNegListener*const) = 0;
  virtual INetworkProfile*  GetNetworkProfile() = 0;
  virtual ITestInterface*   GetTestInterface() = 0;
  virtual IFileDownloader*  GetFileDownloader() = 0;
  virtual void              GetMemoryStatistics(ICrySizer* pSizer) = 0;
	virtual IPatchCheck* 			GetPatchCheck()=0;
	//etc
};

struct INetworkInterface
{
	virtual bool IsAvailable()const = 0;
};

struct IGameQueryListener;

struct ITestInterface
{
	virtual void TestBrowser(IGameQueryListener *GQListener){}
	virtual void TestReport(){}
	virtual void TestChat(){}
	virtual void TestStats(){}
};


struct SBasicServerInfo
{
	int				    m_numPlayers;
	int				    m_maxPlayers;
  bool          m_anticheat;
  bool          m_official;
	bool			    m_private;
  bool          m_dx10;
  bool          m_voicecomm;
  bool          m_friendlyfire;
  bool          m_dedicated;
	ushort        m_hostPort;
  ushort        m_publicPort;
	uint32			  m_publicIP;
	uint32			  m_privateIP;
  const char*		m_hostName;
	const char*		m_mapName;
	const char*		m_gameVersion;
	const char*		m_gameType;
};

enum EServerBrowserError
{
	eSBE_General,
	eSBE_ConnectionFailed,
	eSBE_DuplicateUpdate,
};

struct IServerListener
{
	//new server reported during update and basic values received
	virtual void NewServer(const int id,const SBasicServerInfo*)=0;
  virtual void UpdateServer(const int id,const SBasicServerInfo*)=0;
  //remove server from UI server list
	virtual void RemoveServer(const int id)=0;
	//server successfully pinged (servers behind NAT cannot be pinged)
	virtual void UpdatePing(const int id,const int ping)=0;

	//extended data - can be dependent on game type etc, retrieved only by request via IServerBrowser::UpdateServerInfo
	virtual void UpdateValue(const int id,const char* name,const char* value)=0;
	virtual void UpdatePlayerValue(const int id,const int playerNum,const char* name,const char* value)=0;
	virtual void UpdateTeamValue(const int id,const int teamNum,const char *name,const char* value)=0;

	//called on any error
	virtual void OnError(const EServerBrowserError)=0;

	//all servers in the list processed
	virtual void UpdateComplete(bool cancelled)=0;

	//cannot update/ping server 
	virtual void ServerUpdateFailed(const int id)=0;
  virtual void ServerUpdateComplete(const int id)=0;

  virtual void ServerDirectConnect(bool needsnat, uint ip, ushort port)=0;

};

struct IServerBrowser:public INetworkInterface
{
	virtual void	Start(bool browseLAN) = 0;
  virtual void	SetListener(IServerListener* lst) = 0;
	virtual void	Stop() = 0;
	virtual void	Update() = 0;
	virtual void	UpdateServerInfo(int id) = 0;
  virtual void  BrowseForServer(uint ip, ushort port) = 0;
  virtual void  BrowseForServer(const char* addr, ushort port) = 0;
  virtual void  SendNatCookie(uint ip, ushort port, int cookie) = 0;
  virtual void  CheckDirectConnect(int id, ushort port) = 0;
	virtual int		GetServerCount() = 0;
	virtual int		GetPendingQueryCount() = 0;
};

enum EChatJoinResult
{
	//join successful
	eCJR_Success,
	//error codes:
	eCJR_ChannelError,
	eCJR_Banned,
};

enum EChatUserStatus
{
  eCUS_inchannel,
  eCUS_joined,
  eCUS_left
};

enum ENetworkChatMessageType
{
  eNCMT_server,
  eNCMT_say,
  eNCMT_data
};

struct IChatListener
{
	virtual void Joined(EChatJoinResult)=0;
	virtual void Message(const char* from, const char* message, ENetworkChatMessageType type)=0;
	virtual void ChatUser(const char* nick, EChatUserStatus st)=0;
  virtual void OnError(int err)=0;
};

struct INetworkChat:public INetworkInterface
{
	virtual void Join()=0;
	virtual void Leave()=0;
  virtual void SetListener(IChatListener* lst)=0;
	virtual void Say(const char* message)=0;
	virtual void SendData(const char* nick, const char* message)=0;
};

enum EServerReportError
{
	eSRE_noerror,//everything is ok
	eSRE_socket,//socket problem: creation,bind, etc.
	eSRE_connect,//connection problem, DNS, server unreachable, NAT etc.
	eSRE_noreply,//no reply from the other end
	eSRE_other,//something happened
};

struct IServerReportListener
{
	virtual void OnError(EServerReportError) = 0;
	virtual void OnPublicIP(uint,unsigned short) = 0;

//  virtual void OnNatCookie(int) = 0;
//	virtual void OnAuthRequest(int playerid, const char* challenge) = 0;
//	virtual void OnAuthResult(int playerid, bool ok) = 0;
}; 

struct	IServerReport : public INetworkInterface
{
	virtual void AuthPlayer(EntityId playerid, uint ip, const char* challenge, const char* responce) = 0;
	virtual void ReAuthPlayer(EntityId playerid, const char* responce) = 0;
  virtual void PlayerDisconnected(EntityId playerid) = 0;


	virtual void SetReportParams(int numplayers, int numteams) = 0;
	
	virtual void SetServerValue(const char* key, const char*) = 0;
	virtual void SetPlayerValue(int, const char* key, const char*) = 0;
	virtual void SetTeamValue(int, const char* key, const char*) = 0;

	virtual void Update() = 0;
	virtual void StartReporting(INetNub*,IServerReportListener*) = 0;//Start reporting
	virtual void StopReporting() = 0;//Stop reporting and clear listener

  virtual void ProcessQuery(char* data, int len, struct sockaddr_in* addr) = 0;
};

enum EStatsTrackError
{
	eSTE_noerror,//everything is ok
	eSTE_socket,//socket problem: creation,bind, etc.
	eSTE_connect,//connection problem, DNS, server unreachable, NAT etc.
	eSTE_noreply,//no reply from the other end
	eSTE_other,//something happened
};

struct IStatsTrackListener
{
	virtual void OnError(EStatsTrackError) = 0;	
};

//Player/Team id semantic differ from ServerReport, 
//as ServerReport is stateless, and StatsTrack is not

struct	IStatsTrack : public INetworkInterface
{
	virtual int AddPlayer() = 0;
	virtual int AddTeam() = 0;

	virtual void RemovePlayer(int) = 0;
	virtual void RemoveTeam(int) = 0;

	virtual void SetServerValue(const char* key, const char*) = 0;
	virtual void SetPlayerValue(int, const char* key, const char*) = 0;
	virtual void SetTeamValue(int, const char* key, const char*) = 0;

	virtual void StartGame() = 0;
	virtual void SendSnapShot() = 0;
	virtual void EndGame() = 0;
};


struct INatNegListener
{
  virtual void OnDetected(bool success, bool compatible) = 0;//indicates NAT is possible on this site
  virtual void OnCompleted(int cookie, bool success, struct sockaddr_in* addr) = 0;
};

struct INatNeg : public INetworkInterface
{
  virtual void StartNegotiation(int cookie, bool server, int socket) = 0;
  virtual void CancelNegotiation(int cookie) = 0;
  virtual void OnPacket(char *data, int len, struct sockaddr_in *fromaddr) = 0;
};


struct SFileDownloadParameters
{
	SFileDownloadParameters()
	{
		sourceFilename.clear();
		destFilename.clear();
		destPath.clear();
		for(int i=0; i<16; ++i)
			md5[i] = 0;
		fileSize = 0;
	}

	string sourceFilename;		// eg "http://www.server.com/file.ext"
	string destFilename;			// eg "file.ext"
	string destPath;					// eg "Game\Levels\Multiplayer\IA\LevelName\"
	unsigned char md5[16];		// md5 checksum of the file
	int fileSize;

	virtual void SerializeWith( TSerialize ser )
	{
		ser.Value("SourceFilename", sourceFilename);
		ser.Value("DestFilename", destFilename);
		ser.Value("DestPath", destPath);
		ser.Value("FileSize", fileSize);

		for(int i=0; i<16; ++i)
			ser.Value("MD5Checksum", md5[i]);
	}
};

struct IFileDownload
{
	virtual bool Finished() const = 0;
	virtual float GetProgress() const = 0;
};

struct IFileDownloader : public INetworkInterface
{
	// start downloading from http server and save the file locally
	virtual void DownloadFile(SFileDownloadParameters& dl) = 0;

	// set throttling parameters (so as not to slow down games in progress)
	//	Probably want to turn off throttling if we're at the end of level, etc.
	// datasize: how much data to request
	// timedelay: how often to request it (in ms)
	// Set both to zero to disable throttling
	virtual void SetThrottleParameters(int datasize, int timedelay) = 0;

	// is a download in progress
	virtual bool IsDownloading() const = 0;

	// get progress of current download (0.0 - 1.0)
	virtual float GetDownloadProgress() const = 0;

	// get md5 checksum of downloaded file (only valid when IsDownloading()==false)
	virtual const unsigned char* GetFileMD5() const = 0;

	// stop downloads (also called when no more files to download)
	virtual void Stop() = 0;
};

struct IPatchCheck : public INetworkInterface
{
	// fire off a query to a server somewhere to figure out if there's an update available for download
	virtual bool CheckForUpdate() = 0;

	// is the query still pending?
	virtual bool IsUpdateCheckPending() const = 0;

	// is there an update?
	virtual bool IsUpdateAvailable() const = 0;

	// is it a required update?
	virtual bool IsUpdateMandatory() const = 0;

	// get the URL of the patch file
	virtual const char* GetPatchURL() const = 0;

	// get display name of new version
	virtual const char* GetPatchName() const = 0;

	// trigger install of patch on exit game
	virtual void SetInstallOnExit(bool install) = 0;
	virtual bool GetInstallOnExit() const = 0;

	// store and retrieve the place the patch was downloaded to
	virtual void SetPatchFileName(const char* filename) = 0;
	virtual const char* GetPatchFileName() const = 0;
};

const int MAX_PROFILE_STRING_LEN = 31;

enum EUserStatus
{
	eUS_offline = 0,
	eUS_online,
	eUS_playing,
	eUS_staging,
	eUS_chatting,
	eUS_away
};

struct SNetworkProfileUserStatus
{
	char	m_locationString[MAX_PROFILE_STRING_LEN+1];
	char	m_statusString[MAX_PROFILE_STRING_LEN+1];
	EUserStatus	m_status;
};

enum ENetworkProfileError
{
	eNPE_ok,
	eNPE_network,
	eNPE_fatal,
	eNPE_password,
	eNPE_nick
};

struct INetworkProfileListener
{
  virtual void AddNick(const char* nick) = 0;
  virtual void UpdateFriend(int id, const char* nick, EUserStatus, const char* status) = 0;
  virtual void RemoveFriend(int id) = 0;
  virtual void OnFriendRequest(int id, const char* message) = 0;
  virtual void OnMessage(int id, const char* message) = 0;
  virtual void LoginResult(ENetworkProfileError res, const char* descr, int id, const char* nick) = 0;
  virtual void OnError(ENetworkProfileError res, const char* descr) = 0;
  virtual void OnProfileInfo(int id, const char* key, const char* value) = 0;
  virtual void OnProfileComplete(int id) = 0;//finished updating profile info
  virtual void OnSearchResult(int id, const char* nick) = 0;
  virtual void OnSearchComplete() = 0;
  virtual void OnUserId(const char* nick, int id) = 0;
  virtual void OnUserNick(int id, const char* nick) = 0;
};

struct IStatsAccessor
{
  //input
  virtual const char* GetTableName() = 0;
  virtual int         GetFieldsNum() = 0;
  virtual const char* GetFieldName(int i) = 0;
};

struct IStatsReader : public IStatsAccessor
{
  //output
  virtual void        NextRecord(int id) = 0;
  virtual void        OnValue(int field, const char* val) = 0;
  virtual void        OnValue(int field, int val) = 0;
  virtual void        OnValue(int field, float val) = 0;
  virtual void        End(bool error) = 0;
};

struct IStatsWriter : public IStatsAccessor
{
  //input
  virtual int   GetRecordsNum() = 0;
  virtual int   NextRecord() = 0;
  virtual bool  GetValue(int field, const char*& val) = 0;
  virtual bool  GetValue(int field, int& val) = 0;
  virtual bool  GetValue(int field, float& val) = 0;
  //output
  virtual void  OnResult(int idx, int id, bool success) = 0;
};

struct INetworkProfile : public INetworkInterface
{
  virtual void AddListener(INetworkProfileListener* ) = 0;
  virtual void RemoveListener(INetworkProfileListener* ) = 0;

  virtual void Register(const char* nick, const char* email, const char* password) = 0;
  virtual void Login(const char* nick, const char* password) = 0;
  virtual void Logoff() = 0;
  virtual void SetStatus(EUserStatus status, const char* location) = 0;
  virtual void EnumUserNicks(const char* email, const char* password) = 0;
  virtual void AuthFriend(int id, bool auth) = 0;
  virtual void RemoveFriend(int id, bool ignore) = 0;
  virtual void AddFriend(int id, const char* reason) = 0;
  virtual void SendFriendMessage(int id, const char* message) = 0;
  virtual void GetProfileInfo(int id) = 0;
  virtual bool IsLoggedIn() = 0;
  virtual void UpdateBuddies() = 0;
  virtual void SearchFriends(const char* request) = 0;
  virtual void GetUserId(const char* nick) = 0;
  virtual void GetUserNick(int id) = 0;
  //Player Stats
  virtual void ReadStats(IStatsReader* reader) = 0;
  virtual void WriteStats(IStatsWriter* writer) = 0;
  //Other Player's Stats
  virtual void ReadStats(int id, IStatsReader* reader) = 0;
};
#endif /*__INETWORKSERVICE_H__*/

