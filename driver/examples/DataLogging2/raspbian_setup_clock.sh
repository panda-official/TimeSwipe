#!/bin/bash

# Install packages
apt update -y
apt install -y ntp

# Disable fake-hwclock
systemctl disable fake-hwclock
update-rc.d -f fake-hwclock remove
apt remove -y fake-hwclock
rm /lib/udev/hwclock-set


# Configure boot options
echo kernel=Image > /boot/config.txt
echo disable_overscan=1 >> /boot/config.txt
echo gpu_mem_256=100 >> /boot/config.txt
echo gpu_mem_512=100 >> /boot/config.txt
echo gpu_mem_1024=100 >> /boot/config.txt
echo arm_64bit=1 >> /boot/config.txt
echo dtparam=i2c=on >> /boot/config.txt
echo dtparam=i2c_vc=on >> /boot/config.txt

# Auto link dcf clock
echo 'KERNEL=="ttyUSB*", ATTRS{idProduct}=="e88a", SYMLINK+="refclock-0"' > /etc/udev/rules.d/05-DCF77.rules


# Configure ntp
echo server 127.127.1.0 > /etc/ntp.conf
echo fudge 127.127.1.0 stratum 10 >> /etc/ntp.conf
echo server 127.127.8.0 mode 19 prefer >> /etc/ntp.conf
echo logfile /var/log/ntp >> /etc/ntp.conf
echo logconfig =all >> /etc/ntp.conf
echo driftfile /var/lib/ntp/ntp.drift >> /etc/ntp.conf

# Configure rtc auto update
echo '5 * * * * hwclock -w' >> /etc/cron.d/system

# Create time rtc activation script
echo '#!/bin/bash' > /bin/rtc-clock
echo 'modprobe i2c-dev' >> /bin/rtc-clock
echo 'modprobe i2c-bcm2835' >> /bin/rtc-clock
echo 'modprobe rtc-ds1307' >> /bin/rtc-clock
echo 'echo ds1307 0x68 > /sys/class/i2c-adapter/i2c-1/new_device' >> /bin/rtc-clock
echo 'hwclock -s' >> /bin/rtc-clock
chmod +x /bin/rtc-clock


# Get time at startup
echo '[Unit]' > /etc/systemd/system/rtc.service
echo 'Description=Spark service' >> /etc/systemd/system/rtc.service
echo '[Service]' >> /etc/systemd/system/rtc.service
echo 'ExecStart=/bin/rtc-clock' >> /etc/systemd/system/rtc.service
echo '[Install]' >> /etc/systemd/system/rtc.service
echo 'WantedBy=multi-user.target' >> /etc/systemd/system/rtc.service

systemctl daemon-reload
systemctl enable rtc
systemctl enable ntpd

# Apply all changes
reboot
