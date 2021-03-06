#include "DownloadJob.h"
#include "pathutils.h"
#include "MultiMC.h"

Download::Download (QUrl url, QString target_path, QString expected_md5 )
	:Job()
{
	m_url = url;
	m_target_path = target_path;
	m_expected_md5 = expected_md5;

	m_check_md5 = m_expected_md5.size();
	m_save_to_file = m_target_path.size();
	m_status = Job_NotStarted;
	m_opened_for_saving = false;
}

void Download::start()
{
	if ( m_save_to_file )
	{
		QString filename = m_target_path;
		m_output_file.setFileName ( filename );
		// if there already is a file and md5 checking is in effect and it can be opened
		if ( m_output_file.exists() && m_output_file.open ( QIODevice::ReadOnly ) )
		{
			// check the md5 against the expected one
			QString hash = QCryptographicHash::hash ( m_output_file.readAll(), QCryptographicHash::Md5 ).toHex().constData();
			m_output_file.close();
			// skip this file if they match
			if ( m_check_md5 && hash == m_expected_md5 )
			{
				qDebug() << "Skipping " << m_url.toString() << ": md5 match.";
				emit succeeded(index_within_job);
				return;
			}
			else
			{
				m_expected_md5 = hash;
			}
		}
		if(!ensureFilePathExists(filename))
		{
			emit failed(index_within_job);
			return;
		}
	}
	qDebug() << "Downloading " << m_url.toString();
	QNetworkRequest request ( m_url );
	request.setRawHeader(QString("If-None-Match").toLatin1(), m_expected_md5.toLatin1()); 
	
	auto worker = MMC->qnam();
	QNetworkReply * rep = worker->get ( request );
	
	m_reply = QSharedPointer<QNetworkReply> ( rep, &QObject::deleteLater );
	connect ( rep, SIGNAL ( downloadProgress ( qint64,qint64 ) ), SLOT ( downloadProgress ( qint64,qint64 ) ) );
	connect ( rep, SIGNAL ( finished() ), SLOT ( downloadFinished() ) );
	connect ( rep, SIGNAL ( error ( QNetworkReply::NetworkError ) ), SLOT ( downloadError ( QNetworkReply::NetworkError ) ) );
	connect ( rep, SIGNAL ( readyRead() ), SLOT ( downloadReadyRead() ) );
}

void Download::downloadProgress ( qint64 bytesReceived, qint64 bytesTotal )
{
	emit progress (index_within_job, bytesReceived, bytesTotal );
}

void Download::downloadError ( QNetworkReply::NetworkError error )
{
	// error happened during download.
	// TODO: log the reason why
	m_status = Job_Failed;
}

void Download::downloadFinished()
{
	// if the download succeeded
	if ( m_status != Job_Failed )
	{
		// nothing went wrong...
		m_status = Job_Finished;
		// save the data to the downloadable if we aren't saving to file
		if ( !m_save_to_file )
		{
			m_data = m_reply->readAll();
		}
		else
		{
			m_output_file.close();
		}

		//TODO: check md5 here!
		m_reply.clear();
		emit succeeded(index_within_job);
		return;
	}
	// else the download failed
	else
	{
		if ( m_save_to_file )
		{
			m_output_file.close();
			m_output_file.remove();
		}
		m_reply.clear();
		emit failed(index_within_job);
		return;
	}
}

void Download::downloadReadyRead()
{
	if( m_save_to_file )
	{
		if(!m_opened_for_saving)
		{
			if ( !m_output_file.open ( QIODevice::WriteOnly ) )
			{
				/*
				* Can't open the file... the job failed
				*/
				m_reply->abort();
				emit failed(index_within_job);
				return;
			}
			m_opened_for_saving = true;
		}
		m_output_file.write ( m_reply->readAll() );
	}
}

DownloadPtr DownloadJob::add ( QUrl url, QString rel_target_path, QString expected_md5 )
{
	DownloadPtr ptr (new Download(url, rel_target_path, expected_md5));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	return ptr;
}

void DownloadJob::partSucceeded ( int index )
{
	num_succeeded++;
	if(num_failed + num_succeeded == downloads.size())
	{
		if(num_failed)
		{
			qDebug() << "Download JOB failed: " << this;
			emit failed();
		}
		else
		{
			qDebug() << "Download JOB succeeded: " << this;
			emit succeeded();
		}
	}
}

void DownloadJob::partFailed ( int index )
{
	num_failed++;
	if(num_failed + num_succeeded == downloads.size())
	{
		qDebug() << "Download JOB failed: " << this;
		emit failed();
	}
}

void DownloadJob::partProgress ( int index, qint64 bytesReceived, qint64 bytesTotal )
{
	// PROGRESS? DENIED!
}


void DownloadJob::start()
{
	qDebug() << "Download JOB started: " << this;
	for(auto iter: downloads)
	{
		connect(iter.data(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		iter->start();
	}
}
