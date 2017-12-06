#!/bin/bash
rm -rf /var/www/html/cgi-bin/rpc.fcgi
rm -rf /usr/share/anyconn

cd /etc/init.d
sudo update-rc.d -f anyconn_service.sh remove

rm -rf /etc/init.d/anyconn_service.sh
