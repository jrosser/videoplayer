#pragma once

#include <QObject>
#include "videoTransport.h"

class VideoTransportQT : public QObject, public VideoTransport
{
	Q_OBJECT

public:
	VideoTransportQT(ReaderInterface *r, int read_ahead=16, int lru_cache_len=16)
	: VideoTransport(r, read_ahead, lru_cache_len)
	{}

protected:
	void notifyEndOfFile() { emit(endOfFile()); }
signals:
	void endOfFile();

public slots:
	//slots to connect keys / buttons to play and scrub controls
	void transportFwd100() { VideoTransport::transportPlay(100); }
	void transportFwd50() { VideoTransport::transportPlay(50); }
	void transportFwd20() { VideoTransport::transportPlay(20); }
	void transportFwd10() { VideoTransport::transportPlay(10); }
	void transportFwd5() { VideoTransport::transportPlay(5); }
	void transportFwd2() { VideoTransport::transportPlay(2); }
	void transportFwd1() { VideoTransport::transportPlay(1); }
	void transportStop() { VideoTransport::transportStop(); }
	void transportRev1() { VideoTransport::transportPlay(-1); }
	void transportRev2() { VideoTransport::transportPlay(-2); }
	void transportRev5() { VideoTransport::transportPlay(-5); }
	void transportRev10() { VideoTransport::transportPlay(-10); }
	void transportRev20() { VideoTransport::transportPlay(-20); }
	void transportRev50() { VideoTransport::transportPlay(-50); }
	void transportRev100() { VideoTransport::transportPlay(-100); }

	//frame jog controls
	void transportJogFwd() { VideoTransport::transportJogFwd(); }
	void transportJogRev() { VideoTransport::transportJogRev(); }

	//play / pause
	void transportPlayPause() { VideoTransport::transportPlayPause(); }
};
