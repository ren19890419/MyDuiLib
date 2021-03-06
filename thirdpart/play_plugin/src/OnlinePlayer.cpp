#ifdef _WIN32
#	include <winsock2.h>
#	include <windows.h>
#endif
#include "OnlinePlayer.h"
#include "utility/log.h"
#include <fstream>

#ifdef _WIN32
//#define  USE_P2P
#endif

COnlinePlayer::COnlinePlayer()
	:_pDecoderPlugin(NULL),
	_hDecoder(NULL),
	_isDownFailed(false),
	_nFileBytes(0),
	_nDownBytes(0),
	_isOnline(false),
	_hP2PServ(NULL)
{
	BeginP2PService();
}
COnlinePlayer::~COnlinePlayer()
{
#ifdef USE_P2P
	if (_hP2PServ)
	{
		TerminateProcess(_hP2PServ,0);
		_hP2PServ = NULL;
	}
#endif
}


void COnlinePlayer::BeginP2PService()
{
#ifdef USE_P2P
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	std::string sProcessPath = _FStd(FGetModulePath)();
	std::string module_dir = _FStd(FGetDirectory)(sProcessPath.c_str());
	sProcessPath = _FStd(FJoinPath)(module_dir, "p2pStream.exe");
	
	BOOL  bRet = ::CreateProcessA(sProcessPath.c_str(),NULL,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	DWORD nErr = ::GetLastError();
	assert(bRet);
	if (!bRet)
	{
		return;
	}

	_hP2PServ = pi.hProcess;
#endif
}


unsigned long COnlinePlayer::GetFileBytes() const
{
	return _isOnline ? _nFileBytes : __super::GetFileBytes();
}

unsigned long COnlinePlayer::GetDownloadBytes() const
{
	return _isOnline ? _nDownBytes : __super::GetDownloadBytes();
}

void COnlinePlayer::OnRecvHeader(CResDownloader* resDownloader,size_t fileSize)
{
	assert(fileSize);
	assert(_pDecoderPlugin);
	assert(_hDecoder);

	this->_nFileBytes = fileSize;

	if(_pDecoderPlugin && _hDecoder)
	{
		_pDecoderPlugin->write_data(_hDecoder,NULL,fileSize);
	}
}
void COnlinePlayer::OnRecvData(CResDownloader* resDownloader,const	char* buf,size_t bytes,size_t fileSize)
{
	assert(buf);
	assert(bytes);
	assert(fileSize >= bytes);
	assert(_pDecoderPlugin);
	assert(_hDecoder);

	this->OnIPlayerCallBack(XPET_DOWN_DATA,(void*)buf,&bytes);
	if(_pDecoderPlugin && _hDecoder)
	{	
		size_t writeBytes = _pDecoderPlugin->write_data(_hDecoder,(unsigned char*)buf,bytes);
		assert(writeBytes == bytes);
		this->_nDownBytes += writeBytes;
		assert(this->_nDownBytes <= fileSize);
	}
}
void COnlinePlayer::OnFinished(CResDownloader* resDownloader)
{
	assert(_pDecoderPlugin);
	assert(_hDecoder);

	if(_pDecoderPlugin && _hDecoder)
	{
		_pDecoderPlugin->write_data(_hDecoder,NULL,0);
	}

	this->OnIPlayerCallBack(XPET_DOWN_FINISHED,NULL,NULL);
}
bool COnlinePlayer::OnFailed(CResDownloader* resDownloader)
{
	sLog("COnlinePlayer::OnFailed");
	this->_isDownFailed = true;
	this->OnIPlayerCallBack(XPET_DOWN_FAILED,NULL,NULL);
	{
		//::remove();
	}
	return true;
}
void COnlinePlayer::OnCancled(CResDownloader* resDownloader)
{
//	assert(false);
}

bool COnlinePlayer::IsDownFailed() const
{
	return _isDownFailed && (0 >= (_pDecoderPlugin->get_available_samples_count(_hDecoder))) && (XPS_PLAYING == GetState());
}

std::string COnlinePlayer::SubStrSongID(const std::string& wsFile)
{
	std::string sFile = wsFile;
	size_t nPos = sFile.find_last_of('/');
	std::string sSongID = sFile.substr(nPos+1,sFile.length()-nPos-1);
	return sSongID;
}

void COnlinePlayer::OnPlay(
	const std::string& wsFile,
	const std::string& wsUrl,
	int nFileType,
	int nBegin,
	int nEnd)
{
	sLog("COnlinePlayer::OnPlayBegin<%s>,Hash=%s",wsFile.c_str(),wsUrl.c_str());

	_isOnline = !wsUrl.empty();
	if(!_isOnline) 
		return  CLocalPlayer::OnPlay(wsFile,wsUrl,nFileType,nBegin,nEnd);

	{
		_pDecoderPlugin = NULL;
		_hDecoder =  this->OpenDecoder(wsFile,true,&_pDecoderPlugin,nFileType,nBegin,nEnd);

		assert(_pDecoderPlugin);
		assert(_hDecoder);

		if(!_hDecoder || !_pDecoderPlugin)
		{
			assert(false);
			this->OnIPlayerCallBack(XPET_ERROR,(void*)wsFile.c_str(),NULL);
			return;
		}

		std::shared_ptr<void> spScopedDecoder(_hDecoder,_pDecoderPlugin->close);
		this->_isDownFailed = false;
		this->_nFileBytes   = 0;
		this->_nDownBytes   = 0;

#ifdef USE_P2P
		
		spP2PDownloaderT p2p(new P2PDownLoad);
		
		assert(p2p);
		if(p2p)
		{
			p2p->_dataCallback   = DownloadCallBack;
			p2p->_endCallBack    = EndCallBack;
			p2p->_infoCallback   = ResInfoCallBack;
			p2p->_sHash          = wsUrl;
			p2p->_hash           = (char*)(p2p->_sHash.c_str());
			p2p->_hDecoder       = spScopedDecoder;
			p2p->_pDecoderPlugin = _pDecoderPlugin;
			p2p->_pOnLinePlayer  = this;
			p2p->_reqBegin       = 0;
			p2p->_reqLength      = -1;
			p2p->_sourceLevel    = P2PLibInterface::DownloadHigh;
			p2p->_nFileSize      = 0;
			p2p->_nDownloadSize  = 0;
			p2p->_SongID         = SubStrSongID(wsFile);
			p2p->_timeout		 = 3 * 1000;
			p2p->_nRedownCnt     = 10;
	
			{
				std::unique_lock<std::mutex> lock(_mutex);
				_set.insert(p2p);
			}

			sLog("StartDownload  hash=%s  SongID=%s",wsUrl.c_str(), SubStrSongID(wsFile).c_str());
			P2PLibInterface::StartDownload(p2p.get());
			this->DefaultPlayPro(wsFile,_pDecoderPlugin,spScopedDecoder,nBegin,nEnd,std::bind(&COnlinePlayer::IsDownFailed,this));
			{
				std::unique_lock<std::mutex> lock(_mutex);
				if (_set.find(p2p) != _set.end())
				{
					P2PLibInterface::CancelDownload(p2p.get());
					sLog("CancelDownload  hash=%s  SongID=%s",wsUrl.c_str(), SubStrSongID(wsFile).c_str());
				}
			}
			
		}
#else
			std::string strUrl = wsUrl;
			assert(!strUrl.empty());
			sLog("StartDownload  hash=%s  SongID=%s", wsUrl.c_str(), SubStrSongID(wsFile).c_str());
			CResDownloader resDownloader(strUrl.c_str(),this);
			resDownloader.start();
			this->DefaultPlayPro(wsFile,_pDecoderPlugin,spScopedDecoder,nBegin,nEnd,std::bind(&COnlinePlayer::IsDownFailed,this));
			resDownloader.stop();
#endif	
		}

	sLog("COnlinePlayer::OnPlayEnd<%s>,Hash=%s  SongID=%s",wsFile.c_str(),wsUrl.c_str(), SubStrSongID(wsFile).c_str());
}

void COnlinePlayer::ResInfoCallBack(DownUserInfo* info,size_t resLength)
{
	P2PDownLoad *p2p = (P2PDownLoad*)info;
	assert(p2p);
	if(!p2p) return ;
	COnlinePlayer *pThis = p2p->_pOnLinePlayer;
	assert(pThis);
	if(!pThis) return;

	if (p2p->_pDecoderPlugin && p2p->_hDecoder)
	{
		sLog("ResInfoCallBack  hash=%s  SongID=%s  FileSize=%d",p2p->_hash,p2p->_SongID.c_str(),resLength);
		pThis->_nFileBytes = resLength;
		p2p->_nFileSize    = resLength;
		pThis->_nDownBytes = p2p->_nDownloadSize;
		p2p->_pDecoderPlugin->write_data(p2p->_hDecoder.get(),NULL,resLength);/*通知解码器资源大小*/
	}
}

void COnlinePlayer::DownloadCallBack(DownUserInfo* info, size_t begin, size_t length, void* buffer)
{
	P2PDownLoad *p2p = (P2PDownLoad*)info;
	assert(p2p);
	if(!p2p) return ;
	COnlinePlayer *pThis = p2p->_pOnLinePlayer;
	assert(pThis);
	if(!pThis) return;

	if(p2p->_pDecoderPlugin && p2p->_hDecoder)
	{
		size_t writeBytes = p2p->_pDecoderPlugin->write_data(p2p->_hDecoder.get(),(unsigned char*)buffer,length);
		assert(writeBytes == length);
		pThis->_nDownBytes  += writeBytes;
		p2p->_nDownloadSize += writeBytes;
		p2p->_nRedownCnt     = 10;
	}	

}

void COnlinePlayer::EndCallBack(DownUserInfo* info, P2PLibInterface::CallBackData* pStatus)
{
	P2PDownLoad *p2p = (P2PDownLoad*)info;
	assert(p2p);
	if(!p2p) return ;

	if(pStatus)
	{
		switch (pStatus->state)
		{
		case P2PLibInterface::Unkown:
			sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,Unkown,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
			break;
		case P2PLibInterface::Downing:
			sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,Downing,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
			break;
		case P2PLibInterface::SchedulePause:
			sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,SchedulePause,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
			break;
		case P2PLibInterface::Checking:
			sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,Checking,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
			break;
		case P2PLibInterface::Finish:
			if((p2p->_nDownloadSize == pStatus->downloadedSize) && (p2p->_nFileSize == pStatus->downloadedSize))
			{
				sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,Download Finished,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
				p2p->_pOnLinePlayer->OnFinished(NULL);
			}
			else
			{
				sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,非正常Finish状态，P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
			}
			break;
		case P2PLibInterface::Failed:
			sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,Download Fail,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
			p2p->_pOnLinePlayer->OnFailed(NULL);
			break;
		case P2PLibInterface::Redown:
			{
				int nSampCnt = p2p->_pDecoderPlugin->get_available_samples_count(p2p->_hDecoder.get());
				if( (nSampCnt > 0) || (nSampCnt <= 0 && p2p->_nRedownCnt--) )
				{
					P2PLibInterface::StartDownload(p2p);
					sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,ReDownload,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
					return ;
				}
				else
				{
					sLog("EndCallBack--hash=%s,SongID=%s,Status=%d,已超过失败重试次数，Download Fail,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
					p2p->_pOnLinePlayer->OnFailed(NULL);
					P2PLibInterface::CancelDownload(p2p);
					return;
				}
			}
		default:
			sLog("EndCallBack--hash=%s,SongID=%s,Unknown Status=%d,P2PDownSize=%d,PlayerGetSize=%d,ResourceSize=%d",p2p->_sHash.c_str(),p2p->_SongID.c_str(),pStatus->state,pStatus->downloadedSize,p2p->_nDownloadSize,pStatus->resSize);
		}
	}

	std::unique_lock<std::mutex> lock(p2p->_pOnLinePlayer->_mutex);
	for (auto iter = p2p->_pOnLinePlayer->_set.begin(); iter != p2p->_pOnLinePlayer->_set.end(); ++iter)
	{
		if(iter->get() == p2p)
		{
			p2p->_pOnLinePlayer->_set.erase(iter);
			break;
		}
	}
}

/************************************************************************/
/* 预缓存处理部分                                                       */
/************************************************************************/

void COnlinePlayer::PreCacheNextSong(const char* pwsUrl, void* pUserData)
{
	P2PDownLoad* p2p = new P2PDownLoad;
	if(!p2p) return ;

	p2p->_dataCallback   = NULL;
	p2p->_endCallBack    = PreCacheEndCallBack;
	p2p->_infoCallback   = NULL;
	p2p->_sHash          = pwsUrl;
	p2p->_hash           = (char*)(p2p->_sHash.c_str());
	p2p->_reqBegin       = 0;
	p2p->_reqLength      = -1;
	p2p->_sourceLevel    = P2PLibInterface::DownloadNormal;

	P2PLibInterface::StartDownload(p2p);
	sLog("COnlinePlayer::PreCacheNextSong,hash = %s",pwsUrl);
}

void COnlinePlayer::PreCacheEndCallBack(DownUserInfo* info, P2PLibInterface::CallBackData* pStatus)
{
	P2PDownLoad *p2p = (P2PDownLoad*)info;
	assert(p2p);
	if(!p2p) return ;
	
	sLog("COnlinePlayer::PreCacheEndCallBack,hash = %s",p2p->_sHash.c_str());
	delete p2p;
	p2p = NULL;
	
}