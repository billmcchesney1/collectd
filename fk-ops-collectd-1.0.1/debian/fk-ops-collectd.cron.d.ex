#
# Regular cron jobs for the fk-ops-collectd package
#
0 4	* * *	root	[ -x /usr/bin/fk-ops-collectd_maintenance ] && /usr/bin/fk-ops-collectd_maintenance
