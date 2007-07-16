#ifndef _I_VIDEO_PLAYER_H_
#define _I_VIDEO_PLAYER_H_

#pragma once


struct IVideoPlayer
{
	enum EOptions
	{
		LOOP_PLAYBACK = 0x01,	// infinitely loop playback of video  
		DELAY_START		= 0x02	// delay start of playback until first Render() call
	};

	enum EPlaybackStatus
	{
		PBS_ERROR,
		PBS_PREPARING,		
		PBS_PLAYING,
		PBS_FINISHED,		
		PBS_PAUSED,
		PBS_STOPPED
	};

	// lifetime
	virtual void Release() = 0;

	// initialization
	virtual bool Load(const char* pFilePath, unsigned int options = 0) = 0;

	// rendering
	virtual EPlaybackStatus GetStatus() const = 0;
	virtual bool Pause(bool pause) = 0;	
	virtual bool SetViewport(int x0, int y0, int width, int height) = 0;
	virtual bool Render() = 0;

	// general property queries
	virtual int GetWidth() const = 0;
	virtual int GetHeight() const = 0;

	// more to come...

protected:
	IVideoPlayer() {}
	virtual ~IVideoPlayer() {}
};


#endif // #ifndef _I_VIDEO_PLAYER_H_