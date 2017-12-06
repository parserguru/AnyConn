#!/bin/bash
### BEGIN INIT INFO
# Provides:          http://www.sinbest-international.com/contact.html
# Required-Start:    $local_fs $network
# Required-Stop:     $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: anyconn service
# Description:       anyconn service daemon
### END INIT INFO

#start anyconn service
echo ">>> anyconn service is to be on..."
/usr/share/anyconn

exit 0
