
# ABOUT

**dsbbatmon**
is a Qt battery monitor for FreeBSD. It displays the battery's current status,
capacity, and remaining time when you move the mouse cursor over its tray icon.
**dsbbatmon**
can automatically shut down or suspend your system if the battery's
capacity falls short of a certain percental value.

# INSTALLATION

## Dependencies

**dsbbatmon**
depends on devel/qt5-buildtools, devel/qt5-core, devel/qt5-linguisttools,
devel/qt5-qmake, x11-toolkits/qt5-gui, and x11-toolkits/qt5-widgets

## Building and installation

	# git clone https://github.com/mrclksr/DSBBatmon.git
	# git clone https://github.com/mrclksr/dsbcfg.git
	
	# cd DSBBatmon && qmake && make
	# make install

# SETUP

## Permissions

In order to be able to execute
*shutdown*
as regular user, you can either use
sudo(8) (see below), or you can add your username to the
*operator*
group:

	# pw groupmod operator -m yourusername

If you want to be able to suspend your system as
regular user who is member of the wheel group, you can use
sudo(8).
Add

	%wheel  ALL=(ALL) NOPASSWD: /usr/sbin/acpiconf *

to
*/usr/local/etc/sudoers*.

**Note**:
On FreeBSD &gt;= 12, members of the
*operator*
group are allowed to execute
*acpiconf*.
Using
sudo(8)
is not necessary.

