SUBDIRS = src
dist_doc_DATA = README example.conf

man_MANS = xchainkeys.1
EXTRA_DIST = xchainkeys.1

install-data-hook : 
	$(SED) -i 's|__PREFIX__|@prefix@|g' $(DESTDIR)$(man1dir)/xchainkeys.1
