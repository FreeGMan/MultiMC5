#include "OneSixVersion.h"

RuleAction RuleAction_fromString(QString name)
{
	if(name == "allow")
		return Allow;
	if(name == "disallow")
		return Disallow;
	return Defer;
}

OpSys OpSys_fromString(QString name)
{
	if(name == "linux")
		return Os_Linux;
	if(name == "windows")
		return Os_Windows;
	if(name == "osx")
		return Os_OSX;
	return Os_Other;
}

void Library::finalize()
{
	QStringList parts = m_name.split ( ':' );
	QString relative = parts[0];
	relative.replace ( '.','/' );
	relative += '/' + parts[1] + '/' + parts[2] + '/' + parts[1] + '-' + parts[2];
	if ( !m_is_native )
		relative += ".jar";
	else
	{
		if ( m_native_suffixes.contains ( currentSystem ) )
		{
			relative += "-" + m_native_suffixes[currentSystem] + ".jar";
		}
		else
		{
			// really, bad.
			relative += ".jar";
		}
	}
	m_storage_path = relative;
	m_download_path = m_base_url + relative;

	if ( m_rules.empty() )
	{
		m_is_active = true;
	}
	else
	{
		RuleAction result = Disallow;
		for ( auto rule: m_rules )
		{
			RuleAction temp = rule->apply ( this );
			if ( temp != Defer )
				result = temp;
		}
		m_is_active = ( result == Allow );
	}
	if ( m_is_native )
	{
		m_is_active = m_is_active && m_native_suffixes.contains ( currentSystem );
	}
}

void Library::setName ( QString name )
{
	m_name = name;
}
void Library::setBaseUrl ( QString base_url )
{
	m_base_url = base_url;
}
void Library::setIsNative()
{
	m_is_native = true;
}
void Library::addNative ( OpSys os, QString suffix )
{
	m_is_native = true;
	m_native_suffixes[os] = suffix;
}
void Library::setRules ( QList< QSharedPointer< Rule > > rules )
{
	m_rules = rules;
}
bool Library::isActive()
{
	return m_is_active;
}
bool Library::isNative()
{
	return m_is_native;
}
QString Library::downloadPath()
{
	return m_download_path;
}
QString Library::storagePath()
{
	return m_storage_path;
}


QList<QSharedPointer<Library> > OneSixVersion::getActiveNormalLibs()
{
	QList<QSharedPointer<Library> > output;
	for ( auto lib: libraries )
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<QSharedPointer<Library> > OneSixVersion::getActiveNativeLibs()
{
	QList<QSharedPointer<Library> > output;
	for ( auto lib: libraries )
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}


