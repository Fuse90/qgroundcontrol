/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "AutoPilotPlugin.h"
#include "QGCApplication.h"
#include "QGCMessageBox.h"
#include "MainWindow.h"
#include "ParameterLoader.h"
#include "UAS.h"
#include "FirmwarePlugin.h"

AutoPilotPlugin::AutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _firmwarePlugin(vehicle->firmwarePlugin())
    , _parametersReady(false)
    , _missingParameters(false)
	, _setupComplete(false)
{
    Q_ASSERT(vehicle);
	
	connect(_vehicle->uas(), &UASInterface::disconnected, this, &AutoPilotPlugin::_uasDisconnected);

	connect(this, &AutoPilotPlugin::parametersReadyChanged, this, &AutoPilotPlugin::_parametersReadyChanged);
}

AutoPilotPlugin::~AutoPilotPlugin()
{
    
}

void AutoPilotPlugin::_uasDisconnected(void)
{
	_parametersReady = false;
	emit parametersReadyChanged(_parametersReady);
}

void AutoPilotPlugin::_parametersReadyChanged(bool parametersReady)
{
	if (parametersReady) {
		_recalcSetupComplete();
		if (!_setupComplete) {
			QGCMessageBox::warning("Setup", "One or more vehicle components require setup prior to flight.");
			
			// Take the user to Vehicle Summary
            MainWindow::instance()->showSetupView();
			qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}
}

void AutoPilotPlugin::_recalcSetupComplete(void)
{
	bool newSetupComplete = true;
	
	foreach(const QVariant componentVariant, vehicleComponents()) {
		VehicleComponent* component = qobject_cast<VehicleComponent*>(qvariant_cast<QObject *>(componentVariant));
		Q_ASSERT(component);
		
		if (!component->setupComplete()) {
			newSetupComplete = false;
			break;
		}
	}
	
	if (_setupComplete != newSetupComplete) {
		_setupComplete = newSetupComplete;
		emit setupCompleteChanged(_setupComplete);
	}
}

bool AutoPilotPlugin::setupComplete(void)
{
	Q_ASSERT(_parametersReady);
	return _setupComplete;
}

void AutoPilotPlugin::resetAllParametersToDefaults(void)
{
    mavlink_message_t msg;
    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, _vehicle->uas()->getUASID(), 0, MAV_CMD_PREFLIGHT_STORAGE, 0, 2, -1, 0, 0, 0, 0, 0);
    _vehicle->sendMessage(msg);
}

void AutoPilotPlugin::refreshAllParameters(void)
{
    _vehicle->getParameterLoader()->refreshAllParameters();
}

void AutoPilotPlugin::refreshParameter(int componentId, const QString& name)
{
    _vehicle->getParameterLoader()->refreshParameter(componentId, name);
}

void AutoPilotPlugin::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    _vehicle->getParameterLoader()->refreshParametersPrefix(componentId, namePrefix);
}

bool AutoPilotPlugin::parameterExists(int componentId, const QString& name)
{
    return _vehicle->getParameterLoader()->parameterExists(componentId, name);
}

Fact* AutoPilotPlugin::getParameterFact(int componentId, const QString& name)
{
    return _vehicle->getParameterLoader()->getFact(componentId, name);
}

bool AutoPilotPlugin::factExists(FactSystem::Provider_t provider, int componentId, const QString& name)
{
    switch (provider) {
        case FactSystem::ParameterProvider:
            return _vehicle->getParameterLoader()->parameterExists(componentId, name);
            
        // Other providers will go here once they come online
    }
    
    Q_ASSERT(false);
    return false;
}

Fact* AutoPilotPlugin::getFact(FactSystem::Provider_t provider, int componentId, const QString& name)
{
    switch (provider) {
        case FactSystem::ParameterProvider:
            return _vehicle->getParameterLoader()->getFact(componentId, name);
            
        // Other providers will go here once they come online
    }
    
    Q_ASSERT(false);
    return NULL;
}

QStringList AutoPilotPlugin::parameterNames(int componentId)
{
    return _vehicle->getParameterLoader()->parameterNames(componentId);
}

const QMap<int, QMap<QString, QStringList> >& AutoPilotPlugin::getGroupMap(void)
{
    return _vehicle->getParameterLoader()->getGroupMap();
}

void AutoPilotPlugin::writeParametersToStream(QTextStream &stream)
{
    _vehicle->getParameterLoader()->writeParametersToStream(stream, _vehicle->uas()->getUASName());
}

QString AutoPilotPlugin::readParametersFromStream(QTextStream &stream)
{
    return _vehicle->getParameterLoader()->readParametersFromStream(stream);
}
