#!/bin/bash
cp ./src/cgi/rpc.fcgi /var/www/html/cgi-bin/
cp ./src/server/anyconn /usr/share/

chmod 755 ./anyconn_service.sh
rm -rf /etc/init.d/anyconn_service.sh
cp ./anyconn_service.sh /etc/init.d/

cd /etc/init.d
update-rc.d anyconn_service.sh defaults 99

