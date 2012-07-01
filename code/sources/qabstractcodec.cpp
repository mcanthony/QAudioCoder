#include <qabstractcodec.h>
#include <QFile>

QAbstractCodec::QAbstractCodec()
{
	mName = "undefined";
	mVersion = "undefined";
	mError = QAbstractCodec::NoError;

	addFileExtension("");
	addFileExtension(".a");
	addFileExtension(".lib");
	#ifdef WINDOWS
		addFileExtension(".dll");
	#elif defined MACOSX
		addFileExtension(".so");
		addFileExtension(".so.0");
		addFileExtension(".so.0.0");
		addFileExtension(".so.0.0.0");
		addFileExtension(".dylib");
	#elif defined LINUX
		addFileExtension(".so");
		addFileExtension(".so.0");
		addFileExtension(".so.0.0");
		addFileExtension(".so.0.0.0");
	#endif
}

QAbstractCodec::~QAbstractCodec()
{
	mLibrary.unload();
}

QAbstractCodec::Error QAbstractCodec::load()
{
	if(!mLibrary.isLoaded())
	{
		QFile file(filePath());
		if(!file.exists())
		{
			mError = QAbstractCodec::PathError;
			return mError;
		}
		if(mLibrary.load())
		{
			QAbstractCodec::Error error = initializeLibrary();
			if(error != QAbstractCodec::NoError)
			{
				unload();
			}
			mError = error;
			return mError;
		}
		else
		{
			mError = QAbstractCodec::FileError;
			return mError;
		}
	}
	mError = QAbstractCodec::NoError;
	return mError;
}

QAbstractCodec::Error QAbstractCodec::load(QString filePath)
{
	setFilePath(filePath);
	return load();
}

bool QAbstractCodec::unload()
{
	return mLibrary.unload();
}

QString QAbstractCodec::filePath()
{
	return mLibrary.fileName();
}

void QAbstractCodec::setFilePath(QString filePath)
{
	mLibrary.setFileName(filePath);
}

QCodecFormat QAbstractCodec::format(QAudio::Mode mode)
{
	if(mode == QAudio::AudioInput)
	{
		return mInputFormat;
	}
	else if(mode == QAudio::AudioOutput)
	{
		return mOutputFormat;
	}
	return QCodecFormat();
}

void QAbstractCodec::setFormat(QAudio::Mode mode, QCodecFormat format)
{
	if(mode == QAudio::AudioInput)
	{
		mInputFormat = format;
	}
	else if(mode == QAudio::AudioOutput)
	{
		mOutputFormat = format;
	}
}

QString QAbstractCodec::name()
{
	return mName;
}

QString QAbstractCodec::version()
{
	return mVersion;
}

QStringList QAbstractCodec::fileNames()
{
	return mFileNames;
}

QStringList QAbstractCodec::fileExtensions()
{
	return mFileExtensions;
}

void QAbstractCodec::addFileName(QString name)
{
	mFileNames.append(name);
}

void QAbstractCodec::addFileExtension(QString extension)
{
	mFileExtensions.append(extension);
}

QAbstractCodec::Error QAbstractCodec::error()
{
	return mError;
}

bool QAbstractCodec::hasChunk()
{
	return !mChunks.isEmpty();
}

QAudioChunk QAbstractCodec::takeChunk()
{
	mChunks.dequeue();
}

void QAbstractCodec::addChunk(QAudioChunk chunk)
{
	mChunks.enqueue(chunk);
	emit encoded();
}

bool QAbstractCodec::operator == (const QAbstractCodec &other) const
{
	return mName != "" && mName == other.mName;
}