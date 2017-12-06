#Author: Hongao Lu
#Uninstall apache2 server and clean all configurations
#!/bin/bash
echo "\n======================================================\n"
echo "Uninstall apache2 server and clean all configurations"
echo "Author: Hongao Lu. Version: 0.01"
echo "\n======================================================\n"
echo "\n>>> Uninstall apache..."
sudo apt-get --purge remove apache-common
sudo apt-get --purge remove apache

echo "\n>>> Clean configurations..."
sudo find /etc -name "*apache*" |xargs  sudo rm -rf 
sudo rm -rf /var/www
sudo rm -rf /etc/libapache2-mod-jk
sudo rm -rf /etc/init.d/apache2
sudo rm -rf /etc/apache2

echo "\n>>> Delete associations..."
dpkg -l |grep apache2|awk '{print $2}'|xargs sudo dpkg -P

echo "\n>>> Remove svn..."
sudo apt-get remove subversion
sudo apt-get remove libapache2-svn

echo "\n>>> Verify..."
echo "Note: if nothing is put below then cleaning completed successfully!"
dpkg -l | grep apache
dpkg -l | grep apache2
