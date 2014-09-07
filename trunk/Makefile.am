
ACLOCAL_AMFLAGS = -I m4

APL_DIRS	= workspaces wslib3 wslib4 wslib5
SUBDIRS		= doc gnu-apl.d src support-files tools $(APL_DIRS)
DIST_SUBDIRS	= $(SUBDIRS) rpm debian

dist_doc_DATA = README			\
		README-1-prerequisites	\
		README-2-configure	\
		README-3-keyboard	\
		README-4-compliance	\
		README-5-WINDOWS	\
                README-6-porting	\
		README-7-more-info

nobase_dist_sysconf_DATA = gnu-apl.d/preferences

dist_noinst_DATA = Doxyfile

# show make targets
#
.PHONY:	help
help:
	@echo ""
	@echo ""
	@echo "in addition to standard targets (all, install, clean, ...) "
	@echo "the following make targets may be supported (at this level):"
	@echo ""
	@echo "    make help    - print this text"
	@echo "    make DIST    - dist + a Makefile that calls ./configure"
	@echo "    make DOXY    - create Dougen documentation"
	@echo "    make RPM     - create (source and binary) RPMs"
	@echo "    make DEB     - create debian packages"
	@echo ""
	@echo "    make SVNUP   - update from SVN and ./configure"
	@echo ""
	@echo "NOTE: The RPM and DEB targets may fail because they require" 
	@echo "additional tools that may not be present on your machine."
	@echo ""

# doxygen documentation
#
.PHONY:	DOXY
DOXY:
	doxygen

# source and binary RPMs
#
.PHONY:	RPM
RPM:	DIST
	rm -f rpm/rpm-tmp.*
	make -C rpm $@
	mkdir -p ftp-upload
	cp rpm/SRPMS/$(PACKAGE)-$(PACKAGE_VERSION)-0.src.rpm  ftp-upload/
	cp rpm/RPMS/*/$(PACKAGE)-$(PACKAGE_VERSION)-0.*.rpm   ftp-upload/

# source and binary Debian packages
#
.PHONY:	DEB
DEB:	DIST
	rm -Rf debian_tmp
	mkdir debian_tmp
	cp $(distdir).tar.gz \
           debian_tmp/$(PACKAGE)_$(PACKAGE_VERSION).orig.tar.gz
	cd debian_tmp ; tar xf $(PACKAGE)_$(PACKAGE_VERSION).orig.tar.gz
	cp doc/apl.1 debian_tmp/$(distdir)/debian/manpage.1
	cd debian_tmp/$(distdir) ; dpkg-buildpackage
	mkdir -p ftp-upload
	cp debian_tmp/$(PACKAGE)_$(PACKAGE_VERSION)-1_*.deb          ftp-upload/
	cp debian_tmp/$(PACKAGE)_$(PACKAGE_VERSION)-1.debian.tar.gz  ftp-upload/

# update from SVN repository and ./configure
#
.PHONY:	SVNUP
SVNUP:
	svn update
	./configure --disable-maintainer-mode

# a shortcut to configure for troubleshooting
#
.PHONY:	develop
develop:
	./configure --enable-maintainer-mode DEVELOP_WANTED=yes
	make all

# a shortcut to export changes to ../savannah-repo-apl (which is supposed to be
# a checkout of the GNU APL repository ar Savannah
#
.PHONY:	SYNC
SYNC:	DIST
	tar xvzf apl-$(VERSION).tar.gz
	cp -a apl-$(VERSION)/* ../savannah-repo-apl/trunk/
	rm -Rf apl-$(VERSION)
	cd ../savannah-repo-apl ; svn add . --force -q

# build all packages and SYNC
#
.PHONY:	EXPO
EXPO:
	tools/check_src
	svn -m "auto-commit by make EXPO" commit
	svn update
	cd src; source buildtag
	svn -m "auto-commit by make EXPO" commit
	./configure --disable-maintainer-mode
	make
	make dist
	make DEB
	make RPM
	make SYNC

# EXPO1 is EXPO without updating/committing from/to SVN
.PHONY:	EXPO1
EXPO1:
	tools/check_src
	cd src; source buildtag
	./configure --disable-maintainer-mode
	make
	make dist
	make DEB
	make RPM

# dist and then adding Makefile,dist as Makefile to the tar file
DIST:	dist Makefile.dist
	pwd
	mkdir -p tmp
	cd tmp; tar xzf ../apl-$(VERSION).tar.gz;		\
	cp -f ../Makefile.dist apl-$(VERSION)/Makefile;		\
	tar czf ../apl-$(VERSION).tar.gz apl-$(VERSION)
	rm -Rf tmp

am--refresh:
	echo "making am--refresh"

EXTRA_DIST = config.rpath m4/ChangeLog

.PHONY: git-pull
git-pull:
	make -C src $@
	make -C wslib5 $@

