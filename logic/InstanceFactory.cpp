/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InstanceFactory.h"

#include <QDir>
#include <QFileInfo>

#include "BaseInstance.h"
#include "LegacyInstance.h"
#include "OneSixInstance.h"
#include "NostalgiaInstance.h"
#include "InstanceVersion.h"
#include "MinecraftVersion.h"

#include "inifile.h"
#include <inisettingsobject.h>
#include <setting.h>

#include "pathutils.h"

InstanceFactory InstanceFactory::loader;

InstanceFactory::InstanceFactory() :
	QObject(NULL)
{
	
}

InstanceFactory::InstLoadError InstanceFactory::loadInstance(BaseInstance *&inst, const QString &instDir)
{
	auto m_settings = new INISettingsObject(PathCombine(instDir, "instance.cfg"));
	
	m_settings->registerSetting(new Setting("InstanceType", "Legacy"));
	
	QString inst_type = m_settings->get("InstanceType").toString();
	
	//FIXME: replace with a map lookup, where instance classes register their types
	if(inst_type == "Legacy")
	{
		inst = new LegacyInstance(instDir, m_settings, this);
	}
	else if(inst_type == "OneSix")
	{
		inst = new OneSixInstance(instDir, m_settings, this);
	}
	else if(inst_type == "Nostalgia")
	{
		inst = new NostalgiaInstance(instDir, m_settings, this);
	}
	else
	{
		return InstanceFactory::UnknownLoadError;
	}
	return NoLoadError;
}


InstanceFactory::InstCreateError InstanceFactory::createInstance( BaseInstance*& inst, InstVersionPtr version, const QString& instDir )
{
	QDir rootDir(instDir);
	
	qDebug(instDir.toUtf8());
	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		return InstanceFactory::CantCreateDir;
	}
	auto mcVer = version.dynamicCast<MinecraftVersion>();
	if(!mcVer)
		return InstanceFactory::NoSuchVersion;
	
	auto m_settings = new INISettingsObject(PathCombine(instDir, "instance.cfg"));
	m_settings->registerSetting(new Setting("InstanceType", "Legacy"));
	
	switch(mcVer->type)
	{
		case MinecraftVersion::Legacy:
			m_settings->set("InstanceType", "Legacy");
			inst = new LegacyInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor);
			inst->setShouldUseCustomBaseJar(false);
			break;
		case MinecraftVersion::OneSix:
			m_settings->set("InstanceType", "OneSix");
			inst = new OneSixInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor);
			inst->setShouldUseCustomBaseJar(false);
			break;
		case MinecraftVersion::Nostalgia:
			m_settings->set("InstanceType", "Nostalgia");
			inst = new NostalgiaInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor);
			inst->setShouldUseCustomBaseJar(false);
			break;
		default:
		{
			delete m_settings;
			return InstanceFactory::NoSuchVersion;
		}
	}
	
	//FIXME: really, how do you even know?
	return InstanceFactory::NoCreateError;
}
