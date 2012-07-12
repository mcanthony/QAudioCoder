#include <qwavecoder.h>
#include <qwavecodec.h>
#include <qsamplesizeconverter.h>
#include <QDataStream>
#include <QTextStream>

QWaveCoder::QWaveCoder()
	: QAbstractCoder()
{
	mName = "Wave";
	addFileName("");
	mSupportedCodecs.append(&QWaveCodec::instance());
}

QAbstractCoder::Error QWaveCoder::load()
{
	return QAbstractCoder::NoError;
}

QAbstractCoder::Error QWaveCoder::load(QString filePath)
{
	return QAbstractCoder::NoError;
}

bool QWaveCoder::unload()
{
	return true;
}

QAudioCodec* QWaveCoder::detectCodec(const QByteArray &data)
{
	QDataStream stream((QByteArray*) &data, QIODevice::ReadOnly);
	char data2[2];
	char data4[4];

	if(	stream.readRawData(data4, 4) < 4 || !(QString(data4).toLower() == "riff" || QString(data4).toLower() == "rifx") ||
		stream.readRawData(data4, 4) < 4 || toInt(data4) <= 0 ||
		stream.readRawData(data4, 4) < 4 || QString(data4).toLower() != "wave" ||
		stream.readRawData(data4, 4) < 4 || QString(data4).toLower() != "fmt ")
	{
		return NULL;
	}

	stream.skipRawData(4);
	if(	stream.readRawData(data2, 2) < 2 || data2[0] != 1 ||
		stream.readRawData(data2, 2) < 2 || toShort(data2) <= 0 ||
		stream.readRawData(data4, 4) < 4 || toInt(data4) <= 0 || 
		stream.readRawData(data4, 4) < 4 || toInt(data4) <= 0 || 
		stream.readRawData(data2, 2) < 2 || toShort(data2) <= 0 ||
		stream.readRawData(data2, 2) < 2 || toShort(data2) <= 0)
	{
		return NULL;
	}

	return &QWaveCodec::instance();
}

QByteArray& QWaveCoder::header()
{
	mHeader.clear();
	QDataStream stream((QByteArray*) &mHeader, QIODevice::WriteOnly);

	if(mOutputFormat.byteOrder() == QExtendedAudioFormat::LittleEndian)
	{
		stream.setByteOrder(QDataStream::LittleEndian);
		stream << qint8('R') << qint8('I') << qint8('F') << qint8('F');
	}
	else if(mOutputFormat.byteOrder() == QExtendedAudioFormat::BigEndian)
	{
		stream.setByteOrder(QDataStream::BigEndian);
		stream << qint8('R') << qint8('I') << qint8('F') << qint8('X');
	}
	stream << int((mSamples * mOutputFormat.sampleSize() / 8) - 8);
	stream << qint8('W') << qint8('A') << qint8('V') << qint8('E');
	
	stream << qint8('f') << qint8('m') << qint8('t') << qint8(' ');
	stream << int(16);
	stream << short(1);
	stream << short(mOutputFormat.channelCount());
	stream << int(mOutputFormat.sampleRate());
	stream << int(mOutputFormat.sampleRate() * mOutputFormat.channelCount() * mOutputFormat.sampleSize() / 8);
	stream << short(mOutputFormat.channelCount() * mOutputFormat.sampleSize() / 8);
	stream << short(mOutputFormat.sampleSize());

	stream << qint8('d') << qint8('a') << qint8('t') << qint8('a');
	stream << int(mSamples * mOutputFormat.sampleSize() / 8);

	return mHeader;
}

int QWaveCoder::headerSize()
{
	return 44;
}

/*

void QWaveCoder::createHeader(QByteArray &header, const QExtendedAudioFormat &format, QAudioInfo &content)
{
	QDataStream stream((QByteArray*) &header, QIODevice::WriteOnly);

	if(format.byteOrder() == QExtendedAudioFormat::LittleEndian)
	{
		stream.setByteOrder(QDataStream::LittleEndian);
		stream << qint8('R') << qint8('I') << qint8('F') << qint8('F');
	}
	else if(format.byteOrder() == QExtendedAudioFormat::BigEndian)
	{
		stream.setByteOrder(QDataStream::BigEndian);
		stream << qint8('R') << qint8('I') << qint8('F') << qint8('X');
	}
	stream << int(content.size() - 8);
	stream << qint8('W') << qint8('A') << qint8('V') << qint8('E');
	
	stream << qint8('f') << qint8('m') << qint8('t') << qint8(' ');
	stream << int(16);
	stream << short(1);
	stream << short(format.channelCount());
	stream << int(format.sampleRate());
	stream << int(format.sampleRate() * format.channelCount() * format.sampleSize() / 8);
	stream << short(format.channelCount() * format.sampleSize() / 8);
	stream << short(format.sampleSize());

	stream << qint8('d') << qint8('a') << qint8('t') << qint8('a');
	stream << int(mSamples * format.sampleSize() / 8);
}
*/
bool QWaveCoder::initializeDecode()
{
	mError = QAbstractCoder::NoError;
	decodePointer = &QWaveCoder::decodeHeader;
	return true;
}

bool QWaveCoder::finalizeDecode()
{
	return true;
}

void QWaveCoder::decode(const void *input, int size)
{
	(this->*decodePointer)(input, size);
}

void QWaveCoder::decodeHeader(const void *input, int size)
{
	if(size >= 44)
	{
		decodePointer = &QWaveCoder::decodeData;

		QString string;
		QTextStream stream(&string);
		char *data = (char*) input;

		stream << data[0] << data [1] << data[2] << data[3];
		if(string.toLower() == "riff")
		{
			mInputFormat.setByteOrder(QExtendedAudioFormat::LittleEndian);
		}
		else if(string.toLower() == "rifx")
		{
			mInputFormat.setByteOrder(QExtendedAudioFormat::BigEndian);
		}

		string.clear();
		stream << data[22] << data[23];
		mInputFormat.setChannelCount(toShort(string.toAscii().data()));

		string.clear();
		stream << data[24] << data[25] << data[26] << data[27];
		mInputFormat.setSampleRate(toInt(string.toAscii().data()));

		string.clear();
		stream << data[34] << data[35];
		mInputFormat.setSampleSize(toShort(string.toAscii().data()));

		mInputFormat.setSampleType(QExtendedAudioFormat::SignedInt);
		mInputFormat.setCodec(&QWaveCodec::instance());

		emit formatChanged(mInputFormat);
		decodeData(data + 44, size - 44);
	}
}

void QWaveCoder::decodeData(const void *input, int size)
{
	qbyte *data = new qbyte[size];
	memcpy(data, input, size);
	emit decoded(new QSampleArray(data, size, size / (mInputFormat.sampleSize() / 8)));
}

bool QWaveCoder::initializeEncode()
{
	mSamples = 0;
	int inSize = mInputFormat.sampleSize();
	int outSize = mOutputFormat.sampleSize();
	QExtendedAudioFormat::SampleType inType = mInputFormat.sampleType();
	QExtendedAudioFormat::SampleType outType = mOutputFormat.sampleType();

	if(!(inSize == 8 || inSize == 16 || inSize == 32) && !(outSize == 8 || outSize == 16 || outSize == 32))
	{
		mError = QAbstractCoder::SampleSizeError;
		return false;
	}

	if(inType == QExtendedAudioFormat::Unknown || outType == QExtendedAudioFormat::Unknown)
	{
		mError = QAbstractCoder::SampleTypeError;
		return false;
	}

	if(!mConverter.initialize(mInputFormat, mOutputFormat))
	{
		mError = QAbstractCoder::InitializationError;
		return false;
	}

	mError = QAbstractCoder::NoError;
	return true;
}

bool QWaveCoder::finalizeEncode()
{
	return true;
}

void QWaveCoder::encode(const void *input, int samples)
{
	int bytes;
	char *output = (char*) mConverter.convert(input, samples, bytes);
	mSamples += samples;
	emit encoded(new QSampleArray(output, bytes, samples));
}

QAbstractCoder::Error QWaveCoder::initializeLibrary()
{
	return QAbstractCoder::NoError;
}

short QWaveCoder::toShort(char data[])
{
	short result = 0;
	memcpy(&result, data, 2);
	return result;
}

int QWaveCoder::toInt(char data[])
{
	int result = 0;
	memcpy(&result, data, 4);
	return result;
}
