SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;


DELIMITER $$
CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_float_day_for_date`(IN `date` DATE)
BEGIN

DECLARE done BOOLEAN DEFAULT FALSE;
DECLARE beg DATETIME;
DECLARE idd, par INT UNSIGNED;
DECLARE cur CURSOR FOR SELECT `sensor_id`, `sensor_param` FROM `parameters`;
DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

OPEN cur;

SET beg = TIMESTAMP(date, MAKETIME(0, 0, 0));

mainLoop: LOOP
  FETCH cur INTO idd, par;
  IF done THEN
    LEAVE mainLoop;
  END IF;
  CALL calc_data_float_day_for_id_par_beg(idd, par, beg);
END LOOP mainLoop;

CLOSE cur;
  
END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_float_day_for_id_par_beg`(IN `idd` INT(4) UNSIGNED, IN `par` INT(4) UNSIGNED, IN `beg` DATETIME)
BEGIN

#beg is the beginning of the day in the DATETIME format like '2013-12-20 00:00:00'

DECLARE end DATETIME;
SET end = DATE_ADD(beg, INTERVAL 86399 SECOND);

SELECT AVG(`value_avg`)
FROM(SELECT `value_avg`
FROM `data_float_hour`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @avg;

SELECT MIN(`value_min`)
FROM(SELECT `value_min`
FROM `data_float_hour`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @min;

SELECT MAX(`value_max`)
FROM(SELECT `value_max`
FROM `data_float_hour`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @max;

IF (@avg IS NOT NULL) THEN
 INSERT INTO `data_float_day` (`sensor_id`, `sensor_param`, `time`, `value_avg`, `value_min`, `value_max`)
 VALUES (idd, par, beg, @avg, @min, @max)
 ON DUPLICATE KEY UPDATE value_avg=@avg, value_min=@min, value_max=@max;
END IF;

END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_float_hour_for_date`(IN `date` DATE)
BEGIN

DECLARE done BOOLEAN DEFAULT FALSE;
DECLARE beg DATETIME;
DECLARE idd, par, hour INT UNSIGNED;
DECLARE cur CURSOR FOR SELECT `sensor_id`, `sensor_param` FROM `parameters`;
DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

OPEN cur;

mainLoop: LOOP
  FETCH cur INTO idd, par;
  IF done THEN
    LEAVE mainLoop;
  END IF;
  SET hour = 0;
  WHILE hour < 24 DO
    SET beg = TIMESTAMP(date, MAKETIME(hour, 0, 0));
    CALL calc_data_float_hour_for_id_par_beg(idd, par, beg);
    SET hour = hour + 1;
  END WHILE;
END LOOP mainLoop;

CLOSE cur;
  
END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_float_hour_for_id_par_beg`(IN `idd` INT(4) UNSIGNED, IN `par` INT(4) UNSIGNED, IN `beg` DATETIME)
BEGIN

#beg is the beginning of the hour in the DATETIME format like '2013-12-20 04:00:00'

DECLARE end DATETIME;
SET end = DATE_ADD(beg, INTERVAL 3599 SECOND);

SELECT AVG(`value`)
FROM(SELECT `value`
FROM `data_float`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @avg;

SELECT MIN(`value`)
FROM(SELECT `value`
FROM `data_float`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @min;

SELECT MAX(`value`)
FROM(SELECT `value`
FROM `data_float`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @max;

IF (@avg IS NOT NULL) THEN
 INSERT INTO `data_float_hour` (`sensor_id`, `sensor_param`, `time`, `value_avg`, `value_min`, `value_max`)
 VALUES (idd, par, beg, @avg, @min, @max)
 ON DUPLICATE KEY UPDATE value_avg=@avg, value_min=@min, value_max=@max;
END IF;

END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_float_month_for_date`(IN `date` DATE)
BEGIN

#date is the beginning of the month in the DATE format like '2013-12-01'

DECLARE done BOOLEAN DEFAULT FALSE;
DECLARE beg DATETIME;
DECLARE idd, par INT UNSIGNED;
DECLARE cur CURSOR FOR SELECT `sensor_id`, `sensor_param` FROM `parameters`;
DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

OPEN cur;

SET beg = TIMESTAMP(date, MAKETIME(0, 0, 0));

mainLoop: LOOP
  FETCH cur INTO idd, par;
  IF done THEN
    LEAVE mainLoop;
  END IF;
  CALL calc_data_float_month_for_id_par_beg(idd, par, beg);
END LOOP mainLoop;

CLOSE cur;
  
END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_float_month_for_id_par_beg`(IN `idd` INT(4) UNSIGNED, IN `par` INT(4) UNSIGNED, IN `beg` DATETIME)
BEGIN

#beg is the beginning of the month in the DATETIME format like '2013-12-01 00:00:00'

DECLARE end DATETIME;
SET end = TIMESTAMP(LAST_DAY(beg), MAKETIME(23, 59, 59));

SELECT AVG(`value_avg`)
FROM(SELECT `value_avg`
FROM `data_float_day`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @avg;

SELECT MIN(`value_min`)
FROM(SELECT `value_min`
FROM `data_float_day`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @min;

SELECT MAX(`value_max`)
FROM(SELECT `value_max`
FROM `data_float_day`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @max;

IF (@avg IS NOT NULL) THEN
 INSERT INTO `data_float_month` (`sensor_id`, `sensor_param`, `time`, `value_avg`, `value_min`, `value_max`)
 VALUES (idd, par, beg, @avg, @min, @max)
 ON DUPLICATE KEY UPDATE value_avg=@avg, value_min=@min, value_max=@max;
END IF;

END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_long_day_for_date`(IN `date` DATE)
BEGIN

DECLARE done BOOLEAN DEFAULT FALSE;
DECLARE beg DATETIME;
DECLARE idd, par INT UNSIGNED;
DECLARE cur CURSOR FOR SELECT `sensor_id`, `sensor_param` FROM `parameters`;
DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

OPEN cur;

SET beg = TIMESTAMP(date, MAKETIME(0, 0, 0));

mainLoop: LOOP
  FETCH cur INTO idd, par;
  IF done THEN
    LEAVE mainLoop;
  END IF;
  CALL calc_data_long_day_for_id_par_beg(idd, par, beg);
END LOOP mainLoop;

CLOSE cur;
  
END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_long_day_for_id_par_beg`(IN `idd` INT(4) UNSIGNED, IN `par` INT(4) UNSIGNED, IN `beg` DATETIME)
BEGIN

#beg is the beginning of the day in the DATETIME format like '2013-12-20 00:00:00'

DECLARE end DATETIME;
SET end = DATE_ADD(beg, INTERVAL 86399 SECOND);

SELECT AVG(`value_avg`)
FROM(SELECT `value_avg`
FROM `data_long_hour`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @avg;

SELECT MIN(`value_min`)
FROM(SELECT `value_min`
FROM `data_long_hour`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @min;

SELECT MAX(`value_max`)
FROM(SELECT `value_max`
FROM `data_long_hour`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @max;

IF (@avg IS NOT NULL) THEN
 INSERT INTO `data_long_day` (`sensor_id`, `sensor_param`, `time`, `value_avg`, `value_min`, `value_max`)
 VALUES (idd, par, beg, @avg, @min, @max)
 ON DUPLICATE KEY UPDATE value_avg=@avg, value_min=@min, value_max=@max;
END IF;

END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_long_hour_for_date`(IN `date` DATE)
BEGIN

DECLARE done BOOLEAN DEFAULT FALSE;
DECLARE beg DATETIME;
DECLARE idd, par, hour INT UNSIGNED;
DECLARE cur CURSOR FOR SELECT `sensor_id`, `sensor_param` FROM `parameters`;
DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

OPEN cur;

mainLoop: LOOP
  FETCH cur INTO idd, par;
  IF done THEN
    LEAVE mainLoop;
  END IF;
  SET hour = 0;
  WHILE hour < 24 DO
    SET beg = TIMESTAMP(date, MAKETIME(hour, 0, 0));
    CALL calc_data_long_hour_for_id_par_beg(idd, par, beg);
    SET hour = hour + 1;
  END WHILE;
END LOOP mainLoop;

CLOSE cur;
  
END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_long_hour_for_id_par_beg`(IN `idd` INT(4) UNSIGNED, IN `par` INT(4) UNSIGNED, IN `beg` DATETIME)
BEGIN

#beg is the beginning of the hour in the DATETIME format like '2013-12-20 04:00:00'

DECLARE end DATETIME;
SET end = DATE_ADD(beg, INTERVAL 3599 SECOND);

SELECT AVG(`value`)
FROM(SELECT `value`
FROM `data_long`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @avg;

SELECT MIN(`value`)
FROM(SELECT `value`
FROM `data_long`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @min;

SELECT MAX(`value`)
FROM(SELECT `value`
FROM `data_long`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @max;

IF (@avg IS NOT NULL) THEN
 INSERT INTO `data_long_hour` (`sensor_id`, `sensor_param`, `time`, `value_avg`, `value_min`, `value_max`)
 VALUES (idd, par, beg, @avg, @min, @max)
 ON DUPLICATE KEY UPDATE value_avg=@avg, value_min=@min, value_max=@max;
END IF;

END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_long_month_for_date`(IN `date` DATE)
BEGIN

#date is the beginning of the month in the DATE format like '2013-12-01'

DECLARE done BOOLEAN DEFAULT FALSE;
DECLARE beg DATETIME;
DECLARE idd, par INT UNSIGNED;
DECLARE cur CURSOR FOR SELECT `sensor_id`, `sensor_param` FROM `parameters`;
DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

OPEN cur;

SET beg = TIMESTAMP(date, MAKETIME(0, 0, 0));

mainLoop: LOOP
  FETCH cur INTO idd, par;
  IF done THEN
    LEAVE mainLoop;
  END IF;
  CALL calc_data_long_month_for_id_par_beg(idd, par, beg);
END LOOP mainLoop;

CLOSE cur;
  
END$$

CREATE DEFINER=`root`@`%` PROCEDURE `calc_data_long_month_for_id_par_beg`(IN `idd` INT(4) UNSIGNED, IN `par` INT(4) UNSIGNED, IN `beg` DATETIME)
BEGIN

#beg is the beginning of the month in the DATETIME format like '2013-12-01 00:00:00'

DECLARE end DATETIME;
SET end = TIMESTAMP(LAST_DAY(beg), MAKETIME(23, 59, 59));

SELECT AVG(`value_avg`)
FROM(SELECT `value_avg`
FROM `data_long_day`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @avg;

SELECT MIN(`value_min`)
FROM(SELECT `value_min`
FROM `data_long_day`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @min;

SELECT MAX(`value_max`)
FROM(SELECT `value_max`
FROM `data_long_day`
WHERE `time` >= beg
AND `time` <= end
AND `sensor_id` = idd
AND `sensor_param` = par)
AS temp
INTO @max;

IF (@avg IS NOT NULL) THEN
 INSERT INTO `data_long_month` (`sensor_id`, `sensor_param`, `time`, `value_avg`, `value_min`, `value_max`)
 VALUES (idd, par, beg, @avg, @min, @max)
 ON DUPLICATE KEY UPDATE value_avg=@avg, value_min=@min, value_max=@max;
END IF;

END$$

DELIMITER ;

CREATE TABLE IF NOT EXISTS `data_float` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value` float NOT NULL,
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`),
  KEY `time` (`sensor_id`,`sensor_param`,`time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_float_day` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value_avg` float NOT NULL,
  `value_min` float NOT NULL,
  `value_max` float NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`,`time`),
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_float_hour` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value_avg` float NOT NULL,
  `value_min` float NOT NULL,
  `value_max` float NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`,`time`),
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_float_month` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value_avg` float NOT NULL,
  `value_min` float NOT NULL,
  `value_max` float NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`,`time`),
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_long` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value` int(4) NOT NULL,
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`),
  KEY `time` (`sensor_id`,`sensor_param`,`time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_long_day` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value_avg` float NOT NULL,
  `value_min` int(4) NOT NULL,
  `value_max` int(4) NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`,`time`),
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_long_hour` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value_avg` float NOT NULL,
  `value_min` int(4) NOT NULL,
  `value_max` int(4) NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`,`time`),
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_long_month` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value_avg` float NOT NULL,
  `value_min` int(4) NOT NULL,
  `value_max` int(4) NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`,`time`),
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `data_ut8str` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `time` datetime NOT NULL,
  `value` varchar(73) NOT NULL,
  KEY `id` (`sensor_id`),
  KEY `parameter` (`sensor_id`,`sensor_param`),
  KEY `time` (`sensor_id`,`sensor_param`,`time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `parameters` (
  `sensor_id` int(4) unsigned NOT NULL,
  `sensor_param` int(4) unsigned NOT NULL,
  `unit` varchar(32) CHARACTER SET utf8 NOT NULL,
  PRIMARY KEY (`sensor_id`,`sensor_param`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `sensors` (
  `sensor_id` int(4) unsigned NOT NULL,
  `st_duration` int(4) unsigned NOT NULL,
  `location` varchar(128) CHARACTER SET utf8 NOT NULL,
  `sensor_ip` char(15) CHARACTER SET ascii NOT NULL,
  PRIMARY KEY (`sensor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DELIMITER $$
CREATE DEFINER=`root`@`%` EVENT `calc_data_float_month_for_last_month` ON SCHEDULE EVERY 1 MONTH STARTS '2014-04-01 00:15:00' ON COMPLETION NOT PRESERVE ENABLE DO CALL calc_data_float_month_for_date(DATE_ADD(LAST_DAY(DATE_SUB(NOW(), INTERVAL 2 MONTH)), INTERVAL 1 DAY))$$

CREATE DEFINER=`root`@`%` EVENT `calc_data_float_day_for_yesterday` ON SCHEDULE EVERY 1 DAY STARTS '2014-03-14 00:10:00' ON COMPLETION NOT PRESERVE ENABLE DO CALL calc_data_float_day_for_date(DATE_ADD(CURDATE(), INTERVAL -1 DAY))$$

CREATE DEFINER=`root`@`%` EVENT `calc_data_float_hour_for_yesterday` ON SCHEDULE EVERY 1 DAY STARTS '2014-03-14 00:05:00' ON COMPLETION NOT PRESERVE ENABLE DO CALL calc_data_float_hour_for_date(DATE_ADD(CURDATE(), INTERVAL -1 DAY))$$

CREATE DEFINER=`root`@`%` EVENT `calc_data_long_hour_for_yesterday` ON SCHEDULE EVERY 1 DAY STARTS '2014-03-14 00:05:00' ON COMPLETION NOT PRESERVE ENABLE DO CALL calc_data_long_hour_for_date(DATE_ADD(CURDATE(), INTERVAL -1 DAY))$$

CREATE DEFINER=`root`@`%` EVENT `calc_data_long_day_for_yesterday` ON SCHEDULE EVERY 1 DAY STARTS '2014-03-14 00:10:00' ON COMPLETION NOT PRESERVE ENABLE DO CALL calc_data_long_day_for_date(DATE_ADD(CURDATE(), INTERVAL -1 DAY))$$

CREATE DEFINER=`root`@`%` EVENT `calc_data_long_month_for_last_month` ON SCHEDULE EVERY 1 MONTH STARTS '2014-04-01 00:15:00' ON COMPLETION NOT PRESERVE ENABLE DO CALL calc_data_long_month_for_date(DATE_ADD(LAST_DAY(DATE_SUB(NOW(), INTERVAL 2 MONTH)), INTERVAL 1 DAY))$$

DELIMITER ;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
