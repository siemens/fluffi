/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Junes Najah, Fabian Russwurm
*/

CREATE DATABASE IF NOT EXISTS fluffi;


CREATE TABLE IF NOT EXISTS fluffi.managed_instances (
	`ServiceDescriptorGUID` VARCHAR(50) NOT NULL,
	`ServiceDescriptorHostAndPort` VARCHAR(50) NOT NULL,
	`AgentType` INT(1) NOT NULL,
	`AgentSubType` VARCHAR(50) NOT NULL,
	`Location` VARCHAR(50) NOT NULL,
	`Capabilities` VARCHAR(1000) NULL DEFAULT NULL,
	PRIMARY KEY (`ServiceDescriptorGUID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8; 

CREATE TABLE IF NOT EXISTS fluffi.managed_instances_statuses (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`ServiceDescriptorGUID` VARCHAR(50) NULL,
	`TimeOfStatus` TIMESTAMP NULL DEFAULT NULL,
	`Status` VARCHAR(1000) NULL DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8; 

CREATE TABLE IF NOT EXISTS fluffi.managed_instances_logmessages (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`ServiceDescriptorGUID` VARCHAR(50) NULL,
	`TimeOfInsertion` TIMESTAMP NULL DEFAULT NULL,
	`LogMessage` VARCHAR(2000) NULL DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8; 

CREATE TABLE IF NOT EXISTS fluffi.interesting_testcases (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`CreatorServiceDescriptorGUID` VARCHAR(50) NOT NULL,
	`CreatorLocalID` BIGINT NOT NULL,
	`ParentServiceDescriptorGUID` VARCHAR(50) NOT NULL,
	`ParentLocalID` BIGINT NOT NULL,
	`Rating` INT NOT NULL,
	`RawBytes` LONGBLOB NOT NULL,
	`TestCaseType` INT(1) NOT NULL,
	`TimeOfInsertion` TIMESTAMP NULL DEFAULT NULL,
	`TimeLastChosen` TIMESTAMP NOT NULL,
	`EdgeCoverageHash` CHAR(16) NULL DEFAULT NULL,
	`ChosenCounter` BIGINT NOT NULL DEFAULT 0,
	UNIQUE (`CreatorServiceDescriptorGUID`, `CreatorLocalID`),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8; 

CREATE TABLE IF NOT EXISTS fluffi.edge_coverage (
	`Hash` CHAR(16) NOT NULL,
	`Counter` BIGINT NOT NULL,
	PRIMARY KEY (`Hash`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.completed_testcases (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`CreatorServiceDescriptorGUID` VARCHAR(50) NOT NULL,
	`CreatorLocalID` BIGINT NOT NULL,
	`TimeOfCompletion` TIMESTAMP NULL DEFAULT NULL,
	PRIMARY KEY (`ID`),
	INDEX USING BTREE (`TimeOfCompletion`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.target_modules (
	`ID` TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`ModuleName` VARCHAR(100) NOT NULL,
	`ModulePath` VARCHAR(300) NOT NULL DEFAULT '*',
	`RawBytes` LONGBLOB NULL DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.covered_blocks (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`CreatorTestcaseID` BIGINT NOT NULL,
	`ModuleID` TINYINT UNSIGNED NOT NULL,
	`Offset` BIGINT NOT NULL,
	`TimeOfInsertion` TIMESTAMP NULL DEFAULT NULL,
	FOREIGN KEY (`CreatorTestcaseID`)
      		REFERENCES fluffi.interesting_testcases(`ID`)
      		ON UPDATE CASCADE ON DELETE CASCADE,
	FOREIGN KEY (`ModuleID`)
		REFERENCES fluffi.target_modules(`ID`)
		ON UPDATE RESTRICT ON DELETE CASCADE,
	UNIQUE (`CreatorTestcaseID`, `ModuleID`, `Offset`),
	INDEX(ModuleID, Offset),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.blocks_to_cover (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`ModuleID` TINYINT UNSIGNED NOT NULL,
	`Offset` BIGINT NOT NULL,
	FOREIGN KEY (`ModuleID`)
		REFERENCES fluffi.target_modules(`ID`)
		ON UPDATE RESTRICT ON DELETE CASCADE,
	UNIQUE INDEX(ModuleID, Offset),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.settings (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`SettingName` VARCHAR(100) NOT NULL,
	`SettingValue` VARCHAR(1024) NULL DEFAULT NULL,
	UNIQUE (`SettingName`),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.crash_descriptions (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`CreatorTestcaseID` BIGINT NOT NULL,
	`CrashFootprint` VARCHAR(500) NOT NULL,
	FOREIGN KEY (`CreatorTestcaseID`)
      		REFERENCES fluffi.interesting_testcases(`ID`)
      		ON UPDATE CASCADE ON DELETE CASCADE,
	UNIQUE (`CreatorTestcaseID`),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.billing (
	`ID` INT NOT NULL AUTO_INCREMENT,
	`Resource` VARCHAR(100) NOT NULL,
	`Amount` BIGINT NOT NULL DEFAULT 0,
	UNIQUE (`Resource`),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.nice_names_testcase (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`NiceName` VARCHAR(100) NOT NULL,
	`CreatorServiceDescriptorGUID` VARCHAR(50) NOT NULL,
	`CreatorLocalID` BIGINT NOT NULL,
	UNIQUE (`CreatorServiceDescriptorGUID`, `CreatorLocalID`),
	UNIQUE (`NiceName`),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS fluffi.nice_names_managed_instance (
	`ID` BIGINT NOT NULL AUTO_INCREMENT,
	`NiceName` VARCHAR(100) NOT NULL,
	`ServiceDescriptorGUID` VARCHAR(50) NOT NULL,	
	UNIQUE (`ServiceDescriptorGUID`),	
	UNIQUE (`NiceName`),
	PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT IGNORE INTO fluffi.billing (Resource) VALUES ('RunnerSeconds');
INSERT IGNORE INTO fluffi.billing (Resource) VALUES ('RunTestcasesNoLongerListed');

DELIMITER //
CREATE OR REPLACE PROCEDURE fluffi.populationMinimization()
LANGUAGE SQL
NOT DETERMINISTIC
CONTAINS SQL
SQL SECURITY DEFINER
BEGIN
	DECLARE done INT DEFAULT FALSE;
	DECLARE c, c2 BIGINT;
	DECLARE t, t2, t3 INT;
	DECLARE e INT;
	DECLARE cur1 CURSOR FOR SELECT ID FROM interesting_testcases WHERE TestCaseType = 0;
	DECLARE cur2 CURSOR FOR SELECT ID FROM interesting_testcases WHERE TestCaseType = 0;
	DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;
		
	OPEN cur1;
	
	SET e = 0;
	SELECT LCASE(SettingValue)="true" INTO e FROM settings WHERE SettingName = "populationMinimization";
	
	pop_loop: LOOP
		FETCH cur1 INTO c;

		IF done OR e = 0 THEN
			LEAVE pop_loop;
		END IF;
		
		OPEN cur2;
		less_loop: LOOP
			FETCH cur2 into c2;
			
			IF done THEN
				LEAVE less_loop;
			END IF;
			
			IF c2 > c THEN
					SET t = 42;
					SET t2 = 0;
					SET t3 = 0;
					
					SELECT DISTINCT COUNT(*) INTO t2 FROM covered_blocks
						WHERE CreatorTestcaseID = c;
						
					SELECT DISTINCT COUNT(*) INTO t3 FROM covered_blocks
						WHERE CreatorTestcaseID = c2;
						
					
					IF t2 > 0 AND t3 > 0  AND t2 = t3 THEN
						SELECT DISTINCT COUNT(*) INTO t FROM 
							(SELECT Offset, ModuleID FROM covered_blocks
							WHERE CreatorTestcaseID = c
							UNION ALL
								SELECT Offset, ModuleID FROM covered_blocks
								WHERE CreatorTestcaseID = c2) AS merged
							GROUP BY Offset, ModuleID HAVING count(*) = 1;
						
						IF t = 42 THEN
							UPDATE interesting_testcases SET TestCaseType = 5 WHERE ID = c2;
						END IF;
					END IF;
					
					SET done = FALSE;
			END IF;
		
		END LOOP;
		CLOSE cur2;
		SET done = FALSE;
		
		
	END LOOP;
	
	CLOSE cur1;
END//
DELIMITER ;

CREATE EVENT IF NOT EXISTS fluffi.CallPopulationMinimization
	ON SCHEDULE EVERY 3 HOUR
	STARTS CURRENT_TIMESTAMP
	ON COMPLETION NOT PRESERVE
	ENABLE
	DO CALL fluffi.populationMinimization();
