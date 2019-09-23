/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Junes Najah, Thomas Riedmaier, Abian Blome, Pascal Eckmann
*/

CREATE DATABASE IF NOT EXISTS fluffi_gm;

CREATE TABLE IF NOT EXISTS fluffi_gm.command_queue (
§§	`ID` int(11) NOT NULL AUTO_INCREMENT,
§§	`Command` varchar(256) NOT NULL DEFAULT '',
§§	`Argument` varchar(4096) DEFAULT '',
§§	`CreationDate` timestamp NOT NULL DEFAULT current_timestamp(),
§§	`Done` int(11) NOT NULL DEFAULT 0,
§§	`Error` VARCHAR(4096) NULL,
§§	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.fuzzjob (
§§	`id` int(11) NOT NULL AUTO_INCREMENT,
§§	`name` varchar(256) DEFAULT NULL,
§§	`DBHost` varchar(256) DEFAULT NULL,
§§	`DBUser` varchar(256) DEFAULT NULL,
§§	`DBPass` varchar(256) DEFAULT NULL,
§§	`DBName` varchar(256) DEFAULT NULL,
§§	PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.localmanagers_statuses (
§§	`ID` int(11) NOT NULL AUTO_INCREMENT,
§§	`ServiceDescriptorGUID` varchar(50) DEFAULT NULL,
§§	`TimeOfStatus` timestamp NULL DEFAULT NULL,
§§	`Status` varchar(1000) DEFAULT NULL,
§§	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi_gm.locations (
§§	`id` INT(11) NOT NULL AUTO_INCREMENT,
§§	`Name` VARCHAR(150) NOT NULL,
§§	PRIMARY KEY (`id`),
§§	UNIQUE (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

§§CREATE TABLE IF NOT EXISTS fluffi_gm.systems (
§§	`id` INT(11) NOT NULL AUTO_INCREMENT,
§§	`Name` VARCHAR(150) NOT NULL,
§§	PRIMARY KEY (`id`),
§§	UNIQUE (`Name`)
§§) ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
CREATE TABLE IF NOT EXISTS fluffi_gm.workers (
§§	`ServiceDescriptorGUID` varchar(50) NOT NULL,
§§	`ServiceDescriptorHostAndPort` VARCHAR(50) NOT NULL,
§§	`FuzzJob` int(11) DEFAULT NULL,
§§	`Location` INT(11) NOT NULL,
§§	`TimeOfLastRequest` timestamp NOT NULL DEFAULT current_timestamp(),
§§	`AgentType` int(11) NOT NULL,
§§	`AgentSubTypes` varchar(1000) DEFAULT "",
§§	PRIMARY KEY (`ServiceDescriptorGUID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

§§CREATE TABLE IF NOT EXISTS fluffi_gm.location_fuzzjobs (
§§	`Location` INT(11) NOT NULL,
§§	`Fuzzjob` INT(11) NOT NULL,
§§	PRIMARY KEY (`Location`, `Fuzzjob`),
§§	INDEX `Fuzzjob` (`Fuzzjob`),
§§	CONSTRAINT `location_fuzzjobs_ibfk_1` FOREIGN KEY (`Location`) REFERENCES `locations` (`id`) ON UPDATE CASCADE ON DELETE CASCADE,
§§	CONSTRAINT `location_fuzzjobs_ibfk_2` FOREIGN KEY (`Fuzzjob`) REFERENCES `fuzzjob` (`id`) ON UPDATE CASCADE ON DELETE CASCADE
§§)ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
§§CREATE TABLE IF NOT EXISTS fluffi_gm.localmanagers (
§§	`ServiceDescriptorGUID` varchar(50) NOT NULL,
§§	`ServiceDescriptorHostAndPort` varchar(256) NOT NULL,
§§	`Location` INT(11) NOT NULL,
§§	`FuzzJob` INT(11) NOT NULL,
§§	PRIMARY KEY (`ServiceDescriptorGUID`),
§§	UNIQUE KEY  `Location` (`Location`,`Fuzzjob`),
§§	FOREIGN KEY (`Location`, `FuzzJob`) REFERENCES `location_fuzzjobs` (`Location`, `FuzzJob`) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
§§CREATE TABLE IF NOT EXISTS fluffi_gm.deployment_packages (
§§	`id` INT(11) NOT NULL AUTO_INCREMENT,
§§	`name` VARCHAR(150) NOT NULL,
§§	PRIMARY KEY (`id`),
§§	UNIQUE (`name`)
§§) ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
§§CREATE TABLE IF NOT EXISTS fluffi_gm.fuzzjob_deployment_packages (
§§	`Fuzzjob` INT(11) NOT NULL,
§§	`DeploymentPackage` INT(11) NOT NULL,
§§	PRIMARY KEY (`DeploymentPackage`, `Fuzzjob`),
§§	INDEX `fuzzjob_deployment_packages_FK1` (`Fuzzjob`),
§§	CONSTRAINT `fuzzjob_deployment_packages_FK1` FOREIGN KEY (`Fuzzjob`) REFERENCES `fuzzjob` (`id`) ON UPDATE CASCADE ON DELETE CASCADE,
§§	CONSTRAINT `fuzzjob_deployment_packages_FK2` FOREIGN KEY (`DeploymentPackage`) REFERENCES `deployment_packages` (`id`) ON UPDATE CASCADE ON DELETE CASCADE
§§) ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
§§CREATE TABLE IF NOT EXISTS fluffi_gm.systems_location (
§§	`System` INT(11) NOT NULL,
§§	`Location` INT(11) NULL DEFAULT NULL,
§§	PRIMARY KEY (`System`),
§§	INDEX `systems_location_FK1` (`System`),
§§	CONSTRAINT `systems_location_FK1` FOREIGN KEY (`System`) REFERENCES `systems` (`id`) ON UPDATE CASCADE ON DELETE CASCADE,
§§	CONSTRAINT `systems_location_FK2` FOREIGN KEY (`Location`) REFERENCES `locations` (`id`) ON UPDATE CASCADE ON DELETE CASCADE
§§) ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
§§CREATE TABLE IF NOT EXISTS fluffi_gm.gm_options (
§§	`id` INT(11) NOT NULL AUTO_INCREMENT,
§§	`setting` VARCHAR(150) NOT NULL,
§§	`value` VARCHAR(150) NOT NULL DEFAULT "",
§§	PRIMARY KEY (`id`),
§§	UNIQUE (`setting`)
§§) ENGINE=InnoDB DEFAULT CHARSET=utf8;
§§
§§CREATE TABLE IF NOT EXISTS fluffi_gm.system_fuzzjob_instances (
§§	`id` INT(11) NOT NULL AUTO_INCREMENT,
§§	`System` INT(11) NOT NULL,
§§	`Fuzzjob` INT(11) NULL DEFAULT NULL,
§§	`AgentType` int(11) NOT NULL,
§§	`InstanceCount` INT(11) NOT NULL,
§§	`Architecture` VARCHAR(10) NOT NULL,
§§	PRIMARY KEY (`id`),
§§	UNIQUE KEY(`System`, `Fuzzjob`, `AgentType`),
§§	INDEX `system_fuzzjob_instances_FK1` (`System`),
§§	CONSTRAINT `system_fuzzjob_instances_FK1` FOREIGN KEY (`System`) REFERENCES `systems` (`id`) ON UPDATE CASCADE ON DELETE CASCADE,
§§	CONSTRAINT `system_fuzzjob_instances_FK2` FOREIGN KEY (`Fuzzjob`) REFERENCES `fuzzjob` (`id`) ON UPDATE CASCADE ON DELETE CASCADE
§§) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT IGNORE INTO `gm_options` (`id`, `setting`, `value`) VALUES (1, 'bootsystemdir', 'odroidRoot');
INSERT IGNORE INTO `gm_options` (`id`, `setting`, `value`) VALUES (2, 'agentstartermode', 'active');
§§INSERT IGNORE INTO `gm_options` (`id`, `setting`, `value`) VALUES (3, 'checkrating', '-10000');
