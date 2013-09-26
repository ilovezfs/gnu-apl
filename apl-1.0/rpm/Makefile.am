
TAR_FILENAME = $(PACKAGE)-$(VERSION).tar.gz
RPM_DIRS = BUILD BUILDROOT SOURCES RPMS SPECS SRPMS

.PHONY: RPM
RPM:
	rm -Rf $(RPM_DIRS)
	mkdir  $(RPM_DIRS)
	cp apl.spec SPECS/
	cp ../$(TAR_FILENAME) SOURCES/
	rpmbuild -ba SPECS/apl.spec --buildroot $(abs_top_builddir)/rpm/BUILDROOT

clean-local:
	rm -Rf BUILD BUILDROOT SOURCES RPMS SPECS SRPMS

