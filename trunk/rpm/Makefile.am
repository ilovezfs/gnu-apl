
TAR_FILENAME = $(PACKAGE)-$(VERSION).tar.gz
RPM_DIRS = BUILD BUILDROOT SOURCES RPMS SPECS SRPMS
RPM_ARGS = -ba SPECS/apl.spec --buildroot $(abs_top_builddir)/rpm/BUILDROOT

.PHONY: RPM
RPM:
	rm -Rf $(RPM_DIRS)
	mkdir  $(RPM_DIRS)
	cp apl.spec SPECS/
	cp ../$(TAR_FILENAME) SOURCES/
	rpmbuild $(RPM_ARGS)

clean-local:
	rm -Rf BUILD BUILDROOT SOURCES RPMS SPECS SRPMS

