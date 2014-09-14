1. Create database 'yourdatabasename'.
2. Import sensors.sql dump into the database 'yourdatabasename'.
3. Create whsnbg user for the database 'yourdatabasename' with at least SELECT, INSERT and UPDATE privileges.
4. Permanently activate the event scheduler: write event_scheduler=1 somewhere under the [mysqld] section in the default mysql config file, usually /etc/my.cnf or my.ini.
5. Please be sure all the following events in 'yourdatabasename' are enabled:
calc_data_float_day_for_yesterday
calc_data_float_hour_for_yesterday
calc_data_float_month_for_last_month
calc_data_long_day_for_yesterday
calc_data_long_hour_for_yesterday
calc_data_long_month_for_last_month
6. Edit MySQL section in the whsnbg.conf file.
