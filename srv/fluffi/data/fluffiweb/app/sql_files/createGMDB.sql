/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Junes Najah, Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

CREATE DATABASE IF NOT EXISTS fluffi_gm;

CREATE TABLE IF NOT EXISTS fluffi_gm.command_queue (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`Command` varchar(256) NOT NULL DEFAULT '',
	`Argument` varchar(4096) DEFAULT '',
	`CreationDate` timestamp NOT NULL DEFAULT current_timestamp(),
	`Done` INT(1) NOT NULL DEFAULT 0,
	`Error` VARCHAR(4096) NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.fuzzjob (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`name` varchar(256) DEFAULT NULL,
	`DBHost` varchar(256) DEFAULT NULL,
	`DBUser` varchar(256) DEFAULT NULL,
	`DBPass` varchar(256) DEFAULT NULL,
	`DBName` varchar(256) DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.localmanagers_statuses (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`ServiceDescriptorGUID` varchar(50) DEFAULT NULL,
	`TimeOfStatus` timestamp NULL DEFAULT NULL,
	`Status` varchar(1000) NULL DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.localmanagers_logmessages (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`ServiceDescriptorGUID` varchar(50) DEFAULT NULL,
	`TimeOfInsertion` timestamp NULL DEFAULT NULL,
	`LogMessage` varchar(2000) NULL DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.locations (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`Name` VARCHAR(150) NOT NULL,
	PRIMARY KEY (`ID`),
	UNIQUE (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.systems (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`Name` VARCHAR(150) NOT NULL,
	PRIMARY KEY (`ID`),
	UNIQUE (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.workers (
	`ServiceDescriptorGUID` varchar(50) NOT NULL,
	`ServiceDescriptorHostAndPort` VARCHAR(50) NOT NULL,
	`FuzzJob` INT DEFAULT NULL,
	`Location` INT NOT NULL,
	`TimeOfLastRequest` timestamp NOT NULL DEFAULT current_timestamp(),
	`AgentType` INT(1) NOT NULL,
	`AgentSubTypes` varchar(1000) DEFAULT "",
	PRIMARY KEY (`ServiceDescriptorGUID`),
	CONSTRAINT `location_fuzzjobs_ibfk_1` FOREIGN KEY (`Location`) REFERENCES `locations` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.location_fuzzjobs (
	`Location` INT NOT NULL,
	`Fuzzjob` INT NOT NULL,
	PRIMARY KEY (`Location`, `Fuzzjob`),
	INDEX `Fuzzjob` (`Fuzzjob`),
	CONSTRAINT `FK_location_fuzzjobs_fuzzjob` FOREIGN KEY (`Fuzzjob`) REFERENCES `fuzzjob` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT `FK_location_fuzzjobs_locations` FOREIGN KEY (`Location`) REFERENCES `locations` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.localmanagers (
	`ServiceDescriptorGUID` varchar(50) NOT NULL,
	`ServiceDescriptorHostAndPort` varchar(256) NOT NULL,
	`Location` INT NOT NULL,
	`FuzzJob` INT NOT NULL,
	PRIMARY KEY (`ServiceDescriptorGUID`),
	UNIQUE KEY  `Location` (`Location`,`Fuzzjob`),
	FOREIGN KEY (`Location`, `FuzzJob`) REFERENCES `location_fuzzjobs` (`Location`, `FuzzJob`) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.deployment_packages (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(150) NOT NULL,
	PRIMARY KEY (`ID`),
	UNIQUE (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.fuzzjob_deployment_packages (
	`Fuzzjob` INT NOT NULL,
	`DeploymentPackage` INT NOT NULL,
	PRIMARY KEY (`DeploymentPackage`, `Fuzzjob`),
	INDEX `fuzzjob_deployment_packages_FK1` (`Fuzzjob`),
	CONSTRAINT `fuzzjob_deployment_packages_FK1` FOREIGN KEY (`Fuzzjob`) REFERENCES `fuzzjob` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT `fuzzjob_deployment_packages_FK2` FOREIGN KEY (`DeploymentPackage`) REFERENCES `deployment_packages` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.systems_location (
	`System` INT NOT NULL,
	`Location` INT NULL DEFAULT NULL,
	PRIMARY KEY (`System`),
	INDEX `systems_location_FK1` (`System`),
	CONSTRAINT `systems_location_FK1` FOREIGN KEY (`System`) REFERENCES `systems` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE,
	CONSTRAINT `systems_location_FK2` FOREIGN KEY (`Location`) REFERENCES `locations` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.gm_options (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`Setting` VARCHAR(150) NOT NULL,
	`Value` VARCHAR(150) NOT NULL DEFAULT "",
	PRIMARY KEY (`ID`),
	UNIQUE (`Setting`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.system_fuzzjob_instances (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`System` INT NOT NULL,
	`Fuzzjob` INT DEFAULT NULL,
	`AgentType` INT(1) NOT NULL,
	`InstanceCount` INT NOT NULL,
	`Architecture` VARCHAR(10) NOT NULL,
	PRIMARY KEY (`ID`),
	UNIQUE KEY(`System`, `Fuzzjob`, `AgentType`),
	INDEX `system_fuzzjob_instances_FK1` (`System`),
	CONSTRAINT `system_fuzzjob_instances_FK1` FOREIGN KEY (`System`) REFERENCES `systems` (`ID`) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT IGNORE INTO fluffi_gm.gm_options (`Setting`, `Value`) VALUES ('bootsystemdir', 'odroidRoot');
INSERT IGNORE INTO fluffi_gm.gm_options (`Setting`, `Value`) VALUES ('agentstartermode', 'active');
INSERT IGNORE INTO fluffi_gm.gm_options (`Setting`, `Value`) VALUES ('checkrating', '-10000');
